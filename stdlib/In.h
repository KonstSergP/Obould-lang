#ifndef IN_H
#define IN_H

#include <stdbool.h>
#include <stdint.h>

int64_t ob_2In_Int(void);
double ob_2In_Real(void);
char ob_2In_Char(void);
uint8_t ob_2In_Byte(void);
int64_t ob_2In_String(char* s, int64_t s_len);
int64_t ob_2In_Line(char* s, int64_t s_len);
void ob_2In_2In(void);

extern bool ob_2In_Done;
extern bool ob_2In_Truncated;

#endif /* IN_H */
