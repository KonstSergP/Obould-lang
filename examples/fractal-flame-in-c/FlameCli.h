#ifndef FLAMECLI_H
#define FLAMECLI_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* RTTI Support */
#ifndef STRUCT_DESCRIPTOR_DEFINED
#define STRUCT_DESCRIPTOR_DEFINED
typedef struct StructDescriptor {
    const char* name;
    int level;
    struct StructDescriptor** ancestors;
} StructDescriptor;
#endif

#include "Args.h"
#include "FlameTypes.h"
#include "Out.h"
#include "Parse.h"
#include "Strings.h"

/* Function prototypes */
void ob_8FlameCli_PrintHelp();
bool ob_8FlameCli_LoadConfig(ob_10FlameTypes_Config* config);

void ob_8FlameCli_FlameCli(void);

#endif /* FLAMECLI_H */
