
#ifndef __STDCKDINT__H
#define __STDCKDINT__H
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

#ifndef UIX_STDCKDINT_H
#define UIX_STDCKDINT_H

/* Checked integer arithmetic (C23) */
#define uix_ckd_add(res, a, b) __builtin_add_overflow((a),(b),(res))
#define uix_ckd_sub(res, a, b) __builtin_sub_overflow((a),(b),(res))
#define uix_ckd_mul(res, a, b) __builtin_mul_overflow((a),(b),(res))

#endif /* UIX_STDCKDINT_H */


#endif /* End of __STDCKDINT__H */
/* ***This is End of file, there is no more line should be added after this line*** */