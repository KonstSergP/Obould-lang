#include <stdint.h>
#include <stdio.h>

#if defined(__APPLE__)
#define OB_SYMBOL(name) __asm__("_" name)
#else
#define OB_SYMBOL(name) __asm__(name)
#endif

void WriteInt(int64_t x) OB_SYMBOL("ob_3Out_WriteInt");
void WriteLn(void) OB_SYMBOL("ob_3Out_WriteLn");
void init(void) OB_SYMBOL("ob_3Out_init");
void Out(void) OB_SYMBOL("ob_Out");

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
