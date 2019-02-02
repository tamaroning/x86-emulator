#include "emulator_function.h"
#include<stdio.h>

uint32_t get_code8(Emulator* emu, int index)
{
    return emu->memory[emu->eip + index + emu->segBase[1]];
}

int32_t get_sign_code8(Emulator* emu, int index)
{
    return (int8_t)emu->memory[emu->eip + index + emu->segBase[1]];
}

uint32_t get_code32(Emulator* emu, int index)
{
    int i;
    uint32_t ret = 0;

    /* リトルエンディアンでメモリの値を取得する */
    for (i = 0; i < 4; i++) {
        ret |= get_code8(emu, index + i) << (i * 8);
    }

    return ret;
}

int32_t get_sign_code32(Emulator* emu, int index)
{
    return (int32_t)get_code32(emu, index);
}

uint8_t get_register8(Emulator* emu, int index)
{
    if (index < 4) {
        return emu->registers[index] & 0xff;
    } else {
        return (emu->registers[index - 4] >> 8) & 0xff;
    }
}

uint16_t get_register16(Emulator* emu,int index)
{
    return emu->registers[index] & 0xffff;
}

uint32_t get_register32(Emulator* emu, int index)
{
    return emu->registers[index];
}

void set_register8(Emulator* emu, int index, uint8_t value)
{
    if (index < 4) {
        uint32_t r = emu->registers[index] & 0xffffff00;
        emu->registers[index] = r | (uint32_t)value;
    } else {
        uint32_t r = emu->registers[index - 4] & 0xffff00ff;
        emu->registers[index - 4] = r | ((int32_t)value << 8);
    }
}

void set_register16(Emulator* emu, int index, uint16_t value){
    emu->registers[index] = (emu->registers[index] & 0xffff0000) + value;
}

void set_register32(Emulator* emu, int index, uint32_t value)
{
    emu->registers[index] = value;
}

void set_memory8(Emulator* emu, uint32_t address, uint32_t value)
{
    //if(0x30fb9c+2==address && value==0xaf)printf("yeah"),exit(0);
    emu->memory[address] = value & 0xFF;
}

void set_memory32(Emulator* emu, uint32_t address, uint32_t value)
{
    int i;
    /* リトルエンディアンでメモリの値を設定する */
    for (i = 0; i < 4; i++) {
        set_memory8(emu, address + i, value >> (i * 8));
    }
}

void set_memory16(Emulator* emu, uint32_t address, uint32_t value)
{
    int i;
    /* リトルエンディアンでメモリの値を設定する */
    for (i = 0; i < 2; i++) {
        set_memory8(emu, address + i, value >> (i * 8));
    }
}


uint32_t get_memory8(Emulator* emu, uint32_t address)
{
    return emu->memory[address];
}

uint32_t get_memory16(Emulator* emu, uint32_t address)
{
    int i;
    uint32_t ret = 0;
    /* リトルエンディアンでメモリの値を取得する */
    for (i = 0; i < 2; i++) {
        ret |= get_memory8(emu, address + i) << (8 * i);
    }
    return ret;
}

uint32_t get_memory32(Emulator* emu, uint32_t address)
{
    int i;
    uint32_t ret = 0;
    /* リトルエンディアンでメモリの値を取得する */
    for (i = 0; i < 4; i++) {
        ret |= get_memory8(emu, address + i) << (8 * i);
    }
    return ret;
}


void push32(Emulator* emu, uint32_t value){
    int32_t address = get_register32(emu, ESP) - 4;//esp-=4
    set_register32(emu, ESP, address);
    set_memory32(emu, address, value);
    
    printf("-------------------------------push %x at %x\n",value,address);
}

uint32_t pop32(Emulator* emu){
    uint32_t address = get_register32(emu, ESP);
    uint32_t ret = get_memory32(emu, address);
    set_register32(emu, ESP, address + 4);//esp+=4
    printf("--------------------------------pop %x from %x\n",ret,address);
    return ret;
}



void set_carry(Emulator* emu, int is_carry)
{
    if (is_carry) {
        emu->eflags |= CARRY_FLAG;
    } else {
        emu->eflags &= ~CARRY_FLAG;
    }
}

void set_zero(Emulator* emu, int is_zero)
{
    if (is_zero) {
        emu->eflags |= ZERO_FLAG;
    } else {
        emu->eflags &= ~ZERO_FLAG;
    }
}

void set_sign(Emulator* emu, int is_sign)
{
    if (is_sign) {
        emu->eflags |= SIGN_FLAG;
    } else {
        emu->eflags &= ~SIGN_FLAG;
    }
}

void set_overflow(Emulator* emu, int is_overflow)
{
    if (is_overflow) {
        emu->eflags |= OVERFLOW_FLAG;
    } else {
        emu->eflags &= ~OVERFLOW_FLAG;
    }
}

void set_parity(Emulator* emu, int is_parity)
{
    if (is_parity) {
        emu->eflags |= PARITY_FLAG;
    } else {
        emu->eflags &= ~PARITY_FLAG;
    }
}


void set_interrupt(Emulator* emu, int is_interrupt)
{
    if (is_interrupt) {
        emu->eflags |= INTERRUPT_FLAG;
    } else {
        emu->eflags &= ~INTERRUPT_FLAG;
    }
}

int is_carry(Emulator* emu)
{
    return (emu->eflags & CARRY_FLAG) != 0;
}

int is_zero(Emulator* emu)
{
    return (emu->eflags & ZERO_FLAG) != 0;
}

int is_sign(Emulator* emu)
{
    return (emu->eflags & SIGN_FLAG) != 0;
}

int is_overflow(Emulator* emu)
{
    return (emu->eflags & OVERFLOW_FLAG) != 0;
}

int is_parity(Emulator* emu)
{
    return (emu->eflags & PARITY_FLAG) != 0;
}

int is_interrupt(Emulator* emu)
{
    return (emu->eflags & INTERRUPT_FLAG) != 0;
}

void update_eflags_sub8(Emulator* emu, uint8_t v1, uint8_t v2, uint16_t result){
    /* 各値の符号を取得 */
    int sign1 = v1 >> 7;
    int sign2 = v2 >> 7;
    int signr = (result >> 7) & 1;

    set_carry(emu, result >> 8);
    set_zero(emu, result == 0);
    set_sign(emu, signr);
    set_overflow(emu, sign1 != sign2 && sign1 != signr);
}

void update_eflags_sub(Emulator* emu, uint32_t v1, uint32_t v2, uint64_t result)
{
    /* 各値の符号を取得 */
    int sign1 = v1 >> 31;
    int sign2 = v2 >> 31;
    int signr = (result >> 31) & 1;

    /* 演算結果にcarryがあればCarryフラグ設定 */
    set_carry(emu, result >> 32);
    /* 演算結果が0ならばZeroフラグ設定 */
    set_zero(emu, result == 0);
    /* 演算結果に符合があればSignフラグ設定 */
    set_sign(emu, signr);
    /* 演算結果がオーバーフローしていたらOverflowフラグ設定 */
    set_overflow(emu, sign1 != sign2 && sign1 != signr);
}

void update_eflags_add8(Emulator* emu,uint8_t v1,uint8_t v2,uint16_t result){
    //int sign1 = v1 >> 31;
    //int sign2 = v2 >> 31;
    int signr = (result >> 7) & 1;

    set_carry(emu, result >> 8);
    set_zero(emu, result == 0);
    set_sign(emu, signr);
    set_overflow(emu, result>>8);
}

/*
void update_eflags_add16(Emulator* emu,uint16_t v1,uint16_t v2,uint32_t result){
    //int sign1 = v1 >> 31;
    //int sign2 = v2 >> 31;
    int signr = (result >> 15) & 1;

    set_carry(emu, result >> 16);
    set_zero(emu, result == 0);
    set_sign(emu, signr);
    set_overflow(emu, result>>16);
}*/

void update_eflags_add(Emulator* emu,uint32_t v1,uint32_t v2,uint64_t result){
    //int sign1 = v1 >> 31;
    //int sign2 = v2 >> 31;
    int signr = (result >> 31) & 1;

    set_carry(emu, result >> 32);
    set_zero(emu, result == 0);
    set_sign(emu, signr);
    set_overflow(emu, result>>32);
}

//inc,dec : carry変えない
void update_eflags_inc(Emulator* emu,uint32_t v1){
    uint64_t result=v1+1;
    //int sign1 = v1 >> 31;
    int signr = (result >> 31) & 1;
    set_zero(emu, result == 0);
    set_sign(emu, signr);
    set_overflow(emu, result>>32);
}

void update_eflags_dec(Emulator* emu,uint32_t v1){
    uint64_t result=v1-1;
    //int sign1 = v1 >> 31;
    int signr = (result >> 31) & 1;
    set_zero(emu, result == 0);
    set_sign(emu, signr);
    set_overflow(emu, result>>32);
}

//or and xor 
void update_eflags_or_and(Emulator* emu,uint32_t result){
    int signr = (result >> 31)&1;

    set_carry(emu, 0);
    set_zero(emu, result == 0);
    set_sign(emu, signr);
    set_overflow(emu, 0);
}

void update_eflags_or_and8(Emulator* emu,uint8_t result){
    int signr = (result >> 7)&1;

    set_carry(emu, 0);
    set_zero(emu, result == 0);
    set_sign(emu, signr);
    set_overflow(emu, 0);
}

void update_eflags_sar8(Emulator* emu,uint8_t v1,uint8_t v2,uint8_t result){
    //int sign1 = v1 >> 7;
    //int sign2 = v2 >> 7;
    int signr = (result >> 7)&1;

    set_carry(emu, (v1>>(v2-1))&1);
    set_zero(emu, result == 0);
    set_sign(emu, signr);
    //set_overflow(emu, result>>8);
    if(v2==1)set_overflow(emu,0);
}

void update_eflags_shr8(Emulator* emu,uint8_t v1,uint8_t v2,uint8_t result){
    int sign1 = (v1 >> 7)&1;
    //int sign2 = v2 >> 7;
    int signr = (result >> 7)&1;

    set_carry(emu,  (v1>>(v2-1))&1);
    set_zero(emu, result == 0);
    set_sign(emu, signr);
    set_overflow(emu, sign1);//もとoperandの最上位
}

void update_eflags_sar(Emulator* emu,uint32_t v1,uint8_t v2,uint32_t result){
    //int sign1 = v1 >> 31;
    //int sign2 = v2 >> 31;
    int signr = (result >> 31)&1;

    set_carry(emu, (v1>>(v2-1))&1);
    set_zero(emu, result == 0);
    set_sign(emu, signr);
    //set_overflow(emu, result>>32);
    if(v2==1)set_overflow(emu,0);
    
}

void update_eflags_shr(Emulator* emu,uint32_t v1,uint8_t v2,uint32_t result){
    int sign1 = (v1 >> 31)&1;
    //int sign2 = v2 >> 31;
    int signr = (result >> 31)&1;

    set_carry(emu,  (v1>>(v2-1))&1);
    set_zero(emu, result == 0);
    set_sign(emu, signr);
    set_overflow(emu, sign1);//もとoperandの最上位
}

//last
void update_eflags_imul_2or3(Emulator* emu,uint32_t v1,int32_t v2){
    int32_t dest;
    int64_t temp;

    dest=v1*v2;
    temp=v1*v2;

    if(temp==dest){
        set_carry(emu,0);
        set_overflow(emu,0);
    }else{
        set_carry(emu,1);
        set_overflow(emu,1);
    }
}