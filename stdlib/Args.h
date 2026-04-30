#ifndef ARGS_H
#define ARGS_H

#include <stdint.h>

void ob_4Args_SetArgs(int64_t argc, char** argv);
int64_t ob_4Args_Count(void);
int64_t ob_4Args_Get(int64_t index, char* buf, int64_t buf_len);
void ob_4Args_4Args(void);

#endif /* ARGS_H */
