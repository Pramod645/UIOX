
#ifndef __UIX_STDCKDINT__H
#define __UIX_STDCKDINT__H
/*
stddef.h
*/
/* This is for only STDLIB */

#include "uix_features.h"//??


/* Checked integer arithmetic (C23) */
#define uix_ckd_add(res, a, b) __builtin_add_overflow((a),(b),(res)) //Checked addition — returns true if overflow occurred, C23 standard
#define uix_ckd_sub(res, a, b) __builtin_sub_overflow((a),(b),(res)) //Checked subtraction — maps to __builtin_sub_overflow()
#define uix_ckd_mul(res, a, b) __builtin_mul_overflow((a),(b),(res)) //Checked multiplication — prevents silent integer overflow bugs



#endif /* End of __UIX_STDCKDINT__H */
/* ***This is End of file, there is no more line should be added after this line*** */
