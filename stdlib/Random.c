#include <stdint.h>
#include <stdbool.h>
#include "Random.h"

#if defined(__APPLE__)
#define OB_SYMBOL(name) __asm__("_ob_" name)
#else
#define OB_SYMBOL(name) __asm__("ob_" name)
#endif

typedef struct GeneratorDescriptor
{
    int64_t depth;
    void* descriptors[1];
} GeneratorDescriptor;

GeneratorDescriptor generatorDescriptor OB_SYMBOL("6Random_struct_desc_Generator") = {
    0, {&generatorDescriptor}
};

static ob_6Random_Generator global_gen;


void ob_6Random_Seed(int64_t seed) OB_SYMBOL("6Random_Seed");
int64_t ob_6Random_Int(void) OB_SYMBOL("6Random_Int");
int64_t ob_6Random_IntN(int64_t n) OB_SYMBOL("6Random_IntN");
bool ob_6Random_Bool(void) OB_SYMBOL("6Random_Bool");
double ob_6Random_Real(void) OB_SYMBOL("6Random_Real");
double ob_6Random_Between(double min, double max) OB_SYMBOL("6Random_Between");

void ob_6Random_GenSeed(ob_6Random_Generator* gen, int64_t seed) OB_SYMBOL("6Random_GenSeed");
int64_t ob_6Random_GenInt(ob_6Random_Generator* gen) OB_SYMBOL("6Random_GenInt");
int64_t ob_6Random_GenIntN(ob_6Random_Generator* gen, int64_t n) OB_SYMBOL("6Random_GenIntN");
bool ob_6Random_GenBool(ob_6Random_Generator* gen) OB_SYMBOL("6Random_GenBool");
double ob_6Random_GenReal(ob_6Random_Generator* gen) OB_SYMBOL("6Random_GenReal");
double ob_6Random_GenBetween(ob_6Random_Generator* gen, double min, double max) OB_SYMBOL("6Random_GenBetween");

void ob_6Random_6Random(void) OB_SYMBOL("6Random_6Random");


void ob_6Random_Seed(int64_t seed) {
    ob_6Random_GenSeed(&global_gen, seed);
}

int64_t ob_6Random_Int(void) {
    return ob_6Random_GenInt(&global_gen);
}

int64_t ob_6Random_IntN(int64_t n) {
    return ob_6Random_GenIntN(&global_gen, n);
}

bool ob_6Random_Bool(void) {
    return ob_6Random_GenBool(&global_gen);
}

double ob_6Random_Real(void) {
    return ob_6Random_GenReal(&global_gen);
}

double ob_6Random_Between(double min, double max) {
    return ob_6Random_GenBetween(&global_gen, min, max);
}

// SplitMix64
static uint64_t xorshift_next(ob_6Random_Generator* gen) {
    uint64_t z = gen->state += 0x9e3779b97f4a7c15ULL;
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);
}

void ob_6Random_GenSeed(ob_6Random_Generator* gen, int64_t seed) {
    if (gen) {
        gen->_desc = &generatorDescriptor;
        gen->state = (seed == 0) ? 1 : (uint64_t)seed;
    }
}

int64_t ob_6Random_GenInt(ob_6Random_Generator* gen) {
    if (!gen) return 0;
    return (int64_t)xorshift_next(gen);
}

int64_t ob_6Random_GenIntN(ob_6Random_Generator* gen, int64_t n) {
    if (!gen) return 0;
    uint64_t s = xorshift_next(gen);
    if (n <= 0) {
        return (int64_t)s;
    }
    return (int64_t)((s >> 1) % (uint64_t)n);
}

bool ob_6Random_GenBool(ob_6Random_Generator* gen) {
    if (!gen) return false;
    return (xorshift_next(gen) & 1) == 0;
}

double ob_6Random_GenReal(ob_6Random_Generator* gen) {
    if (!gen) return 0.0;
    return (double)(xorshift_next(gen) >> 11) * (1.0 / (1LL << 53));
}

double ob_6Random_GenBetween(ob_6Random_Generator* gen, double min, double max) {
    if (!gen) return min;
    return min + (max - min) * ob_6Random_GenReal(gen);
}

void ob_6Random_6Random(void) {
    ob_6Random_GenSeed(&global_gen, 0);
}
