#include <stdint.h>
#include <stddef.h>

#if defined(__APPLE__)
#define OB_SYMBOL(name) __asm__("_ob_" name)
#else
#define OB_SYMBOL(name) __asm__("ob_" name)
#endif

void SetArgs(int64_t argc, char** argv) OB_SYMBOL("4Args_SetArgs");
int64_t Count(void) OB_SYMBOL("4Args_Count");
int64_t Get(int64_t index, char* buf, int64_t buf_len) OB_SYMBOL("4Args_Get");
void Args(void) OB_SYMBOL("Args");

static int64_t args_count = 0;
static char** args_values = NULL;

void SetArgs(int64_t argc, char** argv)
{
    args_count = argc < 0 ? 0 : argc;
    args_values = argv;
}

int64_t Count(void)
{
    return args_count;
}

int64_t Get(int64_t index, char* buf, int64_t buf_len)
{
    int64_t i = 0;

    if (buf != NULL && buf_len > 0) {
        buf[0] = '\0';
    }

    if (index < 0 || index >= args_count || args_values == NULL) {
        return 0;
    }

    char* src = args_values[index];
    if (src == NULL) {
        return 0;
    }

    while (src[i] != '\0') {
        if (buf != NULL && buf_len > 0 && i < buf_len - 1) {
            buf[i] = src[i];
        }
        i++;
    }

    if (buf != NULL && buf_len > 0) {
        if (i < buf_len) {
            buf[i] = '\0';
        }
        else {
            buf[buf_len - 1] = '\0';
        }
    }

    return i;
}

void Args(void) {}
