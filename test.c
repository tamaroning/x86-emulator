#include<stdio.h>
#include<stdint.h>

int32_t unsign2sign(uint32_t a){
    return (a & 0x80000000) ?  -(~a & 0x7FFFFFFF)-1 : (a & 0x7FFFFFFF);
}

int main(){

    uint32_t a=(1<<31)+100;
    int32_t b=(int32_t)a;
    int32_t c=a;
    int32_t d=(uint32_t)a;
    int32_t e=unsign2sign(a);
    printf("a =%d , %x\n",a,a);
    printf("b =%d , %x\n",b,b);
    printf("c =%d , %x\n",c,c);
    printf("d =%d , %x\n",d,d);
    printf("e =%d , %x\n",e,e);
    return 0;
}
