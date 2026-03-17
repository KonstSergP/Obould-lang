#include <stdint.h>
#include <stdio.h>

void WriteInt(int64_t x) __asm__("ob_3Out_WriteInt");
void WriteLn(void) __asm__("ob_3Out_WriteLn");
void init(void) __asm__("ob_3Out_init");
void Out(void) __asm__("ob_Out");

void WriteInt(int64_t x)
{
    printf("%lld", (long long)x);
    fflush(stdout);
}

void WriteLn(void)
{
    putchar('\n');
    fflush(stdout);
}

void init(void)
{
}

void Out(void)
{
    init();
}
