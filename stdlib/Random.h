#ifndef RANDOM_H
#define RANDOM_H

#include <stdint.h>
#include <stdbool.h>

typedef struct GeneratorDescriptor GeneratorDescriptor;

typedef struct {
    GeneratorDescriptor* _desc;
    uint64_t state;
} ob_6Random_Generator;

void ob_6Random_Seed(int64_t seed);
int64_t ob_6Random_Int(void);
int64_t ob_6Random_IntN(int64_t n);
bool ob_6Random_Bool(void);
double ob_6Random_Real(void);
double ob_6Random_Between(double min, double max);

void ob_6Random_GenSeed(ob_6Random_Generator* gen, int64_t seed);
int64_t ob_6Random_GenInt(ob_6Random_Generator* gen);
int64_t ob_6Random_GenIntN(ob_6Random_Generator* gen, int64_t n);
bool ob_6Random_GenBool(ob_6Random_Generator* gen);
double ob_6Random_GenReal(ob_6Random_Generator* gen);
double ob_6Random_GenBetween(ob_6Random_Generator* gen, double min, double max);

void ob_6Random_6Random(void);

#endif /* RANDOM_H */