#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#if defined(__APPLE__)
#define OB_SYMBOL(name) __asm__("_ob_" name)
#else
#define OB_SYMBOL(name) __asm__("ob_" name)
#endif

void WriteChar(char ch) OB_SYMBOL("3Out_WriteChar");
void WriteString(const char* s, int64_t s_len) OB_SYMBOL("3Out_WriteString");
void WriteStringLen(const char* s, int64_t n, int64_t s_len) OB_SYMBOL("3Out_WriteStringLen");
void WriteInt(int64_t x) OB_SYMBOL("3Out_WriteInt");
void WriteIntWidth(int64_t x, int64_t width) OB_SYMBOL("3Out_WriteIntWidth");
void WriteHex(int64_t x, int64_t width) OB_SYMBOL("3Out_WriteHex");
void WriteByte(uint8_t x) OB_SYMBOL("3Out_WriteByte");
void WriteBool(bool b) OB_SYMBOL("3Out_WriteBool");
void WriteReal(double x, int64_t digits) OB_SYMBOL("3Out_WriteReal");
void WriteLn(void) OB_SYMBOL("3Out_WriteLn");
void WriteSpace(void) OB_SYMBOL("3Out_WriteSpace");
void Out(void) OB_SYMBOL("Out");


void WriteString(const char* s, int64_t s_len)
{
    printf("%.*s", (int)s_len, s);
    fflush(stdout);
}

void WriteStringLen(const char* s, int64_t n, int64_t s_len)
{
    if (n < 0) n = 0;
    if (n > s_len) n = s_len;
    printf("%.*s", (int)n, s);
    fflush(stdout);
}

void WriteInt(int64_t x)
{
    printf("%lld", (long long)x);
    fflush(stdout);
}

void WriteIntWidth(int64_t x, int64_t width)
{
    int w = (int)width;
    if (w <= 0) {
        printf("%lld", (long long)x);
    }
    else {
        printf("%*lld", w, (long long)x);
    }
    fflush(stdout);
}

void WriteHex(int64_t x, int64_t width)
{
    unsigned long long v = (unsigned long long)x;
    int w = (int)width;
    if (w <= 0) {
        printf("%llx", v);
    }
    else {
        printf("%0*llx", w, v);
    }
    fflush(stdout);
}

void WriteByte(uint8_t x)
{
    printf("%u", (unsigned int)x);
    fflush(stdout);
}

void WriteBool(bool b)
{
    fputs(b ? "True" : "False", stdout);
    fflush(stdout);
}

void WriteReal(double x, int64_t digits)
{
    int p = (int)digits;
    if (p < 0) p = 0;
    printf("%.*f", p, x);
    fflush(stdout);
}

void WriteLn(void)
{
    putchar('\n');
    fflush(stdout);
}

void WriteSpace(void)
{
    putchar(' ');
    fflush(stdout);
}

void WriteChar(char ch)
{
    putchar((unsigned char)ch);
    fflush(stdout);
}

void Out(void) {}
