#ifndef FLAMERENDERER_H
#define FLAMERENDERER_H

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

#include "FlameTypes.h"
#include "Images.h"
#include "Math.h"
#include "Out.h"
#include "Random.h"
#include "Threads.h"

/* Function prototypes */
ob_6Images_Image ob_13FlameRenderer_Render(ob_10FlameTypes_Config config);

void ob_13FlameRenderer_FlameRenderer(void);

#endif /* FLAMERENDERER_H */
