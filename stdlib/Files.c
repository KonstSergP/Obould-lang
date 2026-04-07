#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "Files.h"

#if defined(__APPLE__)
#define OB_SYMBOL(name) __asm__("_ob_" name)
#else
#define OB_SYMBOL(name) __asm__("ob_" name)
#endif


typedef ob_5Files_File File;
typedef ob_5Files_FileRider FileRider;

typedef struct ob_5Files_FileHandle
{
    FILE* stream;
} FileHandle;

typedef struct FileRiderDescriptor
{
    int64_t depth;
    void* descriptors[1];
} FileRiderDescriptor;

FileRiderDescriptor fileRiderDescriptor OB_SYMBOL("5Files_struct_desc_FileRider") = {
    0, {&fileRiderDescriptor}
};

File Open(const char* name, const char* mode, int64_t name_len, int64_t mode_len) OB_SYMBOL("5Files_Open");
void Close(File f) OB_SYMBOL("5Files_Close");
void Set(FileRider* r, File f, int64_t pos) OB_SYMBOL("5Files_Set");
void ReadByte(FileRider* r, uint8_t* b) OB_SYMBOL("5Files_ReadByte");
void Files(void) OB_SYMBOL("Files");


static void FileHandleInit(FileHandle* f)
{
    f->stream = NULL;
}

static void FileRiderInit(FileRider* r)
{
    r->_desc = &fileRiderDescriptor;
    r->eof = false;
    r->res = 0;
    r->pos = 0;
    r->file = NULL;
}

static char* copy_c_string(const char* s, int64_t s_len)
{
    if (s_len < 0) s_len = 0;
    size_t max = (size_t)s_len;

    if (s == NULL) {
        char* out = malloc(1);
        if (out) out[0] = '\0';
        return out;
    }

    size_t actual = 0;
    while (actual < max && s[actual] != '\0') {
        actual++;
    }

    char* out = malloc(actual + 1);
    if (!out) return NULL;
    if (actual > 0) {
        memcpy(out, s, actual);
    }
    out[actual] = '\0';
    return out;
}

File Open(const char* name, const char* mode, int64_t name_len, int64_t mode_len)
{
    char* name_c = copy_c_string(name, name_len);
    char* mode_c = copy_c_string(mode, mode_len);

    if (!name_c || !mode_c) {
        free(name_c);
        free(mode_c);
        return NULL;
    }

    FILE* stream = fopen(name_c, mode_c);
    free(name_c);
    free(mode_c);

    if (!stream) {
        return NULL;
    }

    FileHandle* f = malloc(sizeof(FileHandle));
    if (!f) {
        fclose(stream);
        return NULL;
    }
    FileHandleInit(f);

    f->stream = stream;
    return f;
}

void Close(File f)
{
    if (!f) return;
    if (f->stream) {
        fclose(f->stream);
        f->stream = NULL;
    }
    free(f);
}

void Set(FileRider* r, File f, int64_t pos)
{
    if (!r) return;
    FileRiderInit(r);
    r->file = f;
    r->pos = pos < 0 ? 0 : pos;
    r->eof = false;
    r->res = 0;

    if (!f || !f->stream) {
        r->eof = true;
        r->res = 2;
        return;
    }

    if (fseek(f->stream, r->pos, SEEK_SET) != 0) {
        r->eof = true;
        r->res = 2;
        return;
    }
}

void ReadByte(FileRider* r, uint8_t* b)
{
    if (!r) return;

    if (!r->file || !r->file->stream) {
        r->eof = true;
        r->res = 2;
        return;
    }

    int c = fgetc(r->file->stream);
    if (c == EOF) {
        if (feof(r->file->stream)) {
            r->res = 1;
        }
        else {
            r->res = 2;
        }
        r->eof = true;
        return;
    }

    if (b) *b = (uint8_t)c;
    r->pos += 1;
    r->eof = false;
    r->res = 0;
}

void Files(void) {}
