#include "Strings.h"
#include <string.h>
#include <stdlib.h>
#include <assert.h>

/* Function implementations */
int64_t ob_7Strings_Length(char* text, int64_t text_len) {
    int64_t i;
    i = 0;
    while (((i < text_len) && (((int64_t)((uint8_t)(text[i]))) != 0))) {
        i = (i + 1);
    };
    return i;
}

void ob_7Strings_Copy(char* src, char* dst, int64_t src_len, int64_t dst_len) {
    int64_t i;
    int64_t limit;
    limit = (dst_len - 1);
    if ((limit >= 0)) {
        i = 0;
        while ((((i < limit) && (i < src_len)) && (((int64_t)((uint8_t)(src[i]))) != 0))) {
            dst[i] = src[i];
            i = (i + 1);
        };
        dst[i] = ((char)((uint8_t)(0)));
    };
}

bool ob_7Strings_Equals(char* left, char* right, int64_t left_len, int64_t right_len) {
    int64_t i;
    int64_t leftLen;
    int64_t rightLen;
    bool result;
    leftLen = ob_7Strings_Length(left, left_len);
    rightLen = ob_7Strings_Length(right, right_len);
    result = (leftLen == rightLen);
    if (result) {
        i = 0;
        while (((i < leftLen) && result)) {
            if ((left[i] != right[i])) {
                result = false;
            };
            i = (i + 1);
        };
    };
    return result;
}

bool ob_7Strings_EndsWith(char* text, char* suffix, int64_t text_len, int64_t suffix_len) {
    int64_t textLen;
    int64_t suffixLen;
    int64_t offset;
    int64_t i;
    bool result;
    textLen = ob_7Strings_Length(text, text_len);
    suffixLen = ob_7Strings_Length(suffix, suffix_len);
    result = (suffixLen <= textLen);
    if (result) {
        offset = (textLen - suffixLen);
        i = 0;
        while (((i < suffixLen) && result)) {
            if ((text[(offset + i)] != suffix[i])) {
                result = false;
            };
            i = (i + 1);
        };
    };
    return result;
}

bool ob_7Strings_StartsWith(char* text, char* prefix, int64_t text_len, int64_t prefix_len) {
    int64_t textLen;
    int64_t prefixLen;
    int64_t offset;
    int64_t i;
    bool result;
    textLen = ob_7Strings_Length(text, text_len);
    prefixLen = ob_7Strings_Length(prefix, prefix_len);
    result = (prefixLen <= textLen);
    if (result) {
        i = 0;
        while (((i < prefixLen) && result)) {
            if ((text[i] != prefix[i])) {
                result = false;
            };
            i = (i + 1);
        };
    };
    return result;
}

void ob_7Strings_CopyRange(char* src, int64_t start, int64_t finish, char* dst, int64_t src_len, int64_t dst_len) {
    int64_t i;
    int64_t j;
    int64_t limit;
    int64_t actualFinish;
    int64_t srcLen;
    limit = (dst_len - 1);
    if ((limit >= 0)) {
        if ((start < 0)) {
            start = 0;
        };
        srcLen = ob_7Strings_Length(src, src_len);
        actualFinish = finish;
        if ((actualFinish > srcLen)) {
            actualFinish = srcLen;
        };
        if ((actualFinish < start)) {
            actualFinish = start;
        };
        i = start;
        j = 0;
        while (((i < actualFinish) && (j < limit))) {
            dst[j] = src[i];
            i = (i + 1);
            j = (j + 1);
        };
        dst[j] = ((char)((uint8_t)(0)));
    };
}

int64_t ob_7Strings_Find(char* text, char target, int64_t start, int64_t text_len) {
    int64_t i;
    int64_t textLen;
    int64_t result;
    if ((start < 0)) {
        start = 0;
    };
    textLen = ob_7Strings_Length(text, text_len);
    result = (-1);
    i = start;
    while (((i < textLen) && (result == (-1)))) {
        if ((text[i] == target)) {
            result = i;
        };
        i = (i + 1);
    };
    return result;
}

void ob_7Strings_7Strings(void) {
    static int _initialized = 0;
    if (_initialized) return;
    _initialized = 1;
}

