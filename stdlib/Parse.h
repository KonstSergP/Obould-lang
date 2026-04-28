#ifndef PARSE_H
#define PARSE_H

#include <stdbool.h>
#include <stdint.h>

bool ob_5Parse_Int(const char* text, int64_t* value, int64_t text_len);
bool ob_5Parse_Real(const char* text, double* value, int64_t text_len);
void ob_5Parse_5Parse(void);

#endif /* PARSE_H */
