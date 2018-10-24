#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "emulator.h"//
#include "emulator_function.h"//
#include "instruction.h"

//1MB
#define MEMORY_SIZE (1024 * 1024)

char* registers_name[] = {"EAX", "ECX", "EDX", "EBX", "ESP", "EBP", "ESI", "EDI"};

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
		bootinfo[4] = 0x0000;
		bootinfo[4] = 0xe000;
	
	}
    fclose(binary);
}

//レジスタ表示
static void dump_registers(Emulator* emu)
{
    int i;
    for (i = 0; i < REGISTERS_COUNT; i++) {
        printf("%s = %08x\n", registers_name[i], get_register32(emu, i));
    }
    printf("EIP = %08x\n", emu->eip);
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

int main(int argc, char* argv[])
{
    Emulator* emu;
    int i;
    int quiet = 0, haribote = 0, memsiz = MEMORY_SIZE;

    //-q, -hオプション
    i = 1;
    while (i < argc) {
        if (strcmp(argv[i], "-q") == 0) {
            quiet = 1;
            argc = opt_remove_at(argc, argv, i);
		} else if (strcmp(argv[i], "-h") == 0) {
            haribote = 1;
            argc = opt_remove_at(argc, argv, i);
			memsiz = 32 * 1024 * 1024;
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

    ///eip,esp=0x7c00 memory1MBのemu作成
   	emu = create_emu(memsiz, 0x7c00, 0x7c00);

    read_binary(emu, argv[1], haribote);

    while (emu->eip < memsiz) {
        uint8_t code = get_code8(emu, 0);
        //バイナリ出力
        if (!quiet) {
            printf("EIP = %X, Code = %02X\n", emu->eip, code);
            //printf("esp=%x\n",emu->registers[ESP]);
        }

        if (instructions[code] == NULL) {
            //opecode未実装
            printf("\n\nNot Implemented: %x\n", code);
			printf("[%02x %02x %02x %02x]\n", get_code8(emu, 0), get_code8(emu, 1), get_code8(emu, 2), get_code8(emu, 3));
            break;
        }

        //命令実行
        instructions[code](emu);

        /* EIPが0になったらプログラム終了 */
        if (emu->eip == 0) {
            printf("\n\nend of program.\n\n");
            break;
        }
    }

    dump_registers(emu);
    destroy_emu(emu);
    return 0;
}

