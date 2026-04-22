#ifndef FILES_H
#define FILES_H

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
void ob_5Files_WriteByte(ob_5Files_FileRider* r, uint8_t b);
void ob_5Files_ReadInt64(ob_5Files_FileRider* r, int64_t* i);
void ob_5Files_WriteInt64(ob_5Files_FileRider* r, int64_t i);
void ob_5Files_ReadReal64(ob_5Files_FileRider* r, double* f);
void ob_5Files_WriteReal64(ob_5Files_FileRider* r, double f);
void ob_5Files_ReadString(ob_5Files_FileRider* r, char* s, int64_t s_len);
void ob_5Files_WriteString(ob_5Files_FileRider* r, const char* s, int64_t s_len);
void ob_5Files_Files(void);

#endif // FILES_H
