#include "FlameTypes.h"
#include <string.h>
#include <stdlib.h>
#include <assert.h>

/* RTTI helper functions */
#ifndef RTTI_HELPERS_DEFINED
#define RTTI_HELPERS_DEFINED
static StructDescriptor* createStructDescriptor(
    const char* name,
    StructDescriptor* parent
) {
    StructDescriptor* desc = (StructDescriptor*)malloc(sizeof(StructDescriptor));
    desc->name = name;
    if (parent == NULL) {
        desc->level = 0;
        desc->ancestors = NULL;
    } else {
        desc->level = parent->level + 1;
        desc->ancestors = (StructDescriptor**)malloc(sizeof(StructDescriptor*) * desc->level);
        for (int i = 0; i < parent->level; i++) {
            desc->ancestors[i] = parent->ancestors[i];
        }
        desc->ancestors[desc->level - 1] = parent;
    }
    return desc;
}

static int isInstanceOf(StructDescriptor* actual, StructDescriptor* expected) {
    if (actual == expected) return 1;
    if (actual->level <= expected->level) return 0;
    return actual->ancestors[expected->level] == expected;
}
#endif

/* Struct descriptors */
StructDescriptor ob_10FlameTypes_Point_desc;
StructDescriptor ob_10FlameTypes_AffineParams_desc;
StructDescriptor ob_10FlameTypes_AffineTransform_desc;
StructDescriptor ob_10FlameTypes_TransformSpec_desc;
StructDescriptor ob_10FlameTypes_Config_desc;
StructDescriptor ob_10FlameTypes_HistPixel_desc;
StructDescriptor ob_10FlameTypes_Histogram_desc;

static void _init_descriptors(void) {
    static int initialized = 0;
    if (initialized) return;
    initialized = 1;
    {
        StructDescriptor* d = createStructDescriptor("ob_10FlameTypes_Point", NULL);
        ob_10FlameTypes_Point_desc = *d;
        free(d);
    }
    {
        StructDescriptor* d = createStructDescriptor("ob_10FlameTypes_AffineParams", NULL);
        ob_10FlameTypes_AffineParams_desc = *d;
        free(d);
    }
    {
        StructDescriptor* d = createStructDescriptor("ob_10FlameTypes_AffineTransform", NULL);
        ob_10FlameTypes_AffineTransform_desc = *d;
        free(d);
    }
    {
        StructDescriptor* d = createStructDescriptor("ob_10FlameTypes_TransformSpec", NULL);
        ob_10FlameTypes_TransformSpec_desc = *d;
        free(d);
    }
    {
        StructDescriptor* d = createStructDescriptor("ob_10FlameTypes_Config", NULL);
        ob_10FlameTypes_Config_desc = *d;
        free(d);
    }
    {
        StructDescriptor* d = createStructDescriptor("ob_10FlameTypes_HistPixel", NULL);
        ob_10FlameTypes_HistPixel_desc = *d;
        free(d);
    }
    {
        StructDescriptor* d = createStructDescriptor("ob_10FlameTypes_Histogram", NULL);
        ob_10FlameTypes_Histogram_desc = *d;
        free(d);
    }
}

/* Function implementations */
void ob_10FlameTypes_10FlameTypes(void) {
    static int _initialized = 0;
    if (_initialized) return;
    _initialized = 1;
}

