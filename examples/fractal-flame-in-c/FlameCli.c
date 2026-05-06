#include "FlameCli.h"
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <gc.h>

/* Static function prototypes */
static void ob_8FlameCli_PrintError(char* message, int64_t message_len);
static bool ob_8FlameCli_RequireValue(int64_t argc, int64_t index, char* option, int64_t option_len);
static int64_t ob_8FlameCli_ResolveFunctionName(char* name, int64_t name_len);
static void ob_8FlameCli_SetDefaults(ob_10FlameTypes_Config* config);
static bool ob_8FlameCli_ParseAffineGroup(char* text, int64_t start, int64_t finish, ob_10FlameTypes_AffineParams* affine, int64_t text_len);
static bool ob_8FlameCli_ParseAffineList(char* text, ob_10FlameTypes_Config* config, int64_t text_len);
static bool ob_8FlameCli_ParseFunctionItem(char* text, int64_t start, int64_t finish, ob_10FlameTypes_TransformSpec* spec, int64_t text_len);
static bool ob_8FlameCli_ParseFunctionList(char* text, ob_10FlameTypes_Config* config, int64_t text_len);
static bool ob_8FlameCli_Validate(ob_10FlameTypes_Config config);

/* Function implementations */
static void ob_8FlameCli_PrintError(char* message, int64_t message_len) {
    ob_3Out_String("Error: ", 8);
    ob_3Out_String(message, message_len);
    ob_3Out_Ln();
}

static bool ob_8FlameCli_RequireValue(int64_t argc, int64_t index, char* option, int64_t option_len) {
    bool result;
    result = ((index + 1) < argc);
    if ((!result)) {
        ob_3Out_String("Error: option requires a value: ", 33);
        ob_3Out_String(option, option_len);
        ob_3Out_Ln();
    };
    return result;
}

static int64_t ob_8FlameCli_ResolveFunctionName(char* name, int64_t name_len) {
    int64_t result;
    result = ob_10FlameTypes_FuncInvalid;
    if ((name && strcmp(name, "linear") == 0)) {
        result = ob_10FlameTypes_FuncLinear;
    } else if ((name && strcmp(name, "sinusoidal") == 0)) {
        result = ob_10FlameTypes_FuncSinusoidal;
    } else if ((name && strcmp(name, "spherical") == 0)) {
        result = ob_10FlameTypes_FuncSpherical;
    } else if ((name && strcmp(name, "horseshoe") == 0)) {
        result = ob_10FlameTypes_FuncHorseshoe;
    } else if ((name && strcmp(name, "swirl") == 0)) {
        result = ob_10FlameTypes_FuncSwirl;
    } else if ((name && strcmp(name, "polar") == 0)) {
        result = ob_10FlameTypes_FuncPolar;
    } else if ((name && strcmp(name, "handkerchief") == 0)) {
        result = ob_10FlameTypes_FuncHandkerchief;
    } else if ((name && strcmp(name, "heart") == 0)) {
        result = ob_10FlameTypes_FuncHeart;
    } else if ((name && strcmp(name, "disk") == 0)) {
        result = ob_10FlameTypes_FuncDisk;
    } else if ((name && strcmp(name, "spiral") == 0)) {
        result = ob_10FlameTypes_FuncSpiral;
    } else if ((name && strcmp(name, "hyperbolic") == 0)) {
        result = ob_10FlameTypes_FuncHyperbolic;
    } else if ((name && strcmp(name, "diamond") == 0)) {
        result = ob_10FlameTypes_FuncDiamond;
    } else if ((name && strcmp(name, "ex") == 0)) {
        result = ob_10FlameTypes_FuncEx;
    } else if ((name && strcmp(name, "eyefish") == 0)) {
        result = ob_10FlameTypes_FuncEyefish;
    } else if ((name && strcmp(name, "tangent") == 0)) {
        result = ob_10FlameTypes_FuncTangent;
    };
    return result;
}

static void ob_8FlameCli_SetDefaults(ob_10FlameTypes_Config* config) {
    (*config).width = 1920;
    (*config).height = 1080;
    (*config).seed = 5;
    (*config).iterations = 1000000;
    (*config).threads = 1;
    (*config).symmetry = 1;
    (*config).enableGamma = false;
    (*config).gamma = 2.2;
    strncpy((*config).outputPath, "result.png", sizeof((*config).outputPath));
    (*config).functionCount = 1;
    (*config).functions[0].kind = ob_10FlameTypes_FuncLinear;
    (*config).functions[0].weight = 1;
    (*config).affineCount = 3;
    (*config).affine[0].a = 0.5;
    (*config).affine[0].b = 0;
    (*config).affine[0].c = 0;
    (*config).affine[0].d = 0;
    (*config).affine[0].e = 0.5;
    (*config).affine[0].f = 0;
    (*config).affine[1].a = 0.5;
    (*config).affine[1].b = 0;
    (*config).affine[1].c = 0.5;
    (*config).affine[1].d = 0;
    (*config).affine[1].e = 0.5;
    (*config).affine[1].f = 0;
    (*config).affine[2].a = 0.5;
    (*config).affine[2].b = 0;
    (*config).affine[2].c = 0.25;
    (*config).affine[2].d = 0;
    (*config).affine[2].e = 0.5;
    (*config).affine[2].f = 0.5;
}

static bool ob_8FlameCli_ParseAffineGroup(char* text, int64_t start, int64_t finish, ob_10FlameTypes_AffineParams* affine, int64_t text_len) {
    int64_t fieldIndex;
    int64_t tokenStart;
    int64_t pos;
    char token[ob_10FlameTypes_MaxCliText];
    double value;
    bool result;
    bool tokenOk;
    fieldIndex = 0;
    tokenStart = start;
    pos = start;
    result = true;
    while (((pos <= finish) && result)) {
        if (((pos == finish) || (text[pos] == ','))) {
            ob_7Strings_CopyRange(text, tokenStart, pos, token, text_len, 512);
            tokenOk = ob_5Parse_Real(token, &(value), 512);
            if (tokenOk) {
                if ((fieldIndex < 6)) {
                    switch (fieldIndex) {
                    case 0:
                        (*affine).a = value;
                        break;
                    case 1:
                        (*affine).b = value;
                        break;
                    case 2:
                        (*affine).c = value;
                        break;
                    case 3:
                        (*affine).d = value;
                        break;
                    case 4:
                        (*affine).e = value;
                        break;
                    case 5:
                        (*affine).f = value;
                        break;
                    };
                } else {
                    result = false;
                };
                if (result) {
                    fieldIndex = (fieldIndex + 1);
                    tokenStart = (pos + 1);
                };
            } else {
                result = false;
            };
        };
        pos = (pos + 1);
    };
    if (result) {
        result = (fieldIndex == 6);
    };
    return result;
}

static bool ob_8FlameCli_ParseAffineList(char* text, ob_10FlameTypes_Config* config, int64_t text_len) {
    int64_t start;
    int64_t pos;
    int64_t textLen;
    bool result;
    (*config).affineCount = 0;
    textLen = ob_7Strings_Length(text, text_len);
    result = (textLen > 0);
    if (result) {
        start = 0;
        pos = 0;
        while (((pos <= textLen) && result)) {
            if (((pos == textLen) || (text[pos] == '/'))) {
                if (((*config).affineCount < ob_10FlameTypes_MaxAffine)) {
                    result = ob_8FlameCli_ParseAffineGroup(text, start, pos, &((*config).affine[(*config).affineCount]), text_len);
                    if (result) {
                        (*config).affineCount = ((*config).affineCount + 1);
                        start = (pos + 1);
                    };
                } else {
                    result = false;
                };
            };
            pos = (pos + 1);
        };
    };
    if (result) {
        result = ((*config).affineCount > 0);
    };
    return result;
}

static bool ob_8FlameCli_ParseFunctionItem(char* text, int64_t start, int64_t finish, ob_10FlameTypes_TransformSpec* spec, int64_t text_len) {
    int64_t colon;
    char nameBuf[ob_10FlameTypes_MaxCliText];
    char weightBuf[ob_10FlameTypes_MaxCliText];
    double weight;
    int64_t kind;
    bool result;
    colon = ob_7Strings_Find(text, ':', start, text_len);
    result = ((colon >= 0) && (colon < finish));
    if (result) {
        ob_7Strings_CopyRange(text, start, colon, nameBuf, text_len, 512);
        ob_7Strings_CopyRange(text, (colon + 1), finish, weightBuf, text_len, 512);
        kind = ob_8FlameCli_ResolveFunctionName(nameBuf, 512);
        result = (kind != ob_10FlameTypes_FuncInvalid);
        if (result) {
            result = ob_5Parse_Real(weightBuf, &(weight), 512);
            if (result) {
                (*spec).kind = kind;
                (*spec).weight = weight;
            };
        };
    };
    return result;
}

static bool ob_8FlameCli_ParseFunctionList(char* text, ob_10FlameTypes_Config* config, int64_t text_len) {
    int64_t start;
    int64_t pos;
    int64_t textLen;
    bool result;
    (*config).functionCount = 0;
    textLen = ob_7Strings_Length(text, text_len);
    result = (textLen > 0);
    if (result) {
        start = 0;
        pos = 0;
        while (((pos <= textLen) && result)) {
            if (((pos == textLen) || (text[pos] == ','))) {
                if (((*config).functionCount < ob_10FlameTypes_MaxFunctions)) {
                    result = ob_8FlameCli_ParseFunctionItem(text, start, pos, &((*config).functions[(*config).functionCount]), text_len);
                    if (result) {
                        (*config).functionCount = ((*config).functionCount + 1);
                        start = (pos + 1);
                    };
                } else {
                    result = false;
                };
            };
            pos = (pos + 1);
        };
    };
    if (result) {
        result = ((*config).functionCount > 0);
    };
    return result;
}

static bool ob_8FlameCli_Validate(ob_10FlameTypes_Config config) {
    bool result;
    result = true;
    if ((config.width <= 0)) {
        ob_8FlameCli_PrintError("width must be > 0", 18);
        result = false;
    } else if ((config.height <= 0)) {
        ob_8FlameCli_PrintError("height must be > 0", 19);
        result = false;
    } else if ((config.iterations <= 0)) {
        ob_8FlameCli_PrintError("iterations must be > 0", 23);
        result = false;
    } else if (((config.threads <= 0) || (config.threads > ob_10FlameTypes_MaxThreads))) {
        ob_8FlameCli_PrintError("threads must be between 1 and MaxThreads", 41);
        result = false;
    } else if ((config.affineCount <= 0)) {
        ob_8FlameCli_PrintError("at least one affine transform is required", 42);
        result = false;
    } else if ((config.functionCount <= 0)) {
        ob_8FlameCli_PrintError("at least one variation is required", 35);
        result = false;
    } else if ((config.symmetry <= 0)) {
        ob_8FlameCli_PrintError("symmetry must be >= 1", 22);
        result = false;
    } else if ((config.enableGamma && (config.gamma <= 0))) {
        ob_8FlameCli_PrintError("gamma must be > 0 when gamma correction is enabled", 51);
        result = false;
    } else if ((!ob_7Strings_EndsWith(config.outputPath, ".png", 256, 5))) {
        ob_8FlameCli_PrintError("output path must end with .png", 31);
        result = false;
    };
    return result;
}

void ob_8FlameCli_PrintHelp() {
    ob_3Out_String("Fractal flame for Obould\n", 26);
    ob_3Out_String("  --help                show this help\n", 40);
    ob_3Out_String("  -V, --version         version\n", 33);
    ob_3Out_String("  -w, --width N         image width\n", 37);
    ob_3Out_String("  -h, --height N        image height\n", 38);
    ob_3Out_String("  --seed N              RNG seed\n", 34);
    ob_3Out_String("  -i, --iteration-count N\n", 27);
    ob_3Out_String("  -t, --threads N       number of worker threads\n", 50);
    ob_3Out_String("  -o, --output-path FILE.png\n", 30);
    ob_3Out_String("  -g, --gamma-correction\n", 26);
    ob_3Out_String("  --gamma X             gamma coefficient\n", 43);
    ob_3Out_String("  -s, --symmetry-level N\n", 26);
    ob_3Out_String("  -ap, --affine-params a,b,c,d,e,f/...\n", 40);
    ob_3Out_String("  -f, --functions name:weight,name:weight\n", 43);
}

bool ob_8FlameCli_LoadConfig(ob_10FlameTypes_Config* config) {
    int64_t argc;
    int64_t index;
    int64_t parsedInt;
    double parsedReal;
    char option[ob_10FlameTypes_MaxCliText];
    char value[ob_10FlameTypes_MaxCliText];
    bool result;
    ob_8FlameCli_SetDefaults(config);
    argc = ob_4Args_Count();
    index = 1;
    result = true;
    while (((index < argc) && result)) {
        ob_4Args_Get(index, option, 512);
        if ((strcmp(option, "--help") == 0)) {
            ob_8FlameCli_PrintHelp();
            result = false;
            index = argc;
        } else if (((strcmp(option, "-V") == 0) || (strcmp(option, "--version") == 0))) {
            ob_3Out_String("fractal-flame-obould 0.1\n", 26);
            result = false;
            index = argc;
        } else if (((strcmp(option, "-g") == 0) || (strcmp(option, "--gamma-correction") == 0))) {
            (*config).enableGamma = true;
            index = (index + 1);
        } else if (((strcmp(option, "-w") == 0) || (strcmp(option, "--width") == 0))) {
            result = ob_8FlameCli_RequireValue(argc, index, option, 512);
            if (result) {
                ob_4Args_Get((index + 1), value, 512);
                result = ob_5Parse_Int(value, &(parsedInt), 512);
                if (result) {
                    (*config).width = parsedInt;
                    index = (index + 2);
                } else {
                    ob_8FlameCli_PrintError("invalid width", 14);
                };
            };
        } else if (((strcmp(option, "-h") == 0) || (strcmp(option, "--height") == 0))) {
            result = ob_8FlameCli_RequireValue(argc, index, option, 512);
            if (result) {
                ob_4Args_Get((index + 1), value, 512);
                result = ob_5Parse_Int(value, &(parsedInt), 512);
                if (result) {
                    (*config).height = parsedInt;
                    index = (index + 2);
                } else {
                    ob_8FlameCli_PrintError("invalid height", 15);
                };
            };
        } else if ((strcmp(option, "--seed") == 0)) {
            result = ob_8FlameCli_RequireValue(argc, index, option, 512);
            if (result) {
                ob_4Args_Get((index + 1), value, 512);
                result = ob_5Parse_Int(value, &(parsedInt), 512);
                if (result) {
                    (*config).seed = parsedInt;
                    index = (index + 2);
                } else {
                    ob_8FlameCli_PrintError("invalid seed", 13);
                };
            };
        } else if (((strcmp(option, "-i") == 0) || (strcmp(option, "--iteration-count") == 0))) {
            result = ob_8FlameCli_RequireValue(argc, index, option, 512);
            if (result) {
                ob_4Args_Get((index + 1), value, 512);
                result = ob_5Parse_Int(value, &(parsedInt), 512);
                if (result) {
                    (*config).iterations = parsedInt;
                    index = (index + 2);
                } else {
                    ob_8FlameCli_PrintError("invalid iteration count", 24);
                };
            };
        } else if (((strcmp(option, "-t") == 0) || (strcmp(option, "--threads") == 0))) {
            result = ob_8FlameCli_RequireValue(argc, index, option, 512);
            if (result) {
                ob_4Args_Get((index + 1), value, 512);
                result = ob_5Parse_Int(value, &(parsedInt), 512);
                if (result) {
                    (*config).threads = parsedInt;
                    index = (index + 2);
                } else {
                    ob_8FlameCli_PrintError("invalid thread count", 21);
                };
            };
        } else if (((strcmp(option, "-o") == 0) || (strcmp(option, "--output-path") == 0))) {
            result = ob_8FlameCli_RequireValue(argc, index, option, 512);
            if (result) {
                ob_4Args_Get((index + 1), value, 512);
                ob_7Strings_Copy(value, (*config).outputPath, 512, 256);
                index = (index + 2);
            };
        } else if ((strcmp(option, "--gamma") == 0)) {
            result = ob_8FlameCli_RequireValue(argc, index, option, 512);
            if (result) {
                ob_4Args_Get((index + 1), value, 512);
                result = ob_5Parse_Real(value, &(parsedReal), 512);
                if (result) {
                    (*config).gamma = parsedReal;
                    index = (index + 2);
                } else {
                    ob_8FlameCli_PrintError("invalid gamma value", 20);
                };
            };
        } else if (((strcmp(option, "-s") == 0) || (strcmp(option, "--symmetry-level") == 0))) {
            result = ob_8FlameCli_RequireValue(argc, index, option, 512);
            if (result) {
                ob_4Args_Get((index + 1), value, 512);
                result = ob_5Parse_Int(value, &(parsedInt), 512);
                if (result) {
                    (*config).symmetry = parsedInt;
                    index = (index + 2);
                } else {
                    ob_8FlameCli_PrintError("invalid symmetry value", 23);
                };
            };
        } else if (((strcmp(option, "-ap") == 0) || (strcmp(option, "--affine-params") == 0))) {
            result = ob_8FlameCli_RequireValue(argc, index, option, 512);
            if (result) {
                ob_4Args_Get((index + 1), value, 512);
                result = ob_8FlameCli_ParseAffineList(value, config, 512);
                if (result) {
                    index = (index + 2);
                } else {
                    ob_8FlameCli_PrintError("invalid affine parameter list", 30);
                };
            };
        } else if (((strcmp(option, "-f") == 0) || (strcmp(option, "--functions") == 0))) {
            result = ob_8FlameCli_RequireValue(argc, index, option, 512);
            if (result) {
                ob_4Args_Get((index + 1), value, 512);
                result = ob_8FlameCli_ParseFunctionList(value, config, 512);
                if (result) {
                    index = (index + 2);
                } else {
                    ob_8FlameCli_PrintError("invalid function list", 22);
                };
            };
        } else {
            ob_3Out_String("Error: unknown option ", 23);
            ob_3Out_String(option, 512);
            ob_3Out_Ln();
            result = false;
        };
    };
    if (result) {
        result = ob_8FlameCli_Validate((*config));
    };
    return result;
}

extern void ob_4Args_4Args(void);
extern void ob_10FlameTypes_10FlameTypes(void);
extern void ob_3Out_3Out(void);
extern void ob_5Parse_5Parse(void);
extern void ob_7Strings_7Strings(void);
void ob_8FlameCli_8FlameCli(void) {
    static int _initialized = 0;
    if (_initialized) return;
    _initialized = 1;
    ob_4Args_4Args();
    ob_10FlameTypes_10FlameTypes();
    ob_3Out_3Out();
    ob_5Parse_5Parse();
    ob_7Strings_7Strings();
}

