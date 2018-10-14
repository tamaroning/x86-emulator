#include "io.h"

#include <stdio.h>
#include "emulator.h"

//値かきこみ
uint8_t io_in8(uint16_t address)
{
    switch (address) {
    case 0x03f8:
        return getchar();
        break;
    default:
        return 0;
    }
}

//値取得
void io_out8(uint16_t address, uint8_t value)
{
    switch (address) {
    case 0x03f8:
        putchar(value);
        break;
    }
}
