
#ifndef __UIX_FLOAT__H
#define __UIX_FLOAT__H
/*
uix_float.h
*/
/* This is for only STDLIB */

//#include "uix_features.h"//??


#define UIX_FLT_RADIX       2 //Floating point base — always 2 (binary) on IEEE 754 systems
#define UIX_FLT_ROUNDS      1

#define UIX_FLT_DIG         6
#define UIX_FLT_MANT_DIG    24 // Mantissa digits — IEEE 754 single precision has 24 binary digits
#define UIX_FLT_MAX_EXP     128
#define UIX_FLT_MIN_EXP     (-125)
#define UIX_FLT_MAX_10_EXP  38
#define UIX_FLT_MIN_10_EXP  (-37)
#define UIX_FLT_EPSILON     1.19209290e-07F //Smallest float that changes 1.0f — from C99 <float.h>
#define UIX_FLT_MAX         3.40282347e+38F
#define UIX_FLT_MIN         1.17549435e-38F

#define UIX_DBL_DIG         15
#define UIX_DBL_MANT_DIG    53
#define UIX_DBL_MAX_EXP     1024 //Maximum exponent for double — IEEE 754 double
#define UIX_DBL_MIN_EXP     (-1021)
#define UIX_DBL_MAX_10_EXP  308
#define UIX_DBL_MIN_10_EXP  (-307)
#define UIX_DBL_EPSILON     2.2204460492503131e-16
#define UIX_DBL_MAX         1.7976931348623157e+308
#define UIX_DBL_MIN         2.2250738585072014e-308

#define UIX_LDBL_DIG        18
#define UIX_LDBL_EPSILON    1.08420217248550443401e-19L


#endif /* End of __UIX_FLOAT__H */
/* ***This is End of file, there is no more line should be added after this line*** */
