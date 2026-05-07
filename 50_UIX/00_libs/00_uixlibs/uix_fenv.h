
#ifndef __FENV__H
#define __FENV__H
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


#ifndef UIX_FENV_H
#define UIX_FENV_H

#include "uix_types.h"

typedef uix_uint32_t uix_fexcept_t;
typedef struct { uix_uint32_t cw; uix_uint32_t sw; uix_uint32_t mxcsr; } uix_fenv_t;

#define UIX_FE_INVALID    0x01
#define UIX_FE_DIVBYZERO  0x04
#define UIX_FE_OVERFLOW   0x08
#define UIX_FE_UNDERFLOW  0x10
#define UIX_FE_INEXACT    0x20
#define UIX_FE_ALL_EXCEPT (UIX_FE_INVALID|UIX_FE_DIVBYZERO|UIX_FE_OVERFLOW|\
                           UIX_FE_UNDERFLOW|UIX_FE_INEXACT)

#define UIX_FE_TONEAREST  0x0000
#define UIX_FE_DOWNWARD   0x0400
#define UIX_FE_UPWARD     0x0800
#define UIX_FE_TOWARDZERO 0x0C00

#define UIX_FE_DFL_ENV    ((const uix_fenv_t *)(-1))

int uix_feclearexcept  (int excepts);
int uix_fetestexcept   (int excepts);
int uix_feraiseexcept  (int excepts);
int uix_fegetexceptflag(uix_fexcept_t *flagp, int excepts);
int uix_fesetexceptflag(const uix_fexcept_t *flagp, int excepts);
int uix_fegetround     (void);
int uix_fesetround     (int round);
int uix_fegetenv       (uix_fenv_t *envp);
int uix_fesetenv       (const uix_fenv_t *envp);
int uix_feholdexcept   (uix_fenv_t *envp);
int uix_feupdateenv    (const uix_fenv_t *envp);

#endif /* UIX_FENV_H */


#endif /* End of __FENV__H */
/* ***This is End of file, there is no more line should be added after this line*** */