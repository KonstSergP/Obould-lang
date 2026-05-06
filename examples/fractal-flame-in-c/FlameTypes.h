#ifndef FLAMETYPES_H
#define FLAMETYPES_H

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

/* Constants */
#define ob_10FlameTypes_MaxAffine (16)
#define ob_10FlameTypes_MaxFunctions (16)
#define ob_10FlameTypes_MaxThreads (16)
#define ob_10FlameTypes_MaxPath (256)
#define ob_10FlameTypes_MaxCliText (512)
#define ob_10FlameTypes_MaxName (64)
#define ob_10FlameTypes_WarmupIterations (20)
#define ob_10FlameTypes_FuncInvalid (0)
#define ob_10FlameTypes_FuncLinear (1)
#define ob_10FlameTypes_FuncSinusoidal (2)
#define ob_10FlameTypes_FuncSpherical (3)
#define ob_10FlameTypes_FuncHorseshoe (4)
#define ob_10FlameTypes_FuncSwirl (5)
#define ob_10FlameTypes_FuncPolar (6)
#define ob_10FlameTypes_FuncHandkerchief (7)
#define ob_10FlameTypes_FuncHeart (8)
#define ob_10FlameTypes_FuncDisk (9)
#define ob_10FlameTypes_FuncSpiral (10)
#define ob_10FlameTypes_FuncHyperbolic (11)
#define ob_10FlameTypes_FuncDiamond (12)
#define ob_10FlameTypes_FuncEx (13)
#define ob_10FlameTypes_FuncEyefish (14)
#define ob_10FlameTypes_FuncTangent (15)
#define ob_10FlameTypes_MaxWidth (3840)
#define ob_10FlameTypes_MaxHeight (2160)

/* Forward declarations */
typedef struct ob_10FlameTypes_Point ob_10FlameTypes_Point;
typedef struct ob_10FlameTypes_AffineParams ob_10FlameTypes_AffineParams;
typedef struct ob_10FlameTypes_AffineTransform ob_10FlameTypes_AffineTransform;
typedef struct ob_10FlameTypes_TransformSpec ob_10FlameTypes_TransformSpec;
typedef struct ob_10FlameTypes_Config ob_10FlameTypes_Config;
typedef struct ob_10FlameTypes_HistPixel ob_10FlameTypes_HistPixel;
typedef struct ob_10FlameTypes_Histogram ob_10FlameTypes_Histogram;

/* Types */
struct ob_10FlameTypes_Point {
    StructDescriptor* _desc;
    double x;
    double y;
};
struct ob_10FlameTypes_AffineParams {
    StructDescriptor* _desc;
    double a;
    double b;
    double c;
    double d;
    double e;
    double f;
};
struct ob_10FlameTypes_AffineTransform {
    StructDescriptor* _desc;
    double a;
    double b;
    double c;
    double d;
    double e;
    double f;
    double red;
    double green;
    double blue;
};
struct ob_10FlameTypes_TransformSpec {
    StructDescriptor* _desc;
    int64_t kind;
    double weight;
};
struct ob_10FlameTypes_Config {
    StructDescriptor* _desc;
    int64_t width;
    int64_t height;
    int64_t seed;
    int64_t iterations;
    int64_t threads;
    int64_t symmetry;
    bool enableGamma;
    double gamma;
    char outputPath[MaxPath];
    int64_t affineCount;
    int64_t functionCount;
    ob_10FlameTypes_AffineParams affine[MaxAffine];
    ob_10FlameTypes_TransformSpec functions[MaxFunctions];
};
struct ob_10FlameTypes_HistPixel {
    StructDescriptor* _desc;
    double red;
    double green;
    double blue;
    int64_t counter;
};
struct ob_10FlameTypes_Histogram {
    StructDescriptor* _desc;
    ob_10FlameTypes_HistPixel pixels[MaxWidth][MaxHeight];
};
typedef ob_10FlameTypes_Histogram* ob_10FlameTypes_PHistogram;

/* Struct descriptors */
extern StructDescriptor ob_10FlameTypes_Point_desc;
extern StructDescriptor ob_10FlameTypes_AffineParams_desc;
extern StructDescriptor ob_10FlameTypes_AffineTransform_desc;
extern StructDescriptor ob_10FlameTypes_TransformSpec_desc;
extern StructDescriptor ob_10FlameTypes_Config_desc;
extern StructDescriptor ob_10FlameTypes_HistPixel_desc;
extern StructDescriptor ob_10FlameTypes_Histogram_desc;

void ob_10FlameTypes_FlameTypes(void);

#endif /* FLAMETYPES_H */
