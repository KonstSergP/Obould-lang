#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#if defined(__APPLE__)
#define OB_SYMBOL(name) __asm__("_ob_" name)
#else
#define OB_SYMBOL(name) __asm__("ob_" name)
#endif

void Char(char ch) OB_SYMBOL("3Out_Char");
void String(const char* s, int64_t s_len) OB_SYMBOL("3Out_String");
void StringLen(const char* s, int64_t n, int64_t s_len) OB_SYMBOL("3Out_StringLen");
void Int(int64_t x) OB_SYMBOL("3Out_Int");
void IntWidth(int64_t x, int64_t width) OB_SYMBOL("3Out_IntWidth");
void Hex(int64_t x, int64_t width) OB_SYMBOL("3Out_Hex");
void Byte(uint8_t x) OB_SYMBOL("3Out_Byte");
void Bool(bool b) OB_SYMBOL("3Out_Bool");
void Real(double x, int64_t digits) OB_SYMBOL("3Out_Real");
void Ln(void) OB_SYMBOL("3Out_Ln");
void Space(void) OB_SYMBOL("3Out_Space");
void Out(void) OB_SYMBOL("Out");


void String(const char* s, int64_t s_len)
{
    printf("%.*s", (int)s_len, s);
    fflush(stdout);
}

void StringLen(const char* s, int64_t n, int64_t s_len)
{
    if (n < 0) n = 0;
    if (n > s_len) n = s_len;
    printf("%.*s", (int)n, s);
    fflush(stdout);
}

void Int(int64_t x)
{
    printf("%lld", (long long)x);
    fflush(stdout);
}

void IntWidth(int64_t x, int64_t width)
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

void Hex(int64_t x, int64_t width)
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

void Byte(uint8_t x)
{
    printf("%u", (unsigned int)x);
    fflush(stdout);
}

void Bool(bool b)
{
    fputs(b ? "True" : "False", stdout);
    fflush(stdout);
}

void Real(double x, int64_t digits)
{
    int p = (int)digits;
    if (p < 0) p = 0;
    printf("%.*f", p, x);
    fflush(stdout);
}

void Ln(void)
{
    putchar('\n');
    fflush(stdout);
}

void Space(void)
{
    putchar(' ');
    fflush(stdout);
}

void Char(char ch)
{
    putchar((unsigned char)ch);
    fflush(stdout);
}

void Out(void) {}
