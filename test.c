#include<stdio.h>
#include<stdint.h>


int main(){

    uint8_t us=128;//1000 0000b
    int8_t s=-128;//1000 0000b

    uint8_t cnt=3;

    printf("unsigned  0x%x >> %d = 0x%X\n",us,cnt,(uint8_t)(us>>cnt));
    printf("  signed  0x%x >> %d = 0x%X",(uint8_t)s,cnt,(uint8_t)(s>>cnt));
    
}
