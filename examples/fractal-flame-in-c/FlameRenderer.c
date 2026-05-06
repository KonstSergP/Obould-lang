#include "FlameRenderer.h"
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <gc.h>

/* Global variables */
static ob_10FlameTypes_Config ob_13FlameRenderer_ActiveConfig;
static ob_10FlameTypes_AffineTransform ob_13FlameRenderer_PreparedAffines[ob_10FlameTypes_MaxAffine];
static ob_10FlameTypes_PHistogram ob_13FlameRenderer_ThreadHistograms[ob_10FlameTypes_MaxThreads];
static bool ob_13FlameRenderer_ThreadFailed[ob_10FlameTypes_MaxThreads];

/* Static function prototypes */
static void ob_13FlameRenderer_ApplyLinear(double x, double y, double weight, ob_10FlameTypes_Point* point);
static void ob_13FlameRenderer_ApplySinusoidal(double x, double y, double weight, ob_10FlameTypes_Point* point);
static void ob_13FlameRenderer_ApplySpherical(double x, double y, double weight, ob_10FlameTypes_Point* point);
static void ob_13FlameRenderer_ApplyHorseshoe(double x, double y, double weight, ob_10FlameTypes_Point* point);
static void ob_13FlameRenderer_ApplySwirl(double x, double y, double weight, ob_10FlameTypes_Point* point);
static void ob_13FlameRenderer_ApplyPolar(double x, double y, double weight, ob_10FlameTypes_Point* point);
static void ob_13FlameRenderer_ApplyHandkerchief(double x, double y, double weight, ob_10FlameTypes_Point* point);
static void ob_13FlameRenderer_ApplyHeart(double x, double y, double weight, ob_10FlameTypes_Point* point);
static void ob_13FlameRenderer_ApplyDisk(double x, double y, double weight, ob_10FlameTypes_Point* point);
static void ob_13FlameRenderer_ApplySpiral(double x, double y, double weight, ob_10FlameTypes_Point* point);
static void ob_13FlameRenderer_ApplyHyperbolic(double x, double y, double weight, ob_10FlameTypes_Point* point);
static void ob_13FlameRenderer_ApplyDiamond(double x, double y, double weight, ob_10FlameTypes_Point* point);
static void ob_13FlameRenderer_ApplyEx(double x, double y, double weight, ob_10FlameTypes_Point* point);
static void ob_13FlameRenderer_ApplyEyefish(double x, double y, double weight, ob_10FlameTypes_Point* point);
static void ob_13FlameRenderer_ApplyTangent(double x, double y, double weight, ob_10FlameTypes_Point* point);
static void ob_13FlameRenderer_ApplyVariations(double x, double y, ob_10FlameTypes_Point* point);
static void ob_13FlameRenderer_SetupTransforms();
static int64_t ob_13FlameRenderer_IterationsForThread(int64_t index);
static void ob_13FlameRenderer_HistPlot(ob_10FlameTypes_PHistogram hist, int64_t x, int64_t y, double r, double g, double b);
static void ob_13FlameRenderer_Draw(ob_10FlameTypes_PHistogram hist, int64_t iterations, int64_t threadIndex);
static void ob_13FlameRenderer_Worker(int64_t index);

/* Function implementations */
static void ob_13FlameRenderer_ApplyLinear(double x, double y, double weight, ob_10FlameTypes_Point* point) {
    (*point).x = ((*point).x + (x * weight));
    (*point).y = ((*point).y + (y * weight));
}

static void ob_13FlameRenderer_ApplySinusoidal(double x, double y, double weight, ob_10FlameTypes_Point* point) {
    (*point).x = ((*point).x + (ob_4Math_Sin(x) * weight));
    (*point).y = ((*point).y + (ob_4Math_Sin(y) * weight));
}

static void ob_13FlameRenderer_ApplySpherical(double x, double y, double weight, ob_10FlameTypes_Point* point) {
    double radius2;
    double invRadius2;
    radius2 = ((x * x) + (y * y));
    if ((radius2 != 0)) {
        invRadius2 = (1 / radius2);
        (*point).x = ((*point).x + ((x * invRadius2) * weight));
        (*point).y = ((*point).y + ((y * invRadius2) * weight));
    };
}

static void ob_13FlameRenderer_ApplyHorseshoe(double x, double y, double weight, ob_10FlameTypes_Point* point) {
    double radius;
    double coef;
    radius = ob_4Math_Sqrt(((x * x) + (y * y)));
    if ((radius != 0)) {
        coef = (1 / radius);
        (*point).x = ((*point).x + ((((x - y) * (x + y)) * coef) * weight));
        (*point).y = ((*point).y + ((((2 * x) * y) * coef) * weight));
    };
}

static void ob_13FlameRenderer_ApplySwirl(double x, double y, double weight, ob_10FlameTypes_Point* point) {
    double radius2;
    radius2 = ((x * x) + (y * y));
    (*point).x = ((*point).x + (((x * ob_4Math_Sin(radius2)) - (y * ob_4Math_Cos(radius2))) * weight));
    (*point).y = ((*point).y + (((x * ob_4Math_Cos(radius2)) + (y * ob_4Math_Sin(radius2))) * weight));
}

static void ob_13FlameRenderer_ApplyPolar(double x, double y, double weight, ob_10FlameTypes_Point* point) {
    double radius;
    double theta;
    radius = ob_4Math_Sqrt(((x * x) + (y * y)));
    theta = ob_4Math_Atan2(x, y);
    (*point).x = ((*point).x + ((theta / ob_4Math_PI) * weight));
    (*point).y = ((*point).y + ((radius - 1) * weight));
}

static void ob_13FlameRenderer_ApplyHandkerchief(double x, double y, double weight, ob_10FlameTypes_Point* point) {
    double radius;
    double theta;
    radius = ob_4Math_Sqrt(((x * x) + (y * y)));
    theta = ob_4Math_Atan2(x, y);
    (*point).x = ((*point).x + ((radius * ob_4Math_Sin((theta + radius))) * weight));
    (*point).y = ((*point).y + ((radius * ob_4Math_Cos((theta - radius))) * weight));
}

static void ob_13FlameRenderer_ApplyHeart(double x, double y, double weight, ob_10FlameTypes_Point* point) {
    double radius;
    double theta;
    radius = ob_4Math_Sqrt(((x * x) + (y * y)));
    theta = ob_4Math_Atan2(x, y);
    (*point).x = ((*point).x + ((radius * ob_4Math_Sin((theta * radius))) * weight));
    (*point).y = ((*point).y - ((radius * ob_4Math_Cos((theta * radius))) * weight));
}

static void ob_13FlameRenderer_ApplyDisk(double x, double y, double weight, ob_10FlameTypes_Point* point) {
    double radius;
    double theta;
    double factor;
    radius = ob_4Math_Sqrt(((x * x) + (y * y)));
    theta = ob_4Math_Atan2(x, y);
    factor = (theta / ob_4Math_PI);
    (*point).x = ((*point).x + ((factor * ob_4Math_Sin((ob_4Math_PI * radius))) * weight));
    (*point).y = ((*point).y + ((factor * ob_4Math_Cos((ob_4Math_PI * radius))) * weight));
}

static void ob_13FlameRenderer_ApplySpiral(double x, double y, double weight, ob_10FlameTypes_Point* point) {
    double radius;
    double theta;
    double invRadius;
    radius = ob_4Math_Sqrt(((x * x) + (y * y)));
    if ((radius != 0)) {
        theta = ob_4Math_Atan2(x, y);
        invRadius = (1 / radius);
        (*point).x = ((*point).x + ((invRadius * (ob_4Math_Cos(theta) + ob_4Math_Sin(radius))) * weight));
        (*point).y = ((*point).y + ((invRadius * (ob_4Math_Sin(theta) - ob_4Math_Cos(radius))) * weight));
    };
}

static void ob_13FlameRenderer_ApplyHyperbolic(double x, double y, double weight, ob_10FlameTypes_Point* point) {
    double radius;
    double theta;
    radius = ob_4Math_Sqrt(((x * x) + (y * y)));
    if ((radius != 0)) {
        theta = ob_4Math_Atan2(x, y);
        (*point).x = ((*point).x + ((ob_4Math_Sin(theta) / radius) * weight));
        (*point).y = ((*point).y + ((radius * ob_4Math_Cos(theta)) * weight));
    };
}

static void ob_13FlameRenderer_ApplyDiamond(double x, double y, double weight, ob_10FlameTypes_Point* point) {
    double radius;
    double theta;
    radius = ob_4Math_Sqrt(((x * x) + (y * y)));
    theta = ob_4Math_Atan2(x, y);
    (*point).x = ((*point).x + ((ob_4Math_Sin(theta) * ob_4Math_Cos(radius)) * weight));
    (*point).y = ((*point).y + ((ob_4Math_Cos(theta) * ob_4Math_Sin(radius)) * weight));
}

static void ob_13FlameRenderer_ApplyEx(double x, double y, double weight, ob_10FlameTypes_Point* point) {
    double radius;
    double theta;
    double p0;
    double p1;
    radius = ob_4Math_Sqrt(((x * x) + (y * y)));
    theta = ob_4Math_Atan2(x, y);
    p0 = ob_4Math_Sin((theta + radius));
    p1 = ob_4Math_Cos((theta - radius));
    (*point).x = ((*point).x + ((radius * (((p0 * p0) * p0) + ((p1 * p1) * p1))) * weight));
    (*point).y = ((*point).y + ((radius * (((p0 * p0) * p0) - ((p1 * p1) * p1))) * weight));
}

static void ob_13FlameRenderer_ApplyEyefish(double x, double y, double weight, ob_10FlameTypes_Point* point) {
    double coef;
    double radius;
    radius = ob_4Math_Sqrt(((x * x) + (y * y)));
    coef = (2 / (radius + 1));
    (*point).x = ((*point).x + ((coef * x) * weight));
    (*point).y = ((*point).y + ((coef * y) * weight));
}

static void ob_13FlameRenderer_ApplyTangent(double x, double y, double weight, ob_10FlameTypes_Point* point) {
    (*point).x = ((*point).x + ((ob_4Math_Sin(x) / ob_4Math_Cos(y)) * weight));
    (*point).y = ((*point).y + (ob_4Math_Tan(y) * weight));
}

static void ob_13FlameRenderer_ApplyVariations(double x, double y, ob_10FlameTypes_Point* point) {
    int64_t i;
    (*point).x = 0;
    (*point).y = 0;
    i = 0;
    while ((i < ob_13FlameRenderer_ActiveConfig.functionCount)) {
        switch (ob_13FlameRenderer_ActiveConfig.functions[i].kind) {
        case ob_10FlameTypes_FuncLinear:
            ob_13FlameRenderer_ApplyLinear(x, y, ob_13FlameRenderer_ActiveConfig.functions[i].weight, point);
            break;
        case ob_10FlameTypes_FuncSinusoidal:
            ob_13FlameRenderer_ApplySinusoidal(x, y, ob_13FlameRenderer_ActiveConfig.functions[i].weight, point);
            break;
        case ob_10FlameTypes_FuncSpherical:
            ob_13FlameRenderer_ApplySpherical(x, y, ob_13FlameRenderer_ActiveConfig.functions[i].weight, point);
            break;
        case ob_10FlameTypes_FuncHorseshoe:
            ob_13FlameRenderer_ApplyHorseshoe(x, y, ob_13FlameRenderer_ActiveConfig.functions[i].weight, point);
            break;
        case ob_10FlameTypes_FuncSwirl:
            ob_13FlameRenderer_ApplySwirl(x, y, ob_13FlameRenderer_ActiveConfig.functions[i].weight, point);
            break;
        case ob_10FlameTypes_FuncPolar:
            ob_13FlameRenderer_ApplyPolar(x, y, ob_13FlameRenderer_ActiveConfig.functions[i].weight, point);
            break;
        case ob_10FlameTypes_FuncHandkerchief:
            ob_13FlameRenderer_ApplyHandkerchief(x, y, ob_13FlameRenderer_ActiveConfig.functions[i].weight, point);
            break;
        case ob_10FlameTypes_FuncHeart:
            ob_13FlameRenderer_ApplyHeart(x, y, ob_13FlameRenderer_ActiveConfig.functions[i].weight, point);
            break;
        case ob_10FlameTypes_FuncDisk:
            ob_13FlameRenderer_ApplyDisk(x, y, ob_13FlameRenderer_ActiveConfig.functions[i].weight, point);
            break;
        case ob_10FlameTypes_FuncSpiral:
            ob_13FlameRenderer_ApplySpiral(x, y, ob_13FlameRenderer_ActiveConfig.functions[i].weight, point);
            break;
        case ob_10FlameTypes_FuncHyperbolic:
            ob_13FlameRenderer_ApplyHyperbolic(x, y, ob_13FlameRenderer_ActiveConfig.functions[i].weight, point);
            break;
        case ob_10FlameTypes_FuncDiamond:
            ob_13FlameRenderer_ApplyDiamond(x, y, ob_13FlameRenderer_ActiveConfig.functions[i].weight, point);
            break;
        case ob_10FlameTypes_FuncEx:
            ob_13FlameRenderer_ApplyEx(x, y, ob_13FlameRenderer_ActiveConfig.functions[i].weight, point);
            break;
        case ob_10FlameTypes_FuncEyefish:
            ob_13FlameRenderer_ApplyEyefish(x, y, ob_13FlameRenderer_ActiveConfig.functions[i].weight, point);
            break;
        case ob_10FlameTypes_FuncTangent:
            ob_13FlameRenderer_ApplyTangent(x, y, ob_13FlameRenderer_ActiveConfig.functions[i].weight, point);
            break;
        };
        i = (i + 1);
    };
}

static void ob_13FlameRenderer_SetupTransforms() {
    ob_6Random_Generator rng;
    int64_t i;
    ob_6Random_GenSeed(&(rng), ob_13FlameRenderer_ActiveConfig.seed);
    i = 0;
    while ((i < ob_13FlameRenderer_ActiveConfig.affineCount)) {
        ob_13FlameRenderer_PreparedAffines[i].a = ob_13FlameRenderer_ActiveConfig.affine[i].a;
        ob_13FlameRenderer_PreparedAffines[i].b = ob_13FlameRenderer_ActiveConfig.affine[i].b;
        ob_13FlameRenderer_PreparedAffines[i].c = ob_13FlameRenderer_ActiveConfig.affine[i].c;
        ob_13FlameRenderer_PreparedAffines[i].d = ob_13FlameRenderer_ActiveConfig.affine[i].d;
        ob_13FlameRenderer_PreparedAffines[i].e = ob_13FlameRenderer_ActiveConfig.affine[i].e;
        ob_13FlameRenderer_PreparedAffines[i].f = ob_13FlameRenderer_ActiveConfig.affine[i].f;
        ob_13FlameRenderer_PreparedAffines[i].red = ob_6Random_GenBetween(&(rng), 0, 255);
        ob_13FlameRenderer_PreparedAffines[i].green = ob_6Random_GenBetween(&(rng), 0, 255);
        ob_13FlameRenderer_PreparedAffines[i].blue = ob_6Random_GenBetween(&(rng), 0, 255);
        i = (i + 1);
    };
}

static int64_t ob_13FlameRenderer_IterationsForThread(int64_t index) {
    int64_t base;
    int64_t extra;
    int64_t result;
    base = (ob_13FlameRenderer_ActiveConfig.iterations / ob_13FlameRenderer_ActiveConfig.threads);
    extra = (ob_13FlameRenderer_ActiveConfig.iterations % ob_13FlameRenderer_ActiveConfig.threads);
    result = base;
    if ((index < extra)) {
        result = (result + 1);
    };
    return result;
}

static void ob_13FlameRenderer_HistPlot(ob_10FlameTypes_PHistogram hist, int64_t x, int64_t y, double r, double g, double b) {
    double next;
    if (((((x >= 0) && (x < ob_13FlameRenderer_ActiveConfig.width)) && (y >= 0)) && (y < ob_13FlameRenderer_ActiveConfig.height))) {
        if ((hist->pixels[y][x].counter == 0)) {
            hist->pixels[y][x].red = r;
            hist->pixels[y][x].green = g;
            hist->pixels[y][x].blue = b;
        } else {
            next = ((double)((hist->pixels[y][x].counter + 1)));
            hist->pixels[y][x].red = (((hist->pixels[y][x].red * ((double)(hist->pixels[y][x].counter))) + r) / next);
            hist->pixels[y][x].green = (((hist->pixels[y][x].green * ((double)(hist->pixels[y][x].counter))) + g) / next);
            hist->pixels[y][x].blue = (((hist->pixels[y][x].blue * ((double)(hist->pixels[y][x].counter))) + b) / next);
        };
        hist->pixels[y][x].counter = (hist->pixels[y][x].counter + 1);
    };
}

static void ob_13FlameRenderer_Draw(ob_10FlameTypes_PHistogram hist, int64_t iterations, int64_t threadIndex) {
    ob_6Random_Generator rng;
    double x;
    double y;
    double newX;
    double newY;
    double xMin;
    double xMax;
    double yMin;
    double yMax;
    double rotX;
    double rotY;
    double newRotX;
    double newRotY;
    double cosTheta;
    double sinTheta;
    ob_10FlameTypes_Point transformed;
    int64_t affineIndex;
    int64_t step;
    int64_t symmetryIndex;
    int64_t pixelX;
    int64_t pixelY;
    ob_10FlameTypes_AffineTransform affine;
    if ((hist != NULL)) {
        if ((ob_13FlameRenderer_ActiveConfig.width > ob_13FlameRenderer_ActiveConfig.height)) {
            xMin = (-(((double)(ob_13FlameRenderer_ActiveConfig.width)) / ((double)(ob_13FlameRenderer_ActiveConfig.height))));
            xMax = (((double)(ob_13FlameRenderer_ActiveConfig.width)) / ((double)(ob_13FlameRenderer_ActiveConfig.height)));
            yMin = (-1);
            yMax = 1;
        } else {
            xMin = (-1);
            xMax = 1;
            yMin = (-(((double)(ob_13FlameRenderer_ActiveConfig.height)) / ((double)(ob_13FlameRenderer_ActiveConfig.width))));
            yMax = (((double)(ob_13FlameRenderer_ActiveConfig.height)) / ((double)(ob_13FlameRenderer_ActiveConfig.width)));
        };
        ob_6Random_GenSeed(&(rng), ((ob_13FlameRenderer_ActiveConfig.seed + (threadIndex * 104729)) + 17));
        x = ob_6Random_GenBetween(&(rng), xMin, xMax);
        y = ob_6Random_GenBetween(&(rng), yMin, yMax);
        cosTheta = ob_4Math_Cos(((2 * ob_4Math_PI) / ((double)(ob_13FlameRenderer_ActiveConfig.symmetry))));
        sinTheta = ob_4Math_Sin(((2 * ob_4Math_PI) / ((double)(ob_13FlameRenderer_ActiveConfig.symmetry))));
        step = (0 - ob_10FlameTypes_WarmupIterations);
        while ((step < iterations)) {
            affineIndex = ob_6Random_GenIntN(&(rng), ob_13FlameRenderer_ActiveConfig.affineCount);
            affine = ob_13FlameRenderer_PreparedAffines[affineIndex];
            newX = (((affine.a * x) + (affine.b * y)) + affine.c);
            newY = (((affine.d * x) + (affine.e * y)) + affine.f);
            x = newX;
            y = newY;
            ob_13FlameRenderer_ApplyVariations(x, y, &(transformed));
            x = transformed.x;
            y = transformed.y;
            if ((step >= 0)) {
                rotX = x;
                rotY = y;
                symmetryIndex = 0;
                while ((symmetryIndex < ob_13FlameRenderer_ActiveConfig.symmetry)) {
                    pixelX = ((int64_t)((((rotX - xMin) / (xMax - xMin)) * ((double)(ob_13FlameRenderer_ActiveConfig.width)))));
                    pixelY = ((int64_t)((((rotY - yMin) / (yMax - yMin)) * ((double)(ob_13FlameRenderer_ActiveConfig.height)))));
                    newRotX = ((rotX * cosTheta) - (rotY * sinTheta));
                    newRotY = ((rotX * sinTheta) + (rotY * cosTheta));
                    rotX = newRotX;
                    rotY = newRotY;
                    if (((((pixelX >= 0) && (pixelX < ob_13FlameRenderer_ActiveConfig.width)) && (pixelY >= 0)) && (pixelY < ob_13FlameRenderer_ActiveConfig.height))) {
                        ob_13FlameRenderer_HistPlot(hist, pixelX, pixelY, affine.red, affine.green, affine.blue);
                    };
                    symmetryIndex = (symmetryIndex + 1);
                };
            };
            step = (step + 1);
        };
    };
}

static void ob_13FlameRenderer_Worker(int64_t index) {
    ob_10FlameTypes_PHistogram hist;
    hist = (ob_10FlameTypes_Histogram*)GC_MALLOC(sizeof(ob_10FlameTypes_Histogram));
    ob_13FlameRenderer_ThreadHistograms[index] = hist;
    ob_13FlameRenderer_ThreadFailed[index] = (hist == NULL);
    if ((hist != NULL)) {
        ob_13FlameRenderer_Draw(hist, ob_13FlameRenderer_IterationsForThread(index), index);
    };
}

ob_6Images_Image ob_13FlameRenderer_Render(ob_10FlameTypes_Config config) {
    ob_6Images_Image image;
    ob_10FlameTypes_PHistogram mainHist;
    ob_7Threads_Thread handles[ob_10FlameTypes_MaxThreads];
    int64_t index;
    int64_t x;
    int64_t y;
    bool allStarted;
    double totalCnt;
    double maxNormal;
    double normal;
    double power;
    int64_t r;
    int64_t g;
    int64_t b;
    ob_13FlameRenderer_ActiveConfig = config;
    ob_13FlameRenderer_SetupTransforms();
    mainHist = (ob_10FlameTypes_Histogram*)GC_MALLOC(sizeof(ob_10FlameTypes_Histogram));
    for (index = 0; index <= (ob_13FlameRenderer_ActiveConfig.threads - 1); index += 1) {
        handles[index] = ob_7Threads_StartI64(ob_13FlameRenderer_Worker, index);
    };
    for (index = 0; index <= (ob_13FlameRenderer_ActiveConfig.threads - 1); index += 1) {
        ob_7Threads_Join(handles[index]);
        if ((!ob_13FlameRenderer_ThreadFailed[index])) {
            for (y = 0; y <= (ob_13FlameRenderer_ActiveConfig.height - 1); y += 1) {
                for (x = 0; x <= (ob_13FlameRenderer_ActiveConfig.width - 1); x += 1) {
                    if ((ob_13FlameRenderer_ThreadHistograms[index]->pixels[y][x].counter > 0)) {
                        if ((mainHist->pixels[y][x].counter == 0)) {
                            mainHist->pixels[y][x].red = ob_13FlameRenderer_ThreadHistograms[index]->pixels[y][x].red;
                            mainHist->pixels[y][x].green = ob_13FlameRenderer_ThreadHistograms[index]->pixels[y][x].green;
                            mainHist->pixels[y][x].blue = ob_13FlameRenderer_ThreadHistograms[index]->pixels[y][x].blue;
                        } else {
                            totalCnt = ((double)((mainHist->pixels[y][x].counter + ob_13FlameRenderer_ThreadHistograms[index]->pixels[y][x].counter)));
                            mainHist->pixels[y][x].red = (((mainHist->pixels[y][x].red * ((double)(mainHist->pixels[y][x].counter))) + (ob_13FlameRenderer_ThreadHistograms[index]->pixels[y][x].red * ((double)(ob_13FlameRenderer_ThreadHistograms[index]->pixels[y][x].counter)))) / totalCnt);
                            mainHist->pixels[y][x].green = (((mainHist->pixels[y][x].green * ((double)(mainHist->pixels[y][x].counter))) + (ob_13FlameRenderer_ThreadHistograms[index]->pixels[y][x].green * ((double)(ob_13FlameRenderer_ThreadHistograms[index]->pixels[y][x].counter)))) / totalCnt);
                            mainHist->pixels[y][x].blue = (((mainHist->pixels[y][x].blue * ((double)(mainHist->pixels[y][x].counter))) + (ob_13FlameRenderer_ThreadHistograms[index]->pixels[y][x].blue * ((double)(ob_13FlameRenderer_ThreadHistograms[index]->pixels[y][x].counter)))) / totalCnt);
                        };
                        mainHist->pixels[y][x].counter = (mainHist->pixels[y][x].counter + ob_13FlameRenderer_ThreadHistograms[index]->pixels[y][x].counter);
                    };
                };
            };
        };
    };
    if (ob_13FlameRenderer_ActiveConfig.enableGamma) {
        ob_3Out_String("Applying gamma correction...\n", 30);
        maxNormal = 0;
        for (y = 0; y <= (ob_13FlameRenderer_ActiveConfig.height - 1); y += 1) {
            for (x = 0; x <= (ob_13FlameRenderer_ActiveConfig.width - 1); x += 1) {
                if ((mainHist->pixels[y][x].counter > 0)) {
                    normal = ob_4Math_Log10(((double)(mainHist->pixels[y][x].counter)));
                    if ((normal > maxNormal)) {
                        maxNormal = normal;
                    };
                };
            };
        };
        if ((maxNormal > 0)) {
            for (y = 0; y <= (ob_13FlameRenderer_ActiveConfig.height - 1); y += 1) {
                for (x = 0; x <= (ob_13FlameRenderer_ActiveConfig.width - 1); x += 1) {
                    if ((mainHist->pixels[y][x].counter > 0)) {
                        normal = (ob_4Math_Log10(((double)(mainHist->pixels[y][x].counter))) / maxNormal);
                        power = ob_4Math_Pow(normal, (1 / ob_13FlameRenderer_ActiveConfig.gamma));
                        mainHist->pixels[y][x].red = (mainHist->pixels[y][x].red * power);
                        mainHist->pixels[y][x].green = (mainHist->pixels[y][x].green * power);
                        mainHist->pixels[y][x].blue = (mainHist->pixels[y][x].blue * power);
                    };
                };
            };
        };
    };
    ob_3Out_String("Creating PNG image...\n", 23);
    image = ob_6Images_Create(ob_13FlameRenderer_ActiveConfig.width, ob_13FlameRenderer_ActiveConfig.height);
    for (y = 0; y <= (ob_13FlameRenderer_ActiveConfig.height - 1); y += 1) {
        for (x = 0; x <= (ob_13FlameRenderer_ActiveConfig.width - 1); x += 1) {
            r = ((int64_t)(mainHist->pixels[y][x].red));
            g = ((int64_t)(mainHist->pixels[y][x].green));
            b = ((int64_t)(mainHist->pixels[y][x].blue));
            if ((r > 255)) {
                r = 255;
            };
            if ((r < 0)) {
                r = 0;
            };
            if ((g > 255)) {
                g = 255;
            };
            if ((g < 0)) {
                g = 0;
            };
            if ((b > 255)) {
                b = 255;
            };
            if ((b < 0)) {
                b = 0;
            };
            ob_6Images_SetRGB(image, x, y, r, g, b);
        };
    };
    return image;
}

extern void ob_10FlameTypes_10FlameTypes(void);
extern void ob_6Images_6Images(void);
extern void ob_4Math_4Math(void);
extern void ob_3Out_3Out(void);
extern void ob_6Random_6Random(void);
extern void ob_7Threads_7Threads(void);
void ob_13FlameRenderer_13FlameRenderer(void) {
    static int _initialized = 0;
    if (_initialized) return;
    _initialized = 1;
    ob_10FlameTypes_10FlameTypes();
    ob_6Images_6Images();
    ob_4Math_4Math();
    ob_3Out_3Out();
    ob_6Random_6Random();
    ob_7Threads_7Threads();
}

