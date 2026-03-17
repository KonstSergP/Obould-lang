#include <stdint.h>
#include <stdio.h>

void ob_3Out_WriteInt(int64_t x);
void ob_3Out_WriteLn(void);
void ob_3Out_init(void);
void ob_Out(void);

void ob_3Out_WriteInt(int64_t x)
{
    printf("%lld", (long long)x);
    fflush(stdout);
}

void ob_3Out_WriteLn(void)
{
    putchar('\n');
    fflush(stdout);
}

void ob_3Out_init(void)
{
}

void ob_Out(void)
{
    ob_3Out_init();
}
