#include <stdint.h>
#include <stdio.h>

void Out_WriteInt(int64_t x) __asm__("Out.WriteInt");
void Out_WriteLn(void) __asm__("Out.WriteLn");
void Out_init(void) __asm__("Out.init");
void _Out(void) __asm__("_Out");

void Out_WriteInt(int64_t x)
{
    printf("%lld", (long long)x);
    fflush(stdout);
}

void Out_WriteLn(void)
{
    putchar('\n');
    fflush(stdout);
}

void Out_init(void)
{
}

void _Out(void)
{
    Out_init();
}
