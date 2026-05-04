#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <gc.h>
#include "Parse.h"


#if defined(__APPLE__)
#define OB_SYMBOL(name) __asm__("_ob_" name)
#else
#define OB_SYMBOL(name) __asm__("ob_" name)
#endif

bool Int(const char* text, int64_t* value, int64_t text_len) OB_SYMBOL("5Parse_Int");
bool Real(const char* text, double* value, int64_t text_len) OB_SYMBOL("5Parse_Real");
void Parse(void) OB_SYMBOL("5Parse_5Parse");


static char* to_c_string(const char* s, int64_t s_len)
{
    if (s_len < 0) s_len = 0;
    size_t max = (size_t)s_len;

    if (s == NULL) {
        char* out = GC_MALLOC(1);
        if (out) out[0] = '\0';
        return out;
    }

    size_t actual = 0;
    while (actual < max && s[actual] != '\0') {
        actual++;
    }

    char* out = GC_MALLOC(actual + 1);
    if (!out) return NULL;
    if (actual > 0) {
        memcpy(out, s, actual);
    }
    out[actual] = '\0';
    return out;
}

bool Int(const char* text, int64_t* value, int64_t text_len)
{
    char* endptr;

    if (value == NULL) return false;
    char* buffer = to_c_string(text, text_len);
    if (!buffer) return false;

    long long parsed = strtoll(buffer, &endptr, 10);
    if (endptr == buffer || *endptr != '\0') {
        return false;
    }

    *value = (int64_t)parsed;
    return true;
}

bool Real(const char* text, double* value, int64_t text_len)
{
    char* endptr;

    if (value == NULL) return false;
    char* buffer = to_c_string(text, text_len);
    if (!buffer) return false;

    double parsed = strtod(buffer, &endptr);
    if (endptr == buffer || *endptr != '\0') {
        return false;
    }

    *value = parsed;
    return true;
}

void Parse(void) {}
