#ifndef RANDOM_H
#define RANDOM_H

#include <stdint.h>
#include <stdbool.h>

void ob_6Random_Seed(int64_t seed);
int64_t ob_6Random_Next(void);
int64_t ob_6Random_NextInt(int64_t n);
bool ob_6Random_NextBool(void);
double ob_6Random_NextReal(void);
void ob_6Random_Random(void);

#endif /* RANDOM_H */
