#ifndef OUT_H
#define OUT_H

#include <stdint.h>
#include <stdbool.h>


void ob_3Out_String(const char* s, int64_t s_len);
void ob_3Out_StringLen(const char* s, int64_t n, int64_t s_len);
void ob_3Out_Int(int64_t x);
void ob_3Out_IntWidth(int64_t x, int64_t width);
void ob_3Out_Hex(int64_t x, int64_t width);
void ob_3Out_Byte(uint8_t x);
void ob_3Out_Bool(bool b);
void ob_3Out_Real(double x, int64_t digits);
void ob_3Out_Ln(void);
void ob_3Out_Space(void);
void ob_3Out_Char(char ch);
void ob_3Out_3Out(void);

#endif /* OUT_H */
