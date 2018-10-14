#ifndef MODRM_H_
#define MODRM_H_

#include <stdint.h>

#include "emulator.h"

typedef struct {
    uint8_t mod;

    union {
        uint8_t opecode;//どっちも
        uint8_t reg_index;//同じ
    };

    uint8_t rm;

    /* SIB が必要な mod/rm の組み合わせの時に使う */
    uint8_t sib;

    union {
        int8_t disp8; //signed
        uint32_t disp32;
    };
} ModRM;

/* ModR/M, SIB, ディスプレースメントを解析する
 * emu から ModR/M, SIB, ディスプレースメントを読み取って modrm にセットする。
 * 呼び出しのとき emu->eip は ModR/M バイトを指している必要がある。
 * この関数は emu->eip を即値（即値がない場合は次の命令）の先頭を指すように変更する。
 *
 * 引数
 *   emu: eip が ModR/M バイトの先頭を指しているエミュレータ構造体
 *   modrm: 解析結果を格納する構造体
 */
void parse_modrm(Emulator* emu, ModRM* modrm);

/* ModR/M の内容に基づきメモリの実効アドレスを計算する
 *
 * modrm->mod は 0, 1, 2 のいずれかでなければならない
 */
uint32_t calc_memory_address(Emulator* emu, ModRM* modrm);

/* rm32のレジスタまたはメモリの32bit値を取得する */
uint32_t get_rm32(Emulator* emu, ModRM* modrm);

/* rm32のレジスタまたはメモリの32bit値を設定する
 *
 * modrm の内容に従って value を目的のメモリまたはレジスタに書き込む
 *
 * 引数
 *   emu: エミュレータ構造体（eip はどこを指していても良い）
 *   modrm: ModR/M（SIB, disp を含む）
 *   value: 即値
 */
void set_rm32(Emulator* emu, ModRM* modrm, uint32_t value);

/* r32のレジスタの32bit値を取得する */
uint32_t get_r32(Emulator* emu, ModRM* modrm);

/* r32のレジスタの32bit値を設定する */
void set_r32(Emulator* emu, ModRM* modrm, uint32_t value);

/* 8ビット版 */
uint8_t get_rm8(Emulator* emu, ModRM* modrm);
void set_rm8(Emulator* emu, ModRM* modrm, uint8_t value);
uint8_t get_r8(Emulator* emu, ModRM* modrm);
void set_r8(Emulator* emu, ModRM* modrm, uint8_t value);

#endif
