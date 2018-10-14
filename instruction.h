#ifndef INSTRUCTION_H_
#define INSTRUCTION_H_

#include "emulator.h"

/* 命令セットの初期化関数 */
void init_instructions(void);

typedef void instruction_func_t(Emulator*);

//index=opecode
extern instruction_func_t* instructions[256];

#endif
