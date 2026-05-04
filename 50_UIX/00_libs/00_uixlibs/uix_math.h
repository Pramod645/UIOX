
#ifndef __UIX_MATH__H
#define __UIX_MATH__H
/*
math.h is the standard C mathematics library header, defining mathematical functions (e.g., sin(), pow(), sqrt()) and 
constants (like HUGEVAL) for floating‑point and integer math operations.  

It’s part of the ISO C standard and required on all POSIX systems.

*/
/* This is for only POXIS */

#include "features.h"

#if  (define __POSIX  || define __GLIBC)

#ifdef _cplusplus
extern "C" {
#endif

//Common mathematical constants /
#define MPI        3.14159265358979323846  // π /
#define ME         2.71828182845904523536  // e /

// Error handling constant /
#define HUGEVAL (_builtinhugeval())

// Trigonometric functions /
double sin(double x);
double cos(double x);
double tan(double x);
double asin(double x);
double acos(double x);
double atan(double x);
double atan2(double y, double x);

// Hyperbolic functions /
double sinh(double x);
double cosh(double x);
double tanh(double x);

// Exponential and logarithmic functions /
double exp(double x);
double frexp(double x, int exp);
double ldexp(double x, int exp);
double log(double x);
double log10(double x);
double modf(double x, double iptr);

// Power and absolute value functions /
double pow(double x, double y);
double sqrt(double x);
double fabs(double x);
double fmod(double x, double y);

// Rounding and remainder /
double ceil(double x);
double floor(double x);
double trunc(double x);
double round(double x);
double remainder(double x, double y);

#ifdef cplusplus
}
#endif


#endif /* End  of POXIS and STDLIB*/

#ifndef UIX_MATH_H
#define UIX_MATH_H

#include "uix_types.h"

#define UIX_M_PI      3.14159265358979323846
#define UIX_M_E       2.71828182845904523536
#define UIX_M_LOG2E   1.44269504088896340736
#define UIX_M_LOG10E  0.43429448190325182765
#define UIX_M_LN2     0.69314718055994530942
#define UIX_M_LN10    2.30258509299404568402
#define UIX_M_SQRT2   1.41421356237309504880
#define UIX_HUGE_VAL  __builtin_huge_val()
#define UIX_NAN       __builtin_nan("")
#define UIX_INFINITY  __builtin_inf()

double uix_sin  (double x);
double uix_cos  (double x);
double uix_tan  (double x);
double uix_asin (double x);
double uix_acos (double x);
double uix_atan (double x);
double uix_atan2(double y, double x);
double uix_sinh (double x);
double uix_cosh (double x);
double uix_tanh (double x);
double uix_exp  (double x);
double uix_log  (double x);
double uix_log2 (double x);
double uix_log10(double x);
double uix_pow  (double base, double exp);
double uix_sqrt (double x);
double uix_cbrt (double x);
double uix_ceil (double x);
double uix_floor(double x);
double uix_round(double x);
double uix_trunc(double x);
double uix_fmod (double x, double y);
double uix_fabs (double x);

float  uix_sinf  (float x);
float  uix_cosf  (float x);
float  uix_tanf  (float x);
float  uix_sqrtf (float x);
float  uix_fabsf (float x);
float  uix_powf  (float base, float exp);
float  uix_ceilf (float x);
float  uix_floorf(float x);
float  uix_roundf(float x);

int    uix_isnan    (double x);
int    uix_isinf    (double x);
int    uix_isfinite (double x);

#endif /* UIX_MATH_H */



#endif /* End of __UIX_MATH__H */
/* ***This is End of file, there is no more line should be added after this line*** */