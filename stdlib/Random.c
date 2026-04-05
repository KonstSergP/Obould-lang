#include <stdint.h>
#include <stdbool.h>

#if defined(__APPLE__)
#define OB_SYMBOL(name) __asm__("_ob_" name)
#else
#define OB_SYMBOL(name) __asm__("ob_" name)
#endif

void Seed(int64_t seed) OB_SYMBOL("6Random_Seed");
int64_t Next() OB_SYMBOL("6Random_Next");
int64_t NextInt(int64_t n) OB_SYMBOL("6Random_NextInt");
bool NextBool() OB_SYMBOL("6Random_NextBool");
double NextReal() OB_SYMBOL("6Random_NextReal");
void Random(void) OB_SYMBOL("Random");


static uint64_t state = 1;

static uint64_t next_u64() {
    uint64_t z = (state += 0x9e3779b97f4a7c15ULL);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);
}

void Seed(int64_t seed) {
    if (seed == 0) {
        state = 1;
    } else {
        state = (uint64_t)seed;
    }
}

int64_t Next() {
    return (int64_t)next_u64();
}

int64_t NextInt(int64_t n) {
    if (n <= 0) return 0;
    uint64_t positive_rand = next_u64() >> 1;
    return (int64_t)(positive_rand % (uint64_t)n);
}

bool NextBool() {
    return (next_u64() & 1) == 0;
}

double NextReal() {
    uint64_t r = next_u64() >> 11;
    return (double)r * (1.0 / (1LL << 53));
}

void Random(void) {}
