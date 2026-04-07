#ifndef OBOULDCOMPILER_FILES_H
#define OBOULDCOMPILER_FILES_H

#include <stdint.h>
#include <stdbool.h>

typedef struct ob_5Files_FileHandle ob_5Files_FileHandle;
typedef ob_5Files_FileHandle* ob_5Files_File;

typedef struct ob_5Files_FileRider {
    void* _desc;
    bool eof;
    uint8_t res;
    int64_t pos;
    ob_5Files_File file;
} ob_5Files_FileRider;

ob_5Files_File ob_5Files_Open(const char* name, const char* mode, int64_t name_len, int64_t mode_len);
void ob_5Files_Close(ob_5Files_File f);
void ob_5Files_Set(ob_5Files_FileRider* r, ob_5Files_File f, int64_t pos);
void ob_5Files_ReadByte(ob_5Files_FileRider* r, uint8_t* b);
void ob_Files(void);

#endif //OBOULDCOMPILER_FILES_H
