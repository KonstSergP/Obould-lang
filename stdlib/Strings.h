#ifndef STRINGS_H
#define STRINGS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* RTTI Support */
#ifndef STRUCT_DESCRIPTOR_DEFINED
#define STRUCT_DESCRIPTOR_DEFINED
typedef struct StructDescriptor {
    const char* name;
    int level;
    struct StructDescriptor** ancestors;
} StructDescriptor;
#endif

/* Function prototypes */
int64_t ob_7Strings_Length(char* text, int64_t text_len);
void ob_7Strings_Copy(char* src, char** dst, int64_t src_len, int64_t dst_len);
bool ob_7Strings_Equals(char* left, char* right, int64_t left_len, int64_t right_len);
bool ob_7Strings_EndsWith(char* text, char* suffix, int64_t text_len, int64_t suffix_len);
bool ob_7Strings_StartsWith(char* text, char* prefix, int64_t text_len, int64_t prefix_len);
void ob_7Strings_CopyRange(char* src, int64_t start, int64_t finish, char** dst, int64_t src_len, int64_t dst_len);
int64_t ob_7Strings_Find(char* text, char target, int64_t start, int64_t text_len);

void ob_7Strings_Strings(void);

#endif /* STRINGS_H */
