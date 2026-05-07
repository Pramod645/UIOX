
#ifndef __TGMATH__H
#define __TGMATH__H
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

/* include/uix_tgmath.h */
#ifndef UIX_TGMATH_H
#define UIX_TGMATH_H

#include "uix_math.h"

/* Type-generic wrappers using _Generic (C11) */
#define uix_tg_sin(x)    _Generic((x), float: uix_sinf,  default: uix_sin )(x)
#define uix_tg_cos(x)    _Generic((x), float: uix_cosf,  default: uix_cos )(x)
#define uix_tg_tan(x)    _Generic((x), float: uix_tanf,  default: uix_tan )(x)
#define uix_tg_sqrt(x)   _Generic((x), float: uix_sqrtf, default: uix_sqrt)(x)
#define uix_tg_fabs(x)   _Generic((x), float: uix_fabsf, default: uix_fabs)(x)
#define uix_tg_pow(x,y)  _Generic((x), float: uix_powf,  default: uix_pow )(x,y)
#define uix_tg_ceil(x)   _Generic((x), float: uix_ceilf, default: uix_ceil)(x)
#define uix_tg_floor(x)  _Generic((x), float: uix_floorf,default: uix_floor)(x)
#define uix_tg_round(x)  _Generic((x), float: uix_roundf,default: uix_round)(x)

#endif /* UIX_TGMATH_H */



#endif /* End of __TGMATH__H */
/* ***This is End of file, there is no more line should be added after this line*** */