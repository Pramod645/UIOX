
#ifndef __UIX_MATH__H
#define __UIX_MATH__H
/*
math.h is the standard C mathematics library header, defining mathematical functions (e.g., sin(), pow(), sqrt()) and 
constants (like HUGEVAL) for floating‑point and integer math operations.  

It’s part of the ISO C standard and required on all POSIX systems.

*/
/* This is for only POXIS and standerd*/

//#include "features.h"


#include "sys/uix_types.h"

#define UIX_M_PI      3.14159265358979323846 // π constant — glibc extension, not strictly POSIX but universally available
#define UIX_M_E       2.71828182845904523536
#define UIX_M_LOG2E   1.44269504088896340736
#define UIX_M_LOG10E  0.43429448190325182765
#define UIX_M_LN2     0.69314718055994530942
#define UIX_M_LN10    2.30258509299404568402
#define UIX_M_SQRT2   1.41421356237309504880
#define UIX_HUGE_VAL  __builtin_huge_val()
#define UIX_NAN       __builtin_nan("") // IEEE 754 Not-a-Number — C99
#define UIX_INFINITY  __builtin_inf() // IEEE 754 positive infinity — C99

double uix_sin  (double x); // Sine using Taylor series reduction to [-π,π]
double uix_cos  (double x); // Cosine via sin(x + π/2)
double uix_tan  (double x); // Tangent = sin/cos
double uix_asin (double x); // Arc sine via atan2(x, sqrt(1-x²))
double uix_acos (double x); 
double uix_atan (double x); // Arc tangent via Taylor series for
double uix_atan2(double y, double x); // Four-quadrant arc tangent — POSIX required
double uix_sinh (double x);
double uix_cosh (double x);
double uix_tanh (double x);
double uix_exp  (double x); // e^x via Taylor series
double uix_log  (double x); // Natural log via atanh identity
double uix_log2 (double x); // Log base 2 = log(x)/ln(2)
double uix_log10(double x); // Log base 10 = log(x)/ln(10)
double uix_pow  (double base, double exp); // b^e = exp(e * log(b))
double uix_sqrt (double x); // Square root via Newton-Raphson iteration
double uix_cbrt (double x); // Cube root via Newton's method
double uix_ceil (double x); // Smallest integer ≥ x
double uix_floor(double x); // Largest integer ≤ x
double uix_round(double x); // Round to nearest integer, ties away from zero
double uix_trunc(double x); // Round toward zero — truncates decimal part
double uix_fmod (double x, double y); // Floating-point remainder — x - trunc(x/y)*y
double uix_fabs (double x); // Absolute value of double

float  uix_sinf  (float x); // Float version — C99 added f suffix variants
float  uix_cosf  (float x);
float  uix_tanf  (float x);
float  uix_sqrtf (float x);
float  uix_fabsf (float x);
float  uix_powf  (float base, float exp);
float  uix_ceilf (float x);
float  uix_floorf(float x);
float  uix_roundf(float x);

int    uix_isnan    (double x); // Tests for NaN — x != x is IEEE 754 guarantee
int    uix_isinf    (double x); // Tests for infinity
int    uix_isfinite (double x); // True if neither NaN nor infinity


#endif /* End of __UIX_MATH__H */
/* ***This is End of file, there is no more line should be added after this line*** */
