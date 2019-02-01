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

instruction_func_t* instructions[256];

void dump(Emulator* emu){

    printf("\n[%02X %02X %02X %02X %02X %02X %02X %02X]\n", get_code8(emu, -8), get_code8(emu, -7), get_code8(emu, -6), get_code8(emu, -5), get_code8(emu, -4), get_code8(emu, -3), get_code8(emu, -2), get_code8(emu, -1));
    printf("[%02X %02X %02X %02X %02X %02X %02X %02X]\n\n", get_code8(emu, 0), get_code8(emu, 1), get_code8(emu, 2), get_code8(emu, 3), get_code8(emu, 4), get_code8(emu, 5), get_code8(emu, 6), get_code8(emu, 7));
    
    char* registers_name[] = {"EAX", "ECX", "EDX", "EBX", "ESP", "EBP", "ESI", "EDI"};
    
    puts("---registers---");
    int i;
    for (i = 0; i < REGISTERS_COUNT; i++) {
        printf("%s = %08x\n", registers_name[i], get_register32(emu, i));
    }
    printf("EIP = %08x\n", emu->eip);
}

void error(Emulator* emu){
    printf("error see you~\n");
    dump(emu);
    exit(0);
    return;
}


static void mov_r8_imm8(Emulator* emu){
    if(opsiz)error(emu);
    uint8_t reg = get_code8(emu, 0) - 0xB0;
    set_register8(emu, reg, get_code8(emu, 1));
    emu->eip += 2;
}

static void mov_r32_imm32(Emulator* emu){
    if(opsiz)error(emu);
    uint8_t reg = get_code8(emu, 0) - 0xB8;
    uint32_t value = get_code32(emu, 1);
    set_register32(emu, reg, value);
    emu->eip += 5;
}

static void mov_r8_rm8(Emulator* emu){
    if(opsiz)error(emu);
    emu->eip += 1;
    ModRM modrm;
    parse_modrm(emu, &modrm);
    uint32_t rm8 = get_rm8(emu, &modrm);
    set_r8(emu, &modrm, rm8);
}

static void mov_r32_rm32(Emulator* emu){
    if(!opsiz){
        emu->eip += 1;
        ModRM modrm;
        parse_modrm(emu, &modrm);
        uint32_t rm32 = get_rm32(emu, &modrm);
        set_r32(emu, &modrm, rm32);
    }else{
        emu->eip += 1;
        ModRM modrm;
        parse_modrm(emu, &modrm);
        uint16_t rm16 = get_rm16(emu, &modrm);
        set_r16(emu, &modrm, rm16);
    }
}

//last
static void add_rm8_r8(Emulator* emu){
    if(opsiz)error(emu);
    emu->eip += 1;
    ModRM modrm;
    parse_modrm(emu, &modrm);
    uint8_t rm8 = get_rm8(emu, &modrm);
    uint8_t r8 = get_r8(emu, &modrm);
    set_rm8(emu, &modrm, rm8+r8);

    update_eflags_add8(emu,rm8,r8,rm8+r8);
}

//last
static void add_eax_imm32(Emulator* emu){
    if(opsiz)error(emu);
    emu->eip += 1;
    uint32_t imm32 = get_code32(emu,0);
    uint32_t eax=get_register32(emu,EAX);

    set_register32(emu,EAX,eax+imm32);

    update_eflags_add8(emu,eax,imm32,eax+imm32);
    emu->eip+=4;
}


static void add_rm32_r32(Emulator* emu){
    if(opsiz)error(emu);
    emu->eip += 1;
    ModRM modrm;
    parse_modrm(emu, &modrm);
    uint32_t r32 = get_r32(emu, &modrm);
    uint32_t rm32 = get_rm32(emu, &modrm);
    set_rm32(emu, &modrm, rm32 + r32);

    update_eflags_add(emu,rm32,r32,rm32+r32);
}

static void add_r32_rm32(Emulator* emu){
    if(opsiz)error(emu);
    emu->eip += 1;
    ModRM modrm;
    parse_modrm(emu, &modrm);
    uint32_t r32 = get_r32(emu, &modrm);
    uint32_t rm32 = get_rm32(emu, &modrm);
    set_r32(emu, &modrm, r32 + rm32);

    update_eflags_add(emu,r32,rm32,r32+rm32);
}

//last
static void or_rm8_r8(Emulator* emu){
    if(opsiz)error(emu);
    emu->eip++;
    ModRM modrm;
    parse_modrm(emu,&modrm);
    uint8_t rm8,r8,res;
    rm8=get_rm8(emu,&modrm);
    r8=get_r8(emu,&modrm);
    res = rm8 | r8;
    set_rm8(emu,&modrm,res);

    update_eflags_or_and8(emu,res);
}

static void or_rm32_r32(Emulator* emu){
    if(opsiz)error(emu);
    emu->eip += 1;
    ModRM modrm;
    parse_modrm(emu, &modrm);
    uint32_t r32 = get_r32(emu, &modrm);
    uint32_t rm32 = get_rm32(emu, &modrm);
    uint32_t res = r32|rm32;
    set_rm32(emu, &modrm,res);

    update_eflags_or_and(emu,res);
}

static void mov_rm8_r8(Emulator* emu){
    emu->eip += 1;
    ModRM modrm;
    parse_modrm(emu, &modrm);

    uint32_t r8 = get_r8(emu, &modrm);
    
    set_rm8(emu, &modrm, r8);
}

static void mov_rm32_r32(Emulator* emu){
    if(!opsiz){
        emu->eip += 1;
        ModRM modrm;
        parse_modrm(emu, &modrm);
        uint32_t r32 = get_r32(emu, &modrm);
        set_rm32(emu, &modrm, r32);
    }else{
        emu->eip += 1;
        ModRM modrm;
        parse_modrm(emu, &modrm);
        uint16_t r16 = get_r16(emu, &modrm);
        set_rm16(emu, &modrm, r16);
    }
    
}

static void inc_r32(Emulator* emu){
    if(opsiz)error(emu);
    uint8_t reg=get_code8(emu, 0) - 0x40;
    uint32_t r32 = get_register32(emu,reg);
    set_register32(emu, reg, r32 + 1);
    update_eflags_inc(emu,r32);
    emu->eip += 1;
}

static void dec_r32(Emulator* emu){
    if(opsiz)error(emu);
    uint8_t reg = get_code8(emu, 0) - 0x48;
    uint32_t r32= get_register32(emu, reg);
    set_register32(emu, reg, r32 - 1);
    update_eflags_dec(emu,r32);
    emu->eip += 1;
}

static void push_r32(Emulator* emu){
    if(opsiz)error(emu);
    uint8_t reg = get_code8(emu, 0) - 0x50;
    push32(emu, get_register32(emu, reg));
    emu->eip += 1;
}

static void pop_r32(Emulator* emu){
    if(opsiz)error(emu);
    uint8_t reg = get_code8(emu, 0) - 0x58;
    set_register32(emu, reg, pop32(emu));
    emu->eip += 1;
}

/*
static void popad(Emulator* emu){
    set_register32(emu, EDI,pop32(emu));
    set_register32(emu, ESI,pop32(emu));
    set_register32(emu, EBP,pop32(emu));
    set_register32(emu, EBX,pop32(emu));
    set_register32(emu, EDX,pop32(emu));
    set_register32(emu, ECX,pop32(emu));
    set_register32(emu, EAX,pop32(emu));
    emu->eip++;
}*/

static void push_imm32(Emulator* emu){
    if(opsiz)error(emu);
    uint32_t value = get_code32(emu, 1);
    push32(emu, value);
    emu->eip += 5;
}

static void push_imm8(Emulator* emu){
    if(opsiz)error(emu);
    uint8_t value = get_code8(emu, 1);
    push32(emu, value);
    emu->eip += 2;
}

static void imul_r32_rm32_imm8(Emulator* emu){
    if(opsiz)error(emu);
    emu->eip++;
    ModRM modrm;
    parse_modrm(emu,&modrm);
    if(opsiz==0){
        uint32_t rm32=get_rm32(emu,&modrm);
        int8_t imm8=get_sign_code8(emu,0);
        emu->eip++;
        
        set_r32(emu,&modrm,rm32*imm8);

        update_eflags_imul_2or3(emu,rm32,(int32_t)imm8);

    }else{
        puts("error imul");
        exit(1);
    }

}

//last
static void imul_r32_rm32(Emulator* emu,ModRM* modrm){
    if(opsiz)error(emu);
    uint32_t r32=get_r32(emu,modrm);
    uint32_t rm32=get_rm32(emu,modrm);
    
    int32_t sign32=(int32_t)rm32;

    set_r32(emu,modrm,r32*rm32);

    update_eflags_imul_2or3(emu,r32,sign32);

    /*
    if(r32*rm32 != (uint64_t)res){
        set_overflow(emu,1);
        set_carry(emu,1);
    }else{
        set_overflow(emu,0);
        set_carry(emu,0);
    }*/

}

static void imul_r32_rm32_imm32(Emulator* emu){
    if(opsiz)error(emu);
    emu->eip++;
    ModRM modrm;
    parse_modrm(emu,&modrm);
    if(opsiz==0){
        uint32_t rm32 = get_rm32(emu,&modrm);
        int32_t imm32 = get_sign_code32(emu,0);
        emu->eip+=4;
        
        set_r32(emu,&modrm,rm32*imm32);

        update_eflags_imul_2or3(emu,rm32,imm32);

    }else{
        puts("error imul");
        exit(1);
    }
}

static void add_rm32_imm8(Emulator* emu, ModRM* modrm){
    if(opsiz)error(emu);
    uint32_t rm32 = get_rm32(emu, modrm);
    uint32_t imm8 = get_code8(emu, 0);
    emu->eip += 1;
    set_rm32(emu, modrm, rm32 + imm8);

    update_eflags_add(emu,rm32,imm8,rm32+imm8);
}

static void add_rm32_imm32(Emulator* emu, ModRM* modrm){
    if(opsiz)error(emu);
    uint32_t rm32 = get_rm32(emu, modrm);
    uint32_t imm32 = get_code32(emu, 0);
    emu->eip += 4;
    set_rm32(emu, modrm, rm32 + imm32);

    update_eflags_add(emu,rm32,imm32,rm32+imm32);
}

static void xor_rm32_r32(Emulator* emu){
    if(opsiz)error(emu);
    emu->eip++;
    ModRM modrm;
    parse_modrm(emu,&modrm);
    uint32_t rm32,r32,res;
    rm32=get_rm32(emu,&modrm);
    r32=get_r32(emu,&modrm);

    res = rm32^r32;
    set_rm32(emu,&modrm,res);

    update_eflags_or_and(emu,res);

}

static void cmp_rm8_imm8(Emulator* emu,ModRM* modrm){
    uint8_t rm8=get_rm8(emu,modrm);
    uint8_t imm8=get_code8(emu,0);

    uint16_t res = (uint16_t)rm8 - (uint16_t)imm8;

    update_eflags_sub8(emu,rm8,imm8,res);

    emu->eip++;
}

//imm8を32bitに符号拡張
static void cmp_rm32_imm8(Emulator* emu, ModRM* modrm){
    if(opsiz)error(emu);
    uint32_t rm32 = get_rm32(emu, modrm);
    int32_t imm8 = get_sign_code8(emu, 0);
    uint64_t result = (uint64_t)rm32 - (uint64_t)imm8;

    update_eflags_sub(emu, rm32, imm8, result);

    emu->eip++;
}

//last 符号拡張
static void sub_rm32_imm8(Emulator* emu, ModRM* modrm){
    if(opsiz)error(emu);
    uint32_t rm32 = get_rm32(emu, modrm);
    int32_t imm8 = get_sign_code8(emu, 0);
    uint64_t result = (uint64_t)rm32 - (uint64_t)imm8;
    set_rm32(emu, modrm, result);
    update_eflags_sub(emu, rm32, imm8, result);
    emu->eip += 1;
}

static void sub_rm32_imm32(Emulator* emu, ModRM* modrm){
    if(opsiz)error(emu);
    uint32_t rm32 = get_rm32(emu, modrm);
    uint32_t imm32 = get_code32(emu, 0);
    emu->eip += 4;
    uint64_t result = (uint64_t)rm32 - (uint64_t)imm32;
    set_rm32(emu, modrm, result);
    update_eflags_sub(emu, rm32, imm32, result);
}

//last
static void sub_rm32_r32(Emulator* emu){
    if(opsiz)error(emu);
    emu->eip++;
    ModRM modrm;
    parse_modrm(emu,&modrm);
    uint32_t rm32 = get_rm32(emu, &modrm);
    uint32_t r32 = get_r32(emu,&modrm);
    uint64_t result = (uint64_t)rm32 - (uint64_t)r32;
    set_rm32(emu, &modrm, result);
    update_eflags_sub(emu, rm32, r32, result);
}

//last
static void sub_rm8_r8(Emulator* emu){
    if(opsiz)error(emu);
    emu->eip++;
    ModRM modrm;
    parse_modrm(emu,&modrm);
    uint8_t rm8 = get_rm8(emu, &modrm);
    uint8_t r8 = get_r8(emu,&modrm);
    uint16_t result = (uint16_t)rm8 - (uint16_t)r8;
    set_rm8(emu, &modrm, result);
    update_eflags_sub8(emu, rm8, r8, result);
}

static void and_rm32_imm8(Emulator* emu,ModRM* modrm){
    if(opsiz)error(emu);
    uint32_t imm8= (uint32_t)get_code8(emu,0);
    uint32_t rm32=get_rm32(emu,modrm);
    uint32_t res=rm32 & imm8;
    set_rm32(emu,modrm,rm32);
    
    update_eflags_or_and(emu,res);
    emu->eip++;
}

static void or_rm32_imm8(Emulator* emu,ModRM* modrm){
    if(opsiz)error(emu);
    uint32_t imm8= (uint32_t)get_code8(emu,0);
    uint32_t rm32=get_rm32(emu,modrm);
    uint32_t res=rm32 | imm8;
    set_rm32(emu,modrm,rm32);
    
    update_eflags_or_and(emu,res);
    emu->eip++;
}

//last
static void and_eax_imm32(Emulator* emu){
    if(opsiz)error(emu);
    uint32_t imm32 = get_code32(emu,1);
    uint32_t eax=get_register32(emu,EAX);
    uint32_t res = eax & imm32;
    set_register32(emu,EAX,res);
    
    update_eflags_or_and(emu,res);
    emu->eip+=5;
}

static void code_83(Emulator* emu){
    emu->eip += 1;
    ModRM modrm;
    parse_modrm(emu, &modrm);

    switch (modrm.nnn) {
    case 0:
        add_rm32_imm8(emu, &modrm);
        break;
    case 1:
        or_rm32_imm8(emu,&modrm);
        break;
    case 4:
        and_rm32_imm8(emu,&modrm);
        break;
    case 5:
        sub_rm32_imm8(emu, &modrm);
        break;
    case 7:
        cmp_rm32_imm8(emu, &modrm);
        break;
    default:
        printf("not implemented: 83 /%d\n", modrm.nnn);
        error(emu);
        exit(1);
    }
}
//last
static void test_rm8_r8(Emulator* emu){
    if(opsiz)error(emu);
    emu->eip++;
    ModRM modrm;
    parse_modrm(emu,&modrm);
    uint8_t rm8,r8,res;
    rm8=get_rm8(emu,&modrm);
    r8=get_r8(emu,&modrm);
    res = rm8 & r8;

    update_eflags_or_and8(emu,res);
}

//andをやってeflags更新して結果すてる
static void test_rm32_r32(Emulator* emu){
    if(opsiz)error(emu);
    emu->eip++;
    ModRM modrm;
    parse_modrm(emu,&modrm);
    uint32_t rm32,r32,res;
    rm32=get_rm32(emu,&modrm);
    r32=get_r32(emu,&modrm);
    res = rm32 & r32;

    update_eflags_or_and(emu,res);

}


static void mov_rm8_imm8(Emulator* emu){
    emu->eip += 1;
    ModRM modrm;
    parse_modrm(emu, &modrm);
    uint8_t value = get_code8(emu, 0);
    set_rm8(emu, &modrm, value);
    emu->eip += 1;
}

static void mov_rm32_imm32(Emulator* emu){
    if(opsiz)error(emu);
    emu->eip += 1;
    ModRM modrm;
    parse_modrm(emu, &modrm);
    uint32_t imm32 = get_code32(emu, 0);
    emu->eip += 4;
    set_rm32(emu, &modrm, imm32);
}


static void je(Emulator* emu)//jump if less
{
    int diff = (is_zero(emu)) ? get_sign_code32(emu, 0) : 0;
    emu->eip += (diff + 4);
}

static void jne(Emulator* emu)//jump if less
{
    int diff = (!is_zero(emu)) ? get_sign_code32(emu, 0) : 0;
    emu->eip += (diff + 4);
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

//last
static void ltr_rm16(Emulator* emu,ModRM* modrm){
    uint32_t rm16=get_rm16(emu,modrm);
    //
}

static void movzx_r32_rm8(Emulator* emu,ModRM* modrm){
    if(opsiz)error(emu);
    uint8_t rm8=get_rm8(emu,modrm);
    set_r32(emu,modrm,(uint32_t)rm8);
}

//last
static void movsx_r32_rm16(Emulator* emu,ModRM* modrm){
    if(opsiz)error(emu);
    uint16_t rm16=get_rm16(emu,modrm);//32bitに符号拡張する
    int16_t sign_rm16=(int16_t)rm16;
    set_r32(emu,modrm,(int32_t)sign_rm16);
}

//last
static void movsx_r32_rm8(Emulator* emu,ModRM* modrm){
    if(opsiz)error(emu);
    uint8_t rm8=get_rm8(emu,modrm);//32bitに符号拡張する
    int8_t sign_rm8=(int8_t)rm8;
    set_r32(emu,modrm,(int32_t)sign_rm8);
}

static void setne_rm8(Emulator* emu,ModRM* modrm){
    if(opsiz)error(emu);
    if(is_zero(emu)){
        set_rm8(emu,modrm,1);
    }else{
        set_rm8(emu,modrm,0);
    }
}

static void code_0F(Emulator* emu){
    emu->eip++;
    uint8_t opecode2=get_code8(emu,0);
    emu->eip++;
    if(opecode2==0x00){
        ModRM modrm;
        parse_modrm(emu,&modrm);
        switch(modrm.nnn){
            case 3:
                ltr_rm16(emu,&modrm);
                break;
            default:
                puts("error not implimented");
                exit(1);
                break;
        }
    }else if(opecode2==0x01){
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
                exit(1);
                break;
        }
    }else if(opecode2==0x84){
        //JE JZ
        je(emu);
    }else if(opecode2==0x85){
        //JE JZ
        jne(emu);
    }else if(opecode2==0x95){
        //setne rm8
        ModRM modrm;
        parse_modrm(emu,&modrm);
        setne_rm8(emu,&modrm);
    
    }else if(opecode2==0xAF){
        ModRM modrm;
        parse_modrm(emu,&modrm);
        imul_r32_rm32(emu,&modrm);
    
    }else if(opecode2==0xB6){
        //movzx_r32_rm8
        ModRM modrm;
        parse_modrm(emu,&modrm);
        movzx_r32_rm8(emu,&modrm);
    }else if(opecode2==0xBE){
        //MOVSX reg32,r/m8
        ModRM modrm;
        parse_modrm(emu,&modrm);
        movsx_r32_rm8(emu,&modrm);
    }else if(opecode2==0xBF){
        //MOVSX reg32,r/m16
        ModRM modrm;
        parse_modrm(emu,&modrm);
        movsx_r32_rm16(emu,&modrm);
    }else{
        puts("0F error not implimented opecode2");
        error(emu);
        exit(1);
    }
}


static void code_F3(Emulator* emu){
    uint8_t opecode2=get_code8(emu,1);
    emu->eip+=2;
    switch(opecode2){
        case 0xa4:
            //REP MOVS m8, m8
            puts("REP MOVS");
            while(get_register32(emu,ECX)!=0){
                set_memory8(emu, get_register32(emu,EDI), (uint32_t)get_memory8(emu,get_register32(emu,ESI)) );
                emu->registers[EDI]--;
                emu->registers[ESI]--;
                emu->registers[ECX]--;
            }
            break;
        default:
            puts("error opecode:F3");
            error(emu);
    }
}

//mov al,[dx]
static void in_al_dx(Emulator* emu){
    if(opsiz)error(emu);

    uint16_t dx = get_register16(emu, DX);
    set_register8(emu, AL, io_in8(dx));
    emu->eip += 1;
}


static void out_dx_al(Emulator* emu){
    if(opsiz)error(emu);

    io_out8(get_register16(emu,DX), get_register8(emu, AL));
    emu->eip += 1;
}

//last
static void inc_rm32(Emulator* emu, ModRM* modrm){
    if(opsiz)error(emu);
    uint32_t rm32 = get_rm32(emu, modrm);
    set_rm32(emu, modrm, rm32 + 1);
    update_eflags_inc(emu,rm32);
}

//last
static void dec_rm32(Emulator* emu, ModRM* modrm){
    if(opsiz)error(emu);
    uint32_t rm32 = get_rm32(emu, modrm);
    set_rm32(emu, modrm, rm32 - 1);
    update_eflags_dec(emu,rm32);
}

//last
static void push_rm32(Emulator* emu,ModRM* modrm){
    if(opsiz)error(emu);
    uint32_t rm32=get_rm32(emu,modrm);
    push32(emu, rm32);
}

static void code_FF(Emulator* emu){
    emu->eip += 1;
    ModRM modrm;
    parse_modrm(emu, &modrm);

    switch (modrm.nnn) {
        case 0:
            inc_rm32(emu, &modrm);
            break;
        case 1:
            dec_rm32(emu, &modrm);
            break;
        case 6:
            push_rm32(emu,&modrm);
            break;
        default:
            printf("not implemented: FF /%d\n", modrm.nnn);
            error(emu);
            exit(1);
    }
}

static void call_rel32(Emulator* emu){
    if(opsiz)error(emu);
    int32_t diff = get_sign_code32(emu, 1);
    push32(emu, emu->eip + 5);
    //puts("-------------call");
    
    //
    emu->eipstack[emu->stackcnt]=emu->eip + 5;
    emu->stackcnt++;
    //

    emu->eip += (diff + 5);
}

static void ret(Emulator* emu){
    uint32_t retaddr=pop32(emu);

    //puts("-------------ret");

    //
    uint32_t stackeip=emu->eipstack[emu->stackcnt-1];
    emu->stackcnt--;

    if(stackeip==retaddr){
        //puts("correct return address");
    }else{
        //printf("incorrect return address\n");
        error(emu);
    }

    emu->eip = retaddr;

}

static void leave(Emulator* emu){
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

//last
static void cmp_al_imm8(Emulator* emu){
    if(opsiz)error(emu);
    uint8_t imm8 = get_code8(emu, 1);//offset気を付けて
    uint8_t al = get_register8(emu, AL);
    uint16_t result = (uint16_t)al - (uint16_t)imm8;
    update_eflags_sub8(emu, al, imm8, result);
    emu->eip += 2;//ope&imm8
}

static void cmp_eax_imm32(Emulator* emu){
    if(opsiz)error(emu);
    uint32_t imm32 = get_code32(emu, 1);
    uint32_t eax = get_register32(emu, EAX);
    uint64_t result = (uint64_t)eax - (uint64_t)imm32;
    update_eflags_sub(emu, eax, imm32, result);
    emu->eip += 5;
}

static void cmp_r32_rm32(Emulator* emu){
    if(opsiz)error(emu);
    emu->eip += 1;
    ModRM modrm;
    parse_modrm(emu, &modrm);
    uint32_t r32 = get_r32(emu, &modrm);
    uint32_t rm32 = get_rm32(emu, &modrm);
    uint64_t result = (uint64_t)r32 - (uint64_t)rm32;
    update_eflags_sub(emu, r32, rm32, result);
}

static void cmp_rm32_r32(Emulator* emu){
    if(opsiz)error(emu);
    emu->eip += 1;
    ModRM modrm;
    parse_modrm(emu, &modrm);
    uint32_t r32 = get_r32(emu, &modrm);
    uint32_t rm32 = get_rm32(emu, &modrm);
    uint64_t result = (uint64_t)rm32 - (uint64_t)r32;
    update_eflags_sub(emu, rm32, r32, result);
}

static void cmp_rm32_imm32(Emulator* emu,ModRM* modrm){
    if(opsiz)error(emu);
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

static void jge(Emulator* emu)//jump 
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


static void jbe(Emulator* emu)//jump if less
{
    //CF==1 or ZF==1
    int diff = (is_carry(emu)||is_zero(emu)) ? get_sign_code8(emu, 1) : 0;
    emu->eip += (diff + 2);
}

static void ja(Emulator* emu)//jump if less
{
    //CF==0 and ZF==0
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

static void lea_r32_m(Emulator* emu){
    if(opsiz)error(emu);
    emu->eip += 1;
    ModRM modrm;
    parse_modrm(emu, &modrm);
    set_r32(emu,&modrm,calc_memory_address(emu,&modrm));
}

static void pushfd(Emulator *emu){
    if(opsiz==0){
        push32(emu,emu->eflags);
    }else{
        error(emu);
        /*int32_t address = get_register32(emu, ESP) - 2;//esp-=4
        set_register32(emu, ESP, address);
        set_memory16(emu, address, emu->eflags & 0xffff);*/
    }
    emu->eip++;
}

static void popfd(Emulator* emu){
    if(opsiz==0){
        emu->eflags=pop32(emu);
    }else{
        error(emu);
        /*uint32_t address = get_register32(emu, ESP);
        emu->eflags |= get_memory16(emu, address);
        set_register32(emu, ESP, address + 2);//esp+=4*/
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
            exit(1);
            break;
    }
}

static void code_81(Emulator* emu)
{
    emu->eip += 1;
    ModRM modrm;
    parse_modrm(emu, &modrm);
    switch(modrm.nnn){
        case 0:
            add_rm32_imm32(emu,&modrm);
            break;
        case 5:
            sub_rm32_imm32(emu,&modrm);
            break;
        case 7:
            cmp_rm32_imm32(emu,&modrm);
            break;
        default:
            puts("error code:81");
            exit(1);
            break;
    }
}



static void sar_rm8_imm8(Emulator* emu,ModRM* modrm){
    if(opsiz)error(emu);
    uint8_t rm8,imm8,res;
    int8_t sgn;
    
    imm8=get_code8(emu,0);
    rm8=get_rm8(emu,modrm);

    sgn=(int8_t)rm8;
    sgn=sgn>>imm8;
    res=(uint8_t)sgn;

    set_rm8(emu,modrm,res);

    update_eflags_sar8(emu,rm8,imm8,res);

    emu->eip++;
}

static void shr_rm8_imm8(Emulator* emu,ModRM* modrm){
    if(opsiz)error(emu);
    uint8_t rm8,imm8,res;
    
    imm8=get_code8(emu,0);
    rm8=get_rm8(emu,modrm);
    
    res=rm8>>imm8;
    update_eflags_shr8(emu,rm8,imm8,res);

    emu->eip++;
}

/*static void sal_rm8_imm8(Emulator* emu,ModRM* modrm){
    //if(!opsiz)error(emu);
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
}*/


//（セグメント：オフセット）のバイトをALに転送します
static void mov_al_moffs8(Emulator* emu){
    if(opsiz)error(emu);
    uint32_t offs=get_code32(emu,1);
    set_register8(emu,AL,get_memory8(emu,offs));
    emu->eip+=5;
}

//last
static void mov_eax_moffs32(Emulator* emu){
    if(opsiz)error(emu);
    uint32_t addr=get_code32(emu,1);
    set_register32(emu,EAX,get_memory32(emu,addr));

    emu->eip+=5;
}

static void mov_moffs32_eax(Emulator* emu){
    if(opsiz)error(emu);
    uint32_t addr = get_code32(emu,1);
    set_memory32(emu,addr,get_register32(emu,EAX));
    emu->eip+=5;
}

static void code_C0(Emulator* emu){
    emu->eip++;
    ModRM modrm;
    parse_modrm(emu,&modrm);

    switch(modrm.nnn){
        /*case 4://rm8をimm8だけ左シフト
            sal_rm8_imm8(emu,&modrm);
            break;*/
        case 5:
            shr_rm8_imm8(emu,&modrm);
            break;
        case 7://rm8をimm8だけ右シフト
            sar_rm8_imm8(emu,&modrm);
            break;
        default:
            puts("error code:C0");
            
            error(emu);
            break;
    }
}

static void shr_rm32_imm8(Emulator* emu,ModRM* modrm){
    if(opsiz)error(emu);
    uint32_t rm32,imm8,res;
    
    imm8=get_code8(emu,0);
    rm32=get_rm32(emu,modrm);
    
    res=rm32>>imm8;
    update_eflags_shr(emu,rm32,imm8,res);

    emu->eip++;
}

static void sar_rm32_imm8(Emulator* emu,ModRM* modrm){
    if(opsiz)error(emu);
    uint32_t rm32,imm8,res;
    int32_t sgn;
    
    imm8=(uint32_t)get_code8(emu,0);
    rm32=get_rm32(emu,modrm);

    sgn=(int32_t)rm32;
    sgn=sgn>>imm8;
    res=(uint32_t)sgn;

    set_rm32(emu,modrm,res);

    update_eflags_sar(emu,rm32,imm8,res);

    emu->eip++;
}

static void sal_rm32_imm8(Emulator* emu,ModRM* modrm){
    if(opsiz)error(emu);
    uint32_t rm32,imm8,res;
    rm32=get_rm32(emu,modrm);
    imm8=get_code8(emu,0);

    res=rm32<<imm8;

    set_rm32(emu,modrm,res);

    int8_t sign=(res>>(32-imm8))&1;
    set_carry(emu,sign);//格納先の最高位

    if(imm8==1){
        uint8_t top2bit=(res>>30)&3;
        if(top2bit==0 || top2bit==3)set_overflow(emu,0);
        else  set_overflow(emu,1);
    }

    emu->eip++;
}

static void code_C1(Emulator* emu){
    //uint64_t rm8,imm8,res8;
    emu->eip++;
    ModRM modrm;
    parse_modrm(emu,&modrm);
    switch(modrm.nnn){
        case 4://shl rm32 imm8
            sal_rm32_imm8(emu,&modrm);
            break;

        //rm32を2でimm8回符号なし除算します
        case 5:
            shr_rm32_imm8(emu,&modrm);
            break;
        case 7:
            sar_rm32_imm8(emu,&modrm);
            break;
        default:
            printf("error code:C1/%d\n",modrm.nnn);
            error(emu);
            exit(1);
            break;
    }

}

//last
static void not_rm8(Emulator* emu,ModRM* modrm){
    uint8_t rm8=get_rm8(emu,modrm);
    set_rm8(emu,modrm,~rm8);
}

//last
static void code_F6(Emulator* emu){
    emu->eip++;
    ModRM modrm;
    parse_modrm(emu,&modrm);
    switch(modrm.nnn){
        case 2://not rm8
            not_rm8(emu,&modrm);
            break;
        default:
            printf("error code:F6/%d\n",modrm.nnn);
            error(emu);
            exit(1);
            break;
    }

}


static void opsize(Emulator* emu){
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

static void nop(Emulator* emu){
    emu->eip++;
}

void init_instructions(void){
    int i;
    memset(instructions, 0, sizeof(instructions));
    instructions[0x00] = add_rm8_r8;
    instructions[0x01] = add_rm32_r32;
    instructions[0x03] = add_r32_rm32;

    instructions[0x05] = add_eax_imm32;
    instructions[0x08] = or_rm8_r8;
    instructions[0x09] = or_rm32_r32;
    instructions[0x0F] = code_0F;

    instructions[0x25] = and_eax_imm32;

    instructions[0x28] = sub_rm8_r8;
    instructions[0x29] = sub_rm32_r32;
    
    instructions[0x31] = xor_rm32_r32;

    instructions[0x39] = cmp_rm32_r32;
    instructions[0x3B] = cmp_r32_rm32;
    instructions[0x3C] = cmp_al_imm8;
    //instructions[0x3D] = cmp_eax_imm32;
    //instructions[0x3D] = cmp_rm32_r32;

    

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
    //instructions[0x61] = popad;
    instructions[0x66] = opsize;
    instructions[0x68] = push_imm32;
    instructions[0x69] = imul_r32_rm32_imm32;
    instructions[0x6A] = push_imm8;
    instructions[0x6B] = imul_r32_rm32_imm8;

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
    instructions[0x84] = test_rm8_r8;
    instructions[0x85] = test_rm32_r32;
    instructions[0x88] = mov_rm8_r8;
    instructions[0x89] = mov_rm32_r32;
    instructions[0x90] = nop;
    instructions[0x8A] = mov_r8_rm8;
    instructions[0x8B] = mov_r32_rm32;

    instructions[0x8D] = lea_r32_m;
    instructions[0x9C] = pushfd;
    instructions[0x9D] = popfd;
    

    instructions[0xA0] = mov_al_moffs8;
    instructions[0xA1] = mov_eax_moffs32;
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

    //instructions[0xCD] = swi;

    instructions[0xE8] = call_rel32;
    instructions[0xE9] = near_jump;
    instructions[0xEB] = short_jump;
    instructions[0xEC] = in_al_dx;
    instructions[0xEE] = out_dx_al;


    instructions[0xF3] = code_F3;
    instructions[0xF6] = code_F6;
    instructions[0xFA] = cli;
    instructions[0xFB] = sti;

    instructions[0xFF] = code_FF;
}

