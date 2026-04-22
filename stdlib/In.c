#include <stdint.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

#if defined(__APPLE__)
#define OB_SYMBOL(name) __asm__("_ob_" name)
#else
#define OB_SYMBOL(name) __asm__("ob_" name)
#endif

int64_t Int(void) OB_SYMBOL("2In_Int");
double Real(void) OB_SYMBOL("2In_Real");
char Char(void) OB_SYMBOL("2In_Char");
uint8_t Byte(void) OB_SYMBOL("2In_Byte");
int64_t String(char* s, int64_t s_len) OB_SYMBOL("2In_String");
int64_t Line(char* s, int64_t s_len) OB_SYMBOL("2In_Line");
void In(void) OB_SYMBOL("2In_In");


bool Done OB_SYMBOL("2In_Done") = true;
bool Truncated OB_SYMBOL("2In_Truncated") = false;


static int64_t read_token(char* s, int64_t s_len)
{
    Done = false;
    Truncated = false;
    int c = getchar();

    while (c != EOF && isspace(c)) {
        c = getchar();
    }
    if (c == EOF) {
        if (s_len > 0 && s) s[0] = '\0';
        return 0;
    }

    int64_t i = 0;
    while (c != EOF && !isspace(c)) {
        if (s_len > 0 && s != NULL && i < s_len - 1) {
            s[i++] = (char)c;
        } else if (s_len > 0 && s != NULL) {
            Truncated = true;
        }
        c = getchar();
    }

    if (s_len > 0 && s != NULL) {
        s[i] = '\0';
    }

    if (c == '\r') {
        int next_c = getchar();
        if (next_c != '\n' && next_c != EOF) {
            ungetc(next_c, stdin);
        }
    }

    Done = true;
    return i;
}

int64_t Int(void)
{
    Truncated = false;
    char buf[64];
    if (read_token(buf, sizeof(buf)) == 0) {
        return 0;
    }

    char* endptr;
    int64_t val = (int64_t)strtoll(buf, &endptr, 10);

    if (*endptr != '\0') {
        Done = false;
        return 0;
    }

    Done = true;
    return val;
}

double Real(void)
{
    Truncated = false;
    char buf[128];
    if (read_token(buf, sizeof(buf)) == 0) {
        return 0.0;
    }

    char* endptr;
    double val = strtod(buf, &endptr);

    if (*endptr != '\0') {
        Done = false;
        return 0.0;
    }

    Done = true;
    return val;
}

char Char(void)
{
    Done = false;
    Truncated = false;
    int c = getchar();
    if (c == EOF) {
        return 0;
    }
    Done = true;
    return (char)c;
}

uint8_t Byte(void)
{
    Truncated = false;
    char buf[64];
    if (read_token(buf, sizeof(buf)) == 0) {
        return 0;
    }

    if (buf[0] == '-') {
        Done = false;
        return 0;
    }

    char* endptr;
    unsigned long val = strtoul(buf, &endptr, 10);

    if (endptr == buf || *endptr != '\0' || val > 255) {
        Done = false;
        return 0;
    }

    Done = true;
    return (uint8_t)val;
}

int64_t String(char* s, int64_t s_len)
{
    Truncated = false;
    return read_token(s, s_len);
}

int64_t Line(char* s, int64_t s_len)
{
    Done = false;
    Truncated = false;

    if (s_len <= 0 || s == NULL) {
        int c = getchar();
        while (c != EOF && c != '\n') {
            c = getchar();
        }
        Done = c != EOF;
        return 0;
    }

    int cap = (s_len > INT32_MAX) ? INT32_MAX : (int)s_len;
    if (!fgets(s, cap, stdin)) {
        s[0] = '\0';
        return 0;
    }

    size_t len = strlen(s);

    if (len > 0 && s[len - 1] != '\n') {
        int next_c = getchar();

        if (next_c == '\n' || next_c == '\r') {
            if (next_c == '\r') {
                int peek = getchar();
                if (peek != '\n' && peek != EOF) ungetc(peek, stdin);
            }
        }
        else if (next_c != EOF) {
            Truncated = true;
            while (next_c != '\n' && next_c != EOF) {
                next_c = getchar();
            }
        }
    }

    while (len > 0 && (s[len - 1] == '\n' || s[len - 1] == '\r')) {
        s[--len] = '\0';
    }

    Done = true;
    return (int64_t)len;
}

void In(void) {}
