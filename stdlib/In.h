#ifndef IN_H
#define IN_H

#include <stdbool.h>
#include <stdint.h>

int64_t ob_2In_ReadInt(void);
double ob_2In_ReadReal(void);
char ob_2In_ReadChar(void);
uint8_t ob_2In_ReadByte(void);
int64_t ob_2In_ReadString(char* s, int64_t s_len);
int64_t ob_2In_ReadLine(char* s, int64_t s_len);
void ob_In(void);

extern bool ob_2In_Done;
extern bool ob_2In_Truncated;

#endif /* IN_H */
