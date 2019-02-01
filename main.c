#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <GL/freeglut.h>

#include "emulator.h"
#include "emulator_function.h"
#include "instruction.h"

//1024MB
#define MEMORY_SIZE (1024 * 1024*1024)

char* registers_name[] = {"EAX", "ECX", "EDX", "EBX", "ESP", "EBP", "ESI", "EDI"};

Emulator* emu;

int i;//命令の実行回数

//emuのメモリに512byteコピー
static void read_binary(Emulator* emu, const char* filename, int haribote)
{
    FILE* binary;
    binary = fopen(filename, "rb");

    if (binary == NULL) {
        printf("%s :cannot read the file\n", filename);
        exit(1);
    }

	if (haribote == 0) {
	    //Emulatorのメモリにバイナリファイルを512バイトコピー
    	fread(emu->memory + 0x7c00, 1, 0x200, binary);
	} else {
		fread(emu->memory + 0x00100000, 1, 1440 * 1024, binary);
		memcpy(emu->memory + 0x00280000, emu->memory + 0x00104390, 512 * 1024);
		uint32_t* bootpack = (uint32_t*) (emu->memory + 0x00280000);
		memcpy(emu->memory + bootpack[3], emu->memory + (bootpack[5] + 0x00280000), bootpack[4]);
		emu->registers[ESP] = bootpack[3];
		emu->segBase[1] = 0x00280000;
		emu->eip = 0x001b;
		emu->seg[0] = 1 * 8;
		emu->seg[1] = 2 * 8;
		emu->seg[2] = 1 * 8;
		emu->seg[3] = 1 * 8;
		uint16_t* bootinfo = (uint16_t*) emu->memory + 0x0ff0;
		bootinfo[1] = 8;
		bootinfo[2] = 640;
		bootinfo[3] = 480;
		bootinfo[4] = 0x01f0;
	}
    fclose(binary);
}

//emu作成
static Emulator* create_emu(size_t size, uint32_t eip, uint32_t esp)
{
    Emulator* emu = malloc(sizeof(Emulator));
    emu->memory = malloc(size);//確保

    /* 汎用レジスタを全て0にする */
    memset(emu->registers, 0, sizeof(emu->registers));

    emu->eip = eip;//eip初期値

    /* スタックポインタの初期値 */
    emu->registers[ESP] = esp;

    memset(emu->segBase, 0, sizeof(emu->segBase));
    memset(emu->seg, 0, sizeof(emu->seg));

    //
    emu->stackcnt=0;
    memset(emu->eipstack, 0, sizeof(emu->eipstack));
    //

    opsiz=0;


    return emu;
}

static void destroy_emu(Emulator* emu)
{
    free(emu->memory);
    free(emu);
}

int opt_remove_at(int argc, char* argv[], int index)
{
    if (index < 0 || argc <= index) {
        return argc;
    } else {
        int i = index;
        for (; i < argc - 1; i++) {
            argv[i] = argv[i + 1];
        }
        argv[i] = NULL;
        return argc - 1;
    }
}

//レジスタ表示
static void dump_registers(Emulator* emu)
{
    puts("---registers---");
    int i;
    for (i = 0; i < REGISTERS_COUNT; i++) {
        printf("%s = %08x\n", registers_name[i], get_register32(emu, i));
    }
    printf("EIP = %08x\n", emu->eip);
}

static void dump_eflags(Emulator* emu)
{
    puts("---eflags---");
    printf("CF = %d\n",is_carry(emu));
    printf("OF = %d\n",is_overflow(emu));
    printf("SF = %d\n",is_sign(emu));
    printf("ZF = %d\n",is_zero(emu));
}


void dump_bin(Emulator* emu){
    printf("\ni=%d\n",i);
    printf("\n[%02X %02X %02X %02X %02X %02X %02X %02X]\n", get_code8(emu, -8), get_code8(emu, -7), get_code8(emu, -6), get_code8(emu, -5), get_code8(emu, -4), get_code8(emu, -3), get_code8(emu, -2), get_code8(emu, -1));
    printf("[%02X %02X %02X %02X %02X %02X %02X %02X]\n\n", get_code8(emu, 0), get_code8(emu, 1), get_code8(emu, 2), get_code8(emu, 3), get_code8(emu, 4), get_code8(emu, 5), get_code8(emu, 6), get_code8(emu, 7));
}

void dump_mem(Emulator* emu,uint32_t addr){
    printf("\n[%02X %02X %02X %02X]\n", emu->memory[addr-4],emu->memory[addr-3],emu->memory[addr-2],emu->memory[addr-1]);
    printf("[%02X %02X %02X %02X]\n\n", emu->memory[addr],emu->memory[addr+1],emu->memory[addr+2],emu->memory[addr+3]);
}

void dump_eipstack(Emulator* emu){
    printf("\nstackcnt=%d\n",emu->stackcnt);
    puts("--------");
    int i;
    for(i=0;i<emu->stackcnt;i++){
        printf(" %08X\n",emu->eipstack[i]);
    }
    puts("--------");
}



int scrnx=320,scrny=200;
static unsigned char table_rgb[16 * 3] = {
        0x00, 0x00, 0x00,
        0xff, 0x00, 0x00,
        0x00, 0xff, 0x00,
        0xff, 0xff, 0x00,
        0x00, 0x00, 0xff,
        0xff, 0x00, 0xff,
        0x00, 0xff, 0xff,
        0xff, 0xff, 0xff,
        0xc6, 0xc6, 0xc6,
        0x84, 0x00, 0x00,
        0x00, 0x84, 0x00,
        0x84, 0x84, 0x00,
        0x00, 0x00, 0x84,
        0x84, 0x00, 0x84,
        0x00, 0x84, 0x84,
        0x84, 0x84, 0x84
    };



void display () {
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear (GL_COLOR_BUFFER_BIT);
    /*
    glBegin (GL_LINE_LOOP);
        glVertex2d (-0.9, -0.9);
        glVertex2d (0.9, -0.9);
        glVertex2d (0, 0.9);
    glEnd ();*/

    glPointSize(1.0);
    
    //VRAMよみだし
    glBegin(GL_POINTS);
        glColor4f(0.7, 0.2, 0.2, 0.0);//いろ
        glVertex2f(0 , -0.9);
        glVertex2f(0.0, 0.5);

        int offs=0xa0000;
        
        int x,y;
        for(x=0;x<scrnx;x++){
            for(y=0;y<scrny;y++){
                int data=emu->memory[x+y*scrnx+offs];
                if(data!=0)exit(0);
                //printf("%d",data);
                glColor3b(table_rgb[data*3],table_rgb[data*3+1],table_rgb[data*3+2]);
                glColor4f(0.7, 0.2, 0.2, 0.0);//いろ
                glVertex2f((float)((x-scrnx/2)/scrnx),(float)((y-scrny/2)/scrny));
            }
            //puts("");
        }

    glEnd();
    //puts("aaa");

    glFlush ();
}


int main(int argc, char* argv[])
{
    int stepup = 0;
    int quiet = 0, haribote = 0, memsiz = MEMORY_SIZE;
    int backup_quiet = 0;//最初の方は表示すると遅いので無条件でquietする
    if(haribote)quiet=1;

    //-q, -h,-sオプション
    i = 1;
    while (i < argc) {
        if(strcmp(argv[i],"-s")==0){
            stepup=1;
            argc = opt_remove_at(argc, argv, i);
        }else if (strcmp(argv[i], "-q") == 0) {
            //quiet = 1;
            backup_quiet=1;
            argc = opt_remove_at(argc, argv, i);
		} else if (strcmp(argv[i], "-h") == 0) {
            haribote = 1;
            argc = opt_remove_at(argc, argv, i);
			memsiz = 32 * 1024 * 1024;

            //create window
            glutInit (&argc, argv);
            glutInitDisplayMode (GLUT_RGBA);
            glutCreateWindow ("x86 Emulator");
            glutReshapeWindow(scrnx, scrny);
            glutDisplayFunc (display);

        } else {
            i++;
        }
    }

    //引数エラー
    if (argc != 2) {
        printf("usage: px86 filename\n");
        return 1;
    }

    init_instructions();

    ///eip=esp=0x7c00 memory1MBのemu作成
   	emu = create_emu(memsiz, 0x7c00, 0x7c00);

    read_binary(emu, argv[1], haribote);
    i=0;
    while (emu->eip < memsiz ||1) {
        i++;
        uint8_t code = get_code8(emu, 0);

        if (!quiet) {
            if(opsiz==1)puts("--16bit mode--");
            if(!stepup){
                printf("%d: EIP = %X Code = %02X ebp:%08X esp:%08X esi:0x%x eax:0x%x ebx:0x%x ecx:0x%x edx:0x%x\n",i, emu->eip, code,emu->registers[EBP],emu->registers[ESP],get_register32(emu,ESI),get_register32(emu,EAX),get_register32(emu,EBX),get_register32(emu,ECX),get_register32(emu,EDX));
            }else{
                printf("\n\n%d: EIP = %X, Code = %02X\n",i, emu->eip,code);
                dump_registers(emu);
                dump_eflags(emu);
                char a[100];
                scanf("%s",a);
                
            }
                //printf("esp=%x\n",emu->registers[ESP]);
        }
        
        if(haribote==1){
            //画面更新
            //dump_mem(emu,0xa0000);
            //glutDisplayFunc(display);
            glutMainLoopEvent();
            //printf("%d\n",get_register16(emu,ESI));
        }

        if (instructions[code] == NULL) {
            //opecode未実装
            printf("\n\nNot Implemented: %X\n", code);
			dump_bin(emu);
            break;
        }

        //if(i==752992)dump_registers(emu),dump_bin(emu);

        //命令実行
        instructions[code](emu);

        if(opsiz!=0)opsiz--;

        //デバッグの時間短縮のために文字出力おふる
        //uint32_t ebxx=emu->registers[EBX];
        //dump_registers(emu);
        //if(ebxx==1)quiet=backup_quiet;
        //if(quiet==1 && ebxx<9000 && code==0x4B){printf("%d\n",ebxx);}

        if(haribote){
            if(i<400390)quiet=1;
            if(i<752980)quiet=1;
            if(i<789200)quiet=1;
            //if(ai>100 && emu->eip>=0x3b8)puts("a");
            else quiet=backup_quiet;
        }   


        /* EIPが0になったらプログラム終了 */
        if (emu->eip == 0) {
            printf("\n\nend of program.(eip=0)\n\n");
            break;
        }
    }

    puts("\n\nend of loop.\n\n");

    dump_bin(emu);
    dump_registers(emu);
    //dump_mem(emu,0x30fb9c);
    dump_eipstack(emu);

    destroy_emu(emu);
    return 0;
}

