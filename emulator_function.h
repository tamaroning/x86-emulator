#ifndef EMULATOR_FUNCTION_H_
#define EMULATOR_FUNCTION_H_

#include <stdint.h>

#include "emulator.h"

// eflagsのビットフラグ
#define CARRY_FLAG (1)
#define ZERO_FLAG (1 << 6)
#define SIGN_FLAG (1 << 7)
#define OVERFLOW_FLAG (1 << 11)

#define PARITY_FLAG (1 << 2)
#define INTERRUPT_FLAG (1 << 9)

/* プログラムカウンタから相対位置にある符号無し8bit値を取得 */
uint32_t get_code8(Emulator* emu, int index);

/* プログラムカウンタから相対位置にある符号付き8bit値を取得 */
int32_t get_sign_code8(Emulator* emu, int index);

/* プログラムカウンタから相対位置にある符号無し32bit値を取得 */
uint32_t get_code32(Emulator* emu, int index);

/* プログラムカウンタから相対位置にある符号付き32bit値を取得 */
int32_t get_sign_code32(Emulator* emu, int index);

/* index番目の8bit汎用レジスタの値を取得する */
uint8_t get_register8(Emulator* emu, int index);

uint16_t get_register16(Emulator* emu, int index);
/* index番目の32bit汎用レジスタの値を取得する */
uint32_t get_register32(Emulator* emu, int index);

/* index番目の8bit汎用レジスタに値を設定する */
void set_register8(Emulator* emu, int index, uint8_t value);

void set_register16(Emulator* emu, int index, uint16_t value);

/* index番目の32bit汎用レジスタに値を設定する */
void set_register32(Emulator* emu, int index, uint32_t value);

/* メモリのindex番地の8bit値を取得する */
uint32_t get_memory8(Emulator* emu, uint32_t address);

uint32_t get_memory16(Emulator* emu, uint32_t address);
/* メモリのindex番地の32bit値を取得する */
uint32_t get_memory32(Emulator* emu, uint32_t address);

/* メモリのindex番地に8bit値を設定する */
void set_memory8(Emulator* emu, uint32_t address, uint32_t value);

void set_memory16(Emulator* emu, uint32_t address, uint32_t value);

/* メモリのindex番地に32bit値を設定する */
void set_memory32(Emulator* emu, uint32_t address, uint32_t value);

/* スタックに32bit値を積む */
void push32(Emulator* emu, uint32_t value);

/* スタックから32bit値を取りだす */
uint32_t pop32(Emulator* emu);

/* EFLAGの各フラグ設定用関数 */
void set_carry(Emulator* emu, int is_carry);
void set_zero(Emulator* emu, int is_zero);
void set_sign(Emulator* emu, int is_sign);
void set_overflow(Emulator* emu, int is_overflow);
void set_interrupt(Emulator* emu, int is_interrupt);

/* EFLAGの各フラグ取得用関数 */
int32_t is_carry(Emulator* emu);
int32_t is_zero(Emulator* emu);
int32_t is_sign(Emulator* emu);
int32_t is_overflow(Emulator* emu);
int32_t is_interrupt(Emulator* emu);

void update_eflags_sub8(Emulator* emu, uint8_t v1, uint8_t v2, uint16_t result);
void update_eflags_sub(Emulator* emu, uint32_t v1, uint32_t v2,
                       uint64_t result);

void update_eflags_add8(Emulator* emu, uint8_t v1, uint8_t v2, uint16_t result);
// void update_eflags_add16(Emulator* emu,uint16_t v1,uint16_t v2,uint32_t
// result);
void update_eflags_add(Emulator* emu, uint32_t v1, uint32_t v2,
                       uint64_t result);

void update_eflags_inc(Emulator* emu, uint32_t v1);
void update_eflags_dec(Emulator* emu, uint32_t v1);

void update_eflags_or_and(Emulator* emu, uint32_t result);
void update_eflags_or_and8(Emulator* emu, uint8_t result);

void update_eflags_sar8(Emulator* emu, uint8_t v1, uint8_t v2, uint8_t result);
void update_eflags_shr8(Emulator* emu, uint8_t v1, uint8_t v2, uint8_t result);
void update_eflags_sar(Emulator* emu, uint32_t v1, uint8_t v2, uint32_t result);
void update_eflags_shr(Emulator* emu, uint32_t v1, uint8_t v2, uint32_t result);
void update_eflags_imul_2or3(Emulator* emu, uint32_t v1, int32_t v2);

#endif
