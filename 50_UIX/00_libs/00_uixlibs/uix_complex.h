
#ifndef __COMPLEX__H
#define __COMPLEX__H
/*
stddef.h
*/
/* This is for only STDLIB */

#include "features.h"

#if  (define __GLIBC)

#ifdef _cplusplus
extern "C" {
#endif



#ifdef cplusplus
}
#endif


#endif /* End  of STDLIB*/

#ifndef UIX_COMPLEX_H
#define UIX_COMPLEX_H

#include "uix_types.h"

typedef struct { double real; double imag; } uix_complex_t;        // Complex number as pair of doubles
typedef struct { float  real; float  imag; } uix_complexf_t;

/* Construction */
UIX_INLINE uix_complex_t  uix_cmplx (double r, double i) { uix_complex_t  c={r,i}; return c; }
UIX_INLINE uix_complexf_t uix_cmplxf(float  r, float  i) { uix_complexf_t c={r,i}; return c; }

/* Parts */
UIX_INLINE double uix_creal(uix_complex_t c) { return c.real; }       //Returns real part
UIX_INLINE double uix_cimag(uix_complex_t c) { return c.imag; }       // Returns imaginary part

/* Arithmetic */
UIX_INLINE uix_complex_t uix_cadd(uix_complex_t a, uix_complex_t b)     // Complex addition
    { uix_complex_t r={a.real+b.real,a.imag+b.imag}; return r; }
UIX_INLINE uix_complex_t uix_csub(uix_complex_t a, uix_complex_t b)     
    { uix_complex_t r={a.real-b.real,a.imag-b.imag}; return r; }
UIX_INLINE uix_complex_t uix_cmul(uix_complex_t a, uix_complex_t b) {   //// Complex multiplication using FOIL
    uix_complex_t r={a.real*b.real-a.imag*b.imag,
                     a.real*b.imag+a.imag*b.real}; return r; }
UIX_INLINE uix_complex_t uix_cdiv(uix_complex_t a, uix_complex_t b) {   //Complex division — divides by magnitude squared
    double d=b.real*b.real+b.imag*b.imag;
    uix_complex_t r={(a.real*b.real+a.imag*b.imag)/d,
                     (a.imag*b.real-a.real*b.imag)/d}; return r; }

double uix_cabs (uix_complex_t c);      // Magnitude = sqrt(real² + imag²)
double uix_carg (uix_complex_t c);        //Phase angle = atan2(imag, real)
uix_complex_t uix_cconj(uix_complex_t c);  // Complex conjugate — negates imaginary part
uix_complex_t uix_cexp (uix_complex_t c);         // Complex exponential = e^(r+i*i) = e^r * (cos(i)+i*sin(i))
uix_complex_t uix_clog (uix_complex_t c);
uix_complex_t uix_csqrt(uix_complex_t c);       // Complex square root
uix_complex_t uix_cpow (uix_complex_t base, uix_complex_t exp);
uix_complex_t uix_csin (uix_complex_t c);
uix_complex_t uix_ccos (uix_complex_t c);

#ifndef UIX_INLINE
#define UIX_INLINE static inline
#endif

#endif /* UIX_COMPLEX_H */


#endif /* End of __COMPLEX__H */
/* ***This is End of file, there is no more line should be added after this line*** */