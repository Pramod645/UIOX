
#ifndef __MATH__H
#define __MATH__H
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

#endif /* End of __MATH__H */
/* ***This is End of file, there is no more line should be added after this line*** */