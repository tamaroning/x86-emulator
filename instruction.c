//16bit:iw
//32bit:id<-

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "instruction.h"
#include "emulator.h"
#include "emulator_function.h"
#include "io.h"
#include "bios.h"

#include "modrm.h"

instruction_func_t* instructions[256];//index=opecode

static void mov_r8_imm8(Emulator* emu)//
{
    uint8_t reg = get_code8(emu, 0) - 0xB0;
    set_register8(emu, reg, get_code8(emu, 1));
    emu->eip += 2;
}

static void mov_r32_imm32(Emulator* emu)//
{
    uint8_t reg = get_code8(emu, 0) - 0xB8;
    uint32_t value = get_code32(emu, 1);
    set_register32(emu, reg, value);
    emu->eip += 5;
}

static void mov_r8_rm8(Emulator* emu)
{
    emu->eip += 1;
    ModRM modrm;
    parse_modrm(emu, &modrm);
    uint32_t rm8 = get_rm8(emu, &modrm);
    set_r8(emu, &modrm, rm8);
}

static void mov_r32_rm32(Emulator* emu)
{
    emu->eip += 1;
    ModRM modrm;
    parse_modrm(emu, &modrm);
    uint32_t rm32 = get_rm32(emu, &modrm);
    set_r32(emu, &modrm, rm32);
}

static void add_rm32_r32(Emulator* emu)
{
    emu->eip += 1;
    ModRM modrm;
    parse_modrm(emu, &modrm);
    uint32_t r32 = get_r32(emu, &modrm);
    uint32_t rm32 = get_rm32(emu, &modrm);
    set_rm32(emu, &modrm, rm32 + r32);
}


static void or_rm32_r32(Emulator* emu)
{
    emu->eip += 1;
    ModRM modrm;
    parse_modrm(emu, &modrm);
    uint32_t r32 = get_r32(emu, &modrm);
    uint32_t rm32 = get_rm32(emu, &modrm);
    set_rm32(emu, &modrm, rm32 | r32);
}

//new
static void mov_rm8_r8(Emulator* emu)
{
    emu->eip += 1;
    ModRM modrm;
    parse_modrm(emu, &modrm);
    uint32_t r8 = get_r8(emu, &modrm);
    set_rm8(emu, &modrm, r8);
}

static void mov_rm32_r32(Emulator* emu)
{
    emu->eip += 1;
    ModRM modrm;
    parse_modrm(emu, &modrm);
    uint32_t r32 = get_r32(emu, &modrm);
    set_rm32(emu, &modrm, r32);
}

static void inc_r32(Emulator* emu)
{
    uint8_t reg = get_code8(emu, 0) - 0x40;
    set_register32(emu, reg, get_register32(emu, reg) + 1);
    emu->eip += 1;
}

//new
static void dec_r32(Emulator* emu)
{
    uint8_t reg = get_code8(emu, 0) - 0x40;
    uint32_t r32= get_register32(emu, reg);
    set_register32(emu, reg, r32 - 1);

    set_overflow(emu,r32==0);
    set_zero(emu,r32==1);

    emu->eip += 1;
}

static void push_r32(Emulator* emu)
{
    uint8_t reg = get_code8(emu, 0) - 0x50;
    push32(emu, get_register32(emu, reg));
    emu->eip += 1;
}

static void pop_r32(Emulator* emu)
{
    uint8_t reg = get_code8(emu, 0) - 0x58;
    set_register32(emu, reg, pop32(emu));
    emu->eip += 1;
}

static void push_imm32(Emulator* emu)
{
    uint32_t value = get_code32(emu, 1);
    push32(emu, value);
    emu->eip += 5;
}

static void push_imm8(Emulator* emu)
{
    uint8_t value = get_code8(emu, 1);
    push32(emu, value);
    emu->eip += 2;
}

static void add_rm32_imm8(Emulator* emu, ModRM* modrm)
{
    uint32_t rm32 = get_rm32(emu, modrm);
    uint32_t imm8 = (int32_t)get_sign_code8(emu, 0);
    emu->eip += 1;
    set_rm32(emu, modrm, rm32 + imm8);
}


static void xor_rm32_r32(Emulator* emu){
    emu->eip++;
    ModRM modrm;
    parse_modrm(emu,&modrm);
    uint32_t rm32,r32,res;
    rm32=get_rm32(emu,&modrm);
    r32=get_r32(emu,&modrm);

    res = rm32^r32;
    set_rm32(emu,&modrm,res);

    set_zero(emu,res==0);
    set_overflow(emu,0);
    set_carry(emu,0);

}

static void cmp_rm8_imm8(Emulator* emu,ModRM* modrm){
    uint8_t rm8=get_rm8(emu,modrm);
    uint8_t imm8=get_code8(emu,0);

    uint16_t result = (uint16_t)rm8 - (uint16_t)imm8;
    update_eflags_sub(emu, (uint32_t)rm8, (uint32_t)imm8, (uint64_t)result);

    emu->eip++;
}

static void cmp_rm32_imm8(Emulator* emu, ModRM* modrm)
{
    uint32_t rm32 = get_rm32(emu, modrm);
    uint32_t imm8 = (int32_t)get_sign_code8(emu, 0);
    emu->eip += 1;
    uint64_t result = (uint64_t)rm32 - (uint64_t)imm8;
    update_eflags_sub(emu, rm32, imm8, result);
}

static void sub_rm32_imm8(Emulator* emu, ModRM* modrm)
{
    uint32_t rm32 = get_rm32(emu, modrm);
    uint32_t imm8 = (int32_t)get_sign_code8(emu, 0);
    emu->eip += 1;
    uint64_t result = (uint64_t)rm32 - (uint64_t)imm8;
    set_rm32(emu, modrm, result);
    update_eflags_sub(emu, rm32, imm8, result);
}

static void sub_rm32_imm32(Emulator* emu, ModRM* modrm)
{
    uint32_t rm32 = get_rm32(emu, modrm);
    uint32_t imm32 = (uint32_t)get_sign_code32(emu, 0);
    emu->eip += 4;
    uint64_t result = (uint64_t)rm32 - (uint64_t)imm32;
    set_rm32(emu, modrm, result);
    update_eflags_sub(emu, rm32, imm32, result);
}

//new
static void and_rm8_imm8(Emulator* emu,ModRM* modrm){
    uint8_t imm8=get_code8(emu,0);
    uint8_t rm8=get_rm8(emu,modrm);
    rm8 &= imm8;
    set_rm8(emu,modrm,rm8);
    
    set_overflow(emu,0);
    set_carry(emu,0);
    set_zero(emu,rm8==0);
    emu->eip++;
    //sf
}

static void code_83(Emulator* emu)
{
    emu->eip += 1;
    ModRM modrm;
    parse_modrm(emu, &modrm);

    switch (modrm.nnn) {
    case 0:
        add_rm32_imm8(emu, &modrm);
        break;
    case 4:
        and_rm8_imm8(emu,&modrm);
        break;
    case 5:
        sub_rm32_imm8(emu, &modrm);
        break;
    case 7:
        cmp_rm32_imm8(emu, &modrm);
        break;
    default:
        printf("not implemented: 83 /%d\n", modrm.nnn);
        exit(1);
    }
}

static void test_rm32_r32(Emulator* emu){
    emu->eip++;
    ModRM modrm;
    parse_modrm(emu,&modrm);
    uint32_t rm32,r32;
    rm32=get_rm32(emu,&modrm);
    r32=get_r32(emu,&modrm);
    rm32 = rm32 & r32;
    set_zero(emu,rm32==0);
    set_carry(emu,0);
    set_overflow(emu,0);

}


static void mov_rm8_imm8(Emulator* emu)
{   
    emu->eip += 1;
    ModRM modrm;
    parse_modrm(emu, &modrm);
    uint8_t value = get_code8(emu, 0);
    set_rm8(emu, &modrm, value);
    emu->eip += 1;
}

static void mov_rm32_imm32(Emulator* emu)
{
    emu->eip += 1;
    ModRM modrm;
    parse_modrm(emu, &modrm);
    uint32_t value = get_code32(emu, 0);
    emu->eip += 4;
    set_rm32(emu, &modrm, value);
}


static void lgdt(Emulator* emu, ModRM* modrm){
    uint32_t ret;
    ret = get_rm32(emu, modrm);
    printf("%0d",ret);
}

static void lidt(Emulator* emu, ModRM* modrm){
    uint32_t ret;
    ret = get_rm32(emu, modrm);
    printf("%0d",ret);
}

static void code_0F(Emulator* emu){
    emu->eip+=2;
    ModRM modrm;
    parse_modrm(emu,&modrm);
    switch(modrm.nnn){
        case 2:
            //lgdt m16& 32
            lgdt(emu, &modrm);
            break;
        case 3:
            //lidt m16& 32
            lidt(emu, &modrm);
            break;
        default:
            puts("error not implimented");
    }    
}

//mov al,[dx]
static void in_al_dx(Emulator* emu)
{
    uint16_t address = get_register32(emu, EDX) & 0xffff;
    uint8_t value = io_in8(address);
    set_register8(emu, AL, value);
    emu->eip += 1;
}

//mov [dx],al
static void out_dx_al(Emulator* emu)
{
    uint16_t address = get_register32(emu, EDX) & 0xffff;
    uint8_t value = get_register8(emu, AL);
    io_out8(address, value);
    emu->eip += 1;
}

static void inc_rm32(Emulator* emu, ModRM* modrm)
{
    uint32_t value = get_rm32(emu, modrm);
    set_rm32(emu, modrm, value + 1);
}

static void code_ff(Emulator* emu)
{
    emu->eip += 1;
    ModRM modrm;
    parse_modrm(emu, &modrm);

    switch (modrm.nnn) {
    case 0:
        inc_rm32(emu, &modrm);
        break;
    default:
        printf("not implemented: FF /%d\n", modrm.nnn);
        exit(1);
    }
}

static void call_rel32(Emulator* emu)
{
    int32_t diff = get_sign_code32(emu, 1);
    push32(emu, emu->eip + 5);
    emu->eip += (diff + 5);
}

static void ret(Emulator* emu)
{
    emu->eip = pop32(emu);
}

static void leave(Emulator* emu)
{
    uint32_t ebp = get_register32(emu, EBP);
    set_register32(emu, ESP, ebp);//mov esp,ebp
    set_register32(emu, EBP, pop32(emu));//pop ebp

    emu->eip += 1;
}

static void short_jump(Emulator* emu)
{
    int8_t diff = get_sign_code8(emu, 1);
    emu->eip += (diff + 2);
}

static void near_jump(Emulator* emu)
{
    int32_t diff = get_sign_code32(emu, 1);
    emu->eip += (diff + 5);
}

static void cmp_al_imm8(Emulator* emu)
{
    uint8_t value = get_code8(emu, 1);
    uint8_t al = get_register8(emu, AL);
    uint64_t result = (uint64_t)al - (uint64_t)value;
    update_eflags_sub(emu, al, value, result);
    emu->eip += 2;
}

static void cmp_eax_imm32(Emulator* emu)
{
    uint32_t value = get_code32(emu, 1);
    uint32_t eax = get_register32(emu, EAX);
    uint64_t result = (uint64_t)eax - (uint64_t)value;
    update_eflags_sub(emu, eax, value, result);
    emu->eip += 5;
}

static void cmp_r32_rm32(Emulator* emu)
{
    emu->eip += 1;
    ModRM modrm;
    parse_modrm(emu, &modrm);
    uint32_t r32 = get_r32(emu, &modrm);
    uint32_t rm32 = get_rm32(emu, &modrm);
    uint64_t result = (uint64_t)r32 - (uint64_t)rm32;
    update_eflags_sub(emu, r32, rm32, result);
}

static void cmp_rm32_r32(Emulator* emu)
{
    emu->eip += 1;
    ModRM modrm;
    parse_modrm(emu, &modrm);
    uint32_t r32 = get_r32(emu, &modrm);
    uint32_t rm32 = get_rm32(emu, &modrm);
    uint64_t result = (uint64_t)rm32 - (uint64_t)r32;
    update_eflags_sub(emu, rm32, r32, result);
}

//new
static void cmp_rm32_imm32(Emulator* emu,ModRM* modrm){
    uint32_t imm32=get_code32(emu,0);
    uint32_t rm32=get_rm32(emu,modrm);
    uint64_t result=(uint64_t)rm32-(uint64_t)imm32;
    update_eflags_sub(emu,rm32,imm32,result);
    emu->eip+=4;
}

#define DEFINE_JX(flag, is_flag) \
static void j ## flag(Emulator* emu) \
{ \
    int diff = is_flag(emu) ? get_sign_code8(emu, 1) : 0; \
    emu->eip += (diff + 2); \
} \
static void jn ## flag(Emulator* emu) \
{ \
    int diff = is_flag(emu) ? 0 : get_sign_code8(emu, 1); \
    emu->eip += (diff + 2); \
}

DEFINE_JX(c, is_carry)
DEFINE_JX(z, is_zero)
DEFINE_JX(s, is_sign)
DEFINE_JX(o, is_overflow)


#undef DEFINE_JX

static void jl(Emulator* emu)//jump if less
{
    //signフラグが1ならjump
    int diff = (is_sign(emu) != is_overflow(emu)) ? get_sign_code8(emu, 1) : 0;
    emu->eip += (diff + 2);
}

static void jge(Emulator* emu)//jump if less
{
    //sf==of
    int diff = (is_sign(emu) == is_overflow(emu)) ? get_sign_code8(emu, 1) : 0;
    emu->eip += (diff + 2);
}

static void jle(Emulator* emu)//jump less or equal
{
    int diff = (is_zero(emu) || (is_sign(emu) != is_overflow(emu))) ? get_sign_code8(emu, 1) : 0;
    emu->eip += (diff + 2);
}

static void jg(Emulator* emu)//jump less or equal
{
    int diff = (!is_zero(emu) && is_sign(emu)==is_overflow(emu)) ? get_sign_code8(emu, 1) : 0;
    emu->eip += (diff + 2);
}

//new
static void jbe(Emulator* emu)//jump if less
{
    //CF==1 or ZF==1
    int diff = (is_carry(emu)||is_zero(emu)) ? get_sign_code8(emu, 1) : 0;
    emu->eip += (diff + 2);
}

static void ja(Emulator* emu)//jump if less
{
    //CF==1 or ZF==1
    int diff = (!is_carry(emu) && !is_zero(emu)) ? get_sign_code8(emu, 1) : 0;
    emu->eip += (diff + 2);
}



//int
static void swi(Emulator* emu)
{
    uint8_t int_index = get_code8(emu, 1);
    emu->eip += 2;

    switch (int_index){
        case 0x10:
            bios_video(emu);
        break;
        default:
            printf("unknown interrupt: 0x%02x\n", int_index);
    }
}

//new
//lea
//第2オペランド（読み込み元）の実効アドレスを計算し、第1オペランド（格納先）に格納
static void lea(Emulator* emu){
    //printf("ebp:%08x\n",emu->registers[EBP]);
    emu->eip += 1;
    ModRM modrm;
    parse_modrm(emu, &modrm);
    uint32_t addr=calc_memory_address(emu, &modrm);
    set_r32(emu,&modrm,addr);
}

static void pushfd(Emulator *emu){
    if(opsiz==0){
        push32(emu,emu->eflags);
    }else{
        int32_t address = get_register32(emu, ESP) - 2;//esp-=4
        set_register32(emu, ESP, address);
        set_memory16(emu, address, emu->eflags & 0xffff);
    }
    emu->eip++;
}

static void popfd(Emulator* emu){
    if(opsiz==0){
        emu->eflags=pop32(emu);
    }else{
        uint32_t address = get_register32(emu, ESP);
        emu->eflags |= get_memory16(emu, address);
        set_register32(emu, ESP, address + 2);//esp+=4
    }
    emu->eip++;
}



static void code_80(Emulator* emu){
    emu->eip++;
    ModRM modrm;
    parse_modrm(emu,&modrm);
    switch(modrm.nnn){
        case 7:
            cmp_rm8_imm8(emu,&modrm);
            break;
        default:
            puts("error code_80");
            break;
    }
}

//new
static void code_81(Emulator* emu)
{
    emu->eip += 1;
    ModRM modrm;
    parse_modrm(emu, &modrm);
    switch(modrm.nnn){
        case 5:
            sub_rm32_imm32(emu,&modrm);
            break;
        case 7:
            cmp_rm32_imm32(emu,&modrm);
            break;
        default:
            puts("error code:81");
            exit(0);
            break;
    }
}

/*
static void cmp_rm16_r16(Emulator* emu){
    puts("cmpえらー");
    exit(0);
    emu->eip++;
    ModRM modrm;
    parse_modrm(emu,&modrm);
    //int32_t rm16 = (int32_t)get_rm16(emu,&modrm);
    //get_r
}
*/


static void sar_rm8_imm8(Emulator* emu,ModRM* modrm){
    uint32_t rm8,imm8,res8;
    int64_t sgn;
    
    imm8=get_code8(emu,0);
    rm8=(uint32_t)get_rm8(emu,modrm);
    sgn=(int64_t)rm8;
    sgn=sgn>>imm8;
    res8=(uint32_t)sgn;
    set_rm8(emu,modrm,(uint8_t)res8);
    set_carry(emu, rm8 & (1<<(imm8-1)) );
    set_zero(emu,res8==0);
    if(imm8==1)set_overflow(emu,0);

    emu->eip++;
}

static void sal_rm8_imm8(Emulator* emu,ModRM* modrm){
    uint32_t rm8,imm8,res8;
    imm8=get_code8(emu,0);
    rm8=(uint32_t)get_rm8(emu,modrm);
    res8=rm8<<imm8;

    set_rm8(emu,modrm,(uint8_t)res8%256);
    set_zero(emu,rm8==0);
    set_carry(emu, rm8 & (1<<(8-imm8)) );
    if(imm8==1){
        set_overflow(emu, (rm8 & 0xc0)==0 || (rm8 & 0xc0)==3 );
    }
}


//（セグメント：オフセット）のバイトをALに転送します
static void mov_al_moffs8(Emulator* emu){
    uint32_t offs=get_code32(emu,1);
    set_register32(emu,AL,offs);
    emu->eip+=5;
}

static void mov_moffs32_eax(Emulator* emu){
    uint32_t addr=get_code32(emu,1);
    set_memory32(emu,addr,get_register32(emu,EAX));

    emu->eip+=5;
}

//new
static void code_C0(Emulator* emu){
    emu->eip++;
    ModRM modrm;
    parse_modrm(emu,&modrm);

    switch(modrm.nnn){
        case 4://rm8をimm8だけ左シフト
            sal_rm8_imm8(emu,&modrm);
            break;
        case 5:
        case 7://rm8をimm8だけ右シフト
            sar_rm8_imm8(emu,&modrm);
            break;
        default:
            puts("error code:C0");
            break;
    }
}

static void shr_rm32_imm8(Emulator* emu,ModRM* modrm){
    uint32_t rm32,res32;
    uint8_t imm8;
    
    imm8=get_code8(emu,0);
    rm32=get_rm32(emu,modrm);
    
    res32=rm32>>imm8;

    set_rm32(emu,modrm,res32);
    set_carry(emu, rm32 & (1<<(imm8-1)) );
    set_zero(emu,res32==0);
    if(imm8==1)set_overflow(emu,0);

    emu->eip++;
}

static void sar_rm32_imm8(Emulator* emu,ModRM* modrm){
    uint32_t rm32,res32;
    uint8_t imm8;
    int64_t sgn;
    
    imm8=get_code8(emu,0);
    rm32=get_rm32(emu,modrm);
    sgn=(int64_t)rm32;
    sgn=rm32>>imm8;
    res32=(uint32_t)sgn;
    set_rm8(emu,modrm,(uint8_t)res32);
    set_carry(emu, rm32 & (1<<(imm8-1)) );
    set_zero(emu,res32==0);
    if(imm8==1)set_overflow(emu,0);

    emu->eip++;
}

static void code_C1(Emulator* emu){
    //uint64_t rm8,imm8,res8;
    emu->eip++;
    ModRM modrm;
    parse_modrm(emu,&modrm);
    switch(modrm.nnn){
        //case 4://shl rm32 imm8
            //break;

            //帰宅したらシフト系は関数にまとめますーーーー
        
        //r/m32を2でimm8回符号なし除算します
        case 5://shr rm32 imm8
            shr_rm32_imm8(emu,&modrm);
            
            break;
        case 7://sar rm32 imm8
            sar_rm32_imm8(emu,&modrm);
            break;
        default:
            printf("error code:C1/%d\n",modrm.nnn);
            break;
    }

}

static void code_66(Emulator* emu){
    opsiz=2;
    emu->eip++;
}

static void cli(Emulator* emu){
    set_interrupt(emu,0);
    emu->eip++; 
}

static void sti(Emulator* emu){
    set_interrupt(emu,1);
    emu->eip++; 
}

void init_instructions(void){
    int i;
    memset(instructions, 0, sizeof(instructions));
    instructions[0x01] = add_rm32_r32;
    instructions[0x09] = or_rm32_r32;
    instructions[0x0F] = code_0F;

    instructions[0x31] = xor_rm32_r32;

    instructions[0x39] = cmp_rm32_r32;
    instructions[0x3B] = cmp_r32_rm32;
    instructions[0x3C] = cmp_al_imm8;
    instructions[0x3D] = cmp_eax_imm32;
    instructions[0x3D] = cmp_rm32_r32;

    for (i = 0; i < 8; i++) {
        instructions[0x40 + i] = inc_r32;
    }

    for (i = 0; i < 8; i++) {
        instructions[0x48 + i] = dec_r32;
    }

    for (i = 0; i < 8; i++) {
        instructions[0x50 + i] = push_r32;
    }

    for (i = 0; i < 8; i++) {
        instructions[0x58 + i] = pop_r32;
    }
    instructions[0x66] = code_66;
    instructions[0x68] = push_imm32;
    instructions[0x6A] = push_imm8;


    instructions[0x70] = jo;//overflow
    instructions[0x71] = jno;//not overflow
    instructions[0x72] = jc;//carry
    instructions[0x73] = jnc;//not carry
    instructions[0x74] = jz;//zero
    instructions[0x75] = jnz;//not zero
    instructions[0x76] = jbe;
    instructions[0x77] = ja;
    instructions[0x78] = js;//sign
    instructions[0x79] = jns;//not sign
    instructions[0x7C] = jl;//less
    instructions[0x7D] = jge;//
    instructions[0x7E] = jle;//less or equal
    instructions[0x7F] = jg;

    instructions[0x80] = code_80;
    instructions[0x81] = code_81;

    instructions[0x83] = code_83;
    instructions[0x85] = test_rm32_r32;
    instructions[0x88] = mov_rm8_r8;
    instructions[0x89] = mov_rm32_r32;
    instructions[0x8A] = mov_r8_rm8;
    instructions[0x8B] = mov_r32_rm32;

    instructions[0x8D] = lea;
    instructions[0x9C] = pushfd;
    instructions[0x9D] = popfd;
    

    instructions[0xA0] = mov_al_moffs8;
    instructions[0xA3] = mov_moffs32_eax;

    //0xb0~0xb7
    for (i = 0; i < 8; i++) {
        instructions[0xB0 + i] = mov_r8_imm8;
    }
    //0xb8~0xbf
    for (i = 0; i < 8; i++) {
        instructions[0xB8 + i] = mov_r32_imm32;
    }
    instructions[0xC0] = code_C0;
    instructions[0xC1] = code_C1;
    instructions[0xC3] = ret;
    instructions[0xC6] = mov_rm8_imm8;
    instructions[0xC7] = mov_rm32_imm32;
    instructions[0xC9] = leave;

    instructions[0xCD] = swi;

    instructions[0xE8] = call_rel32;
    instructions[0xE9] = near_jump;
    instructions[0xEB] = short_jump;
    instructions[0xEC] = in_al_dx;
    instructions[0xEE] = out_dx_al;


    instructions[0xFA] = cli;
    instructions[0xFB] = sti;

    instructions[0xFF] = code_ff;
}

