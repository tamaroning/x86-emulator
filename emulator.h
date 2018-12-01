#ifndef EMULATOR_H_
#define EMULATOR_H_

#include <stdint.h>

enum Register { EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI, REGISTERS_COUNT,
                AL = EAX, CL = ECX, DL = EDX, BL = EBX,
                AH = AL + 4, CH = CL + 4, DH = DL + 4, BH = BL + 4,
                AX = EAX, CX = ECX, DX = EDX, BX = EBX,
                SP = ESP, BP = EBP, SI = ESI, DI = EDI
                };

typedef struct {
    uint32_t registers[REGISTERS_COUNT];//汎用レジスタ

    uint32_t eflags;

    uint8_t* memory;

    uint32_t eip;

	uint32_t segBase[8], seg[8];  // CS, DS, SS, ES, FG, GS, TR, LDTR

    uint32_t eipstack[10000];
    uint32_t stackcnt;
} Emulator;

int opsiz;

#endif
