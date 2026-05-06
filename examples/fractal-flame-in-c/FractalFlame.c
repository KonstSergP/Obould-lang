#include "FractalFlame.h"
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <gc.h>

/* Static function prototypes */
static void ob_12FractalFlame_init();

/* Function implementations */
static void ob_12FractalFlame_init() {
    ob_10FlameTypes_Config config;
    ob_6Images_Image image;
    bool ok;
    ok = ob_8FlameCli_LoadConfig(&(config));
    if (ok) {
        image = ob_13FlameRenderer_Render(config);
        if ((image == NULL)) {
            ob_3Out_String("Rendering failed.", 18);
            ob_3Out_Ln();
        } else {
            ok = ob_6Images_SavePNG(image, config.outputPath, 256);
            if (ok) {
                ob_3Out_String("Saved image to ", 16);
                ob_3Out_String(config.outputPath, 256);
                ob_3Out_Ln();
            } else {
                ob_3Out_String("Failed to save image to ", 25);
                ob_3Out_String(config.outputPath, 256);
                ob_3Out_Ln();
            };
        };
    };
}

extern void ob_4Args_4Args(void);
extern void ob_8FlameCli_8FlameCli(void);
extern void ob_13FlameRenderer_13FlameRenderer(void);
extern void ob_10FlameTypes_10FlameTypes(void);
extern void ob_6Images_6Images(void);
extern void ob_3Out_3Out(void);
void ob_12FractalFlame_12FractalFlame(void) {
    static int _initialized = 0;
    if (_initialized) return;
    _initialized = 1;
    ob_4Args_4Args();
    ob_8FlameCli_8FlameCli();
    ob_13FlameRenderer_13FlameRenderer();
    ob_10FlameTypes_10FlameTypes();
    ob_6Images_6Images();
    ob_3Out_3Out();
    ob_12FractalFlame_init();
}

int main(int argc, char** argv) {
    GC_INIT();
    ob_4Args_SetArgs((int64_t)argc, argv);
    ob_12FractalFlame_12FractalFlame();
    return 0;
}
