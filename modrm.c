#include "modrm.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "emulator_function.h"

void parse_sib(Emulator* emu, ModRM* modrm) {
  // assert(emu != NULL && modrm->sib != NULL);//true確認
  memset(&modrm->sib, 0, sizeof(SIB));  //全部を0に初期化

  modrm->sib.scale = ((modrm->sib_byte >> 6) & 3);
  modrm->sib.reg_index = ((modrm->sib_byte >> 3) & 7);  //==ESPなら使わない
  modrm->sib.base = modrm->sib_byte & 7;

  modrm->sib.scale = 1 << modrm->sib.scale;  // scale=1,2,4,8
  // modrm->sib.base_addr = get_register32(emu,modrm->sib.base_addr);

  // printf("sib:%X scale:%d reg:%d
  // base:%d\n",modrm->sib_byte,modrm->sib.scale,modrm->sib.reg_index,modrm->sib.base);

  emu->eip++;
}

// eipがsibの次バイトをさすようにする
uint32_t sib_calc_mem_addr(Emulator* emu, SIB* sib) {
  //[base+index*n+disp]
  // index=espのときはつかわない

  // printf("%2X",emu->memory[emu->eip]);
  if (sib->base != ESP) {
    // printf("sib :::: %x",get_register32(emu,sib->base) +
    // get_register32(emu,sib->reg_index) * sib->scale);
    return get_register32(emu, sib->base) +
           get_register32(emu, sib->reg_index) * sib->scale;
  } else {
    // if(modrm.mod==0)return disp;
    return get_register32(emu, sib->reg_index);
  }
}

void parse_modrm(Emulator* emu, ModRM* modrm)  // eipがmodR/Mの先頭
{
  assert(emu != NULL && modrm != NULL);  // true確認

  memset(modrm, 0, sizeof(ModRM));  //全部を0に初期化

  uint8_t code = get_code8(emu, 0);  //全体取得
  modrm->mod = ((code & 0xC0) >> 6);
  modrm->nnn = ((code & 0x38) >> 3);
  modrm->rm = code & 0x07;

  emu->eip += 1;

  if (modrm->mod != 3 && modrm->rm == 4) {
    // sibバイト取得
    modrm->sib_byte = get_code8(emu, 0);
    parse_sib(emu, modrm);
  }

  if ((modrm->mod == 0 && modrm->rm == 5) || modrm->mod == 2) {
    modrm->disp32 = get_sign_code32(emu, 0);
    emu->eip += 4;

  } else if (modrm->mod == 1) {
    modrm->disp8 = get_sign_code8(emu, 0);
    emu->eip += 1;
  }
}

uint32_t calc_memory_address(Emulator* emu, ModRM* modrm) {
  if (modrm->mod == 0) {
    if (modrm->rm == 4) {
      //[--][--]
      return sib_calc_mem_addr(emu, &(modrm->sib));
      // printf("not implemented ModRM mod = 0, rm = 4\n");
      // exit(0);
    } else if (modrm->rm == 5) {
      return modrm->disp32;
    } else {
      return get_register32(emu, modrm->rm);
    }
  } else if (modrm->mod == 1) {
    if (modrm->rm == 4) {
      //[--][--]+disp8
      return sib_calc_mem_addr(emu, &(modrm->sib)) + modrm->disp8;
      // printf("not implemented ModRM mod = 1, rm = 4\n");
      // exit(0);
    } else {
      return get_register32(emu, modrm->rm) + modrm->disp8;
    }
  } else if (modrm->mod == 2) {
    if (modrm->rm == 4) {
      //[--][--]+disp32
      return sib_calc_mem_addr(emu, &(modrm->sib)) + modrm->disp32;
      // printf("not implemented ModRM mod = 2, rm = 4\n");
      // exit(0);
    } else {
      return get_register32(emu, modrm->rm) + modrm->disp32;
    }
  } else {  // mod==3
    // last
    return get_register32(emu, modrm->rm);
    // printf("not implemented ModRM mod = 3\n");
    error(emu);
    exit(0);
  }
}

void set_rm8(Emulator* emu, ModRM* modrm, uint8_t value) {
  if (modrm->mod == 3) {
    set_register8(emu, modrm->rm, value);
  } else {
    uint32_t address = calc_memory_address(emu, modrm);
    // printf("0x%X",address);
    set_memory8(emu, address, value);
  }
}

void set_rm16(Emulator* emu, ModRM* modrm, uint16_t value) {
  if (modrm->mod == 3) {
    set_register16(emu, modrm->rm, value);
  } else {
    uint32_t address = calc_memory_address(emu, modrm);
    // printf("0x%X",address);
    set_memory16(emu, address, value);
  }
}

void set_rm32(Emulator* emu, ModRM* modrm, uint32_t value) {
  if (modrm->mod == 3) {
    set_register32(emu, modrm->rm, value);
  } else {
    uint32_t address = calc_memory_address(emu, modrm);
    set_memory32(emu, address, value);
  }
}

uint8_t get_rm8(Emulator* emu, ModRM* modrm) {
  if (modrm->mod == 3) {
    return get_register8(emu, modrm->rm);
  } else {
    uint32_t address = calc_memory_address(emu, modrm);
    return get_memory8(emu, address);
  }
}

uint16_t get_rm16(Emulator* emu, ModRM* modrm) {
  if (modrm->mod == 3) {
    return get_register16(emu, modrm->rm);
  } else {
    uint32_t address = calc_memory_address(emu, modrm);
    return get_memory16(emu, address);
  }
}

uint32_t get_rm32(Emulator* emu, ModRM* modrm) {
  if (modrm->mod == 3) {
    return get_register32(emu, modrm->rm);
  } else {
    uint32_t address = calc_memory_address(emu, modrm);
    return get_memory32(emu, address);
  }
}

void set_r8(Emulator* emu, ModRM* modrm, uint8_t value) {
  set_register8(emu, modrm->reg_index, value);
}

void set_r16(Emulator* emu, ModRM* modrm, uint16_t value) {
  set_register16(emu, modrm->reg_index, value);
}

void set_r32(Emulator* emu, ModRM* modrm, uint32_t value) {
  set_register32(emu, modrm->reg_index, value);
}

uint8_t get_r8(Emulator* emu, ModRM* modrm) {
  return get_register8(emu, modrm->reg_index);
}

uint16_t get_r16(Emulator* emu, ModRM* modrm) {
  return get_register16(emu, modrm->reg_index);
}

uint32_t get_r32(Emulator* emu, ModRM* modrm) {
  return get_register32(emu, modrm->reg_index);
}
