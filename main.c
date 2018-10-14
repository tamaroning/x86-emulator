#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "emulator.h"//
#include "emulator_function.h"//
#include "instruction.h"

/* メモリは1MB */
#define MEMORY_SIZE (1024 * 1024)

char* registers_name[] = {"EAX", "ECX", "EDX", "EBX", "ESP", "EBP", "ESI", "EDI"};

//emuのメモリに512byteコピー
static void read_binary(Emulator* emu, const char* filename)
{
    FILE* binary;

    binary = fopen(filename, "rb");

    if (binary == NULL) {
        printf("%s :cannot read the file\n", filename);
        exit(1);
    }

    /* Emulatorのメモリにバイナリファイルの内容を512バイトコピーする */
    fread(emu->memory + 0x7c00, 1, 0x200, binary);
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

    return emu;
}

static void destroy_emu(Emulator* emu)
{
    free(emu->memory);
    free(emu);
}

//whats this
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
    int quiet = 0;

    //-qオプション
    i = 1;
    while (i < argc) {
        if (strcmp(argv[i], "-q") == 0) {
            quiet = 1;
            argc = opt_remove_at(argc, argv, i);
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
    emu = create_emu(MEMORY_SIZE, 0x7c00, 0x7c00);

    read_binary(emu, argv[1]);

    while (emu->eip < MEMORY_SIZE) {
        uint8_t code = get_code8(emu, 0);
        //バイナリ出力
        if (!quiet) {
            printf("EIP = %X, Code = %02X\n", emu->eip, code);
        }

        if (instructions[code] == NULL) {
            //opecode未実装
            printf("\n\nNot Implemented: %x\n", code);
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

