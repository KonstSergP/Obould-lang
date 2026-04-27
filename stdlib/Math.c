#include <math.h>
#include <stdint.h>

#if defined(__APPLE__)
#define OB_SYMBOL(name) __asm__("_ob_" name)
#else
#define OB_SYMBOL(name) __asm__("ob_" name)
#endif

double Sin(double x) OB_SYMBOL("4Math_Sin");
double Cos(double x) OB_SYMBOL("4Math_Cos");
double Tan(double x) OB_SYMBOL("4Math_Tan");
double Atan(double x) OB_SYMBOL("4Math_Atan");
double Atan2(double y, double x) OB_SYMBOL("4Math_Atan2");
double Sqrt(double x) OB_SYMBOL("4Math_Sqrt");
double Pow(double base, double exponent) OB_SYMBOL("4Math_Pow");
double Log10(double x) OB_SYMBOL("4Math_Log10");
double Fabs(double x) OB_SYMBOL("4Math_Fabs");
void Math(void) OB_SYMBOL("Math");

double Sin(double x) { return sin(x); }
double Cos(double x) { return cos(x); }
double Tan(double x) { return tan(x); }
double Atan(double x) { return atan(x); }
double Atan2(double y, double x) { return atan2(y, x); }
double Sqrt(double x) { return sqrt(x); }
double Pow(double base, double exponent) { return pow(base, exponent); }
double Log10(double x) { return log10(x); }
double Fabs(double x) { return fabs(x); }
void Math(void) {}
