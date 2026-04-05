#ifndef OUT_H
#define OUT_H

#include <stdint.h>
#include <stdbool.h>


void ob_3Out_WriteString(const char* s, int64_t s_len);
void ob_3Out_WriteStringLen(const char* s, int64_t n, int64_t s_len);
void ob_3Out_WriteInt(int64_t x);
void ob_3Out_WriteIntWidth(int64_t x, int64_t width);
void ob_3Out_WriteHex(int64_t x, int64_t width);
void ob_3Out_WriteByte(uint8_t x);
void ob_3Out_WriteBool(bool b);
void ob_3Out_WriteReal(double x, int64_t digits);
void ob_3Out_WriteLn(void);
void ob_3Out_WriteSpace(void);
void ob_3Out_WriteChar(char ch);
void ob_Out(void);

#endif /* OUT_H */
