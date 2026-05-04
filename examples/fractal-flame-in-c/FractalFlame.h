#ifndef FRACTALFLAME_H
#define FRACTALFLAME_H

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
#include "FlameCli.h"
#include "FlameRenderer.h"
#include "FlameTypes.h"
#include "Images.h"
#include "Out.h"

void ob_12FractalFlame_FractalFlame(void);

#endif /* FRACTALFLAME_H */
