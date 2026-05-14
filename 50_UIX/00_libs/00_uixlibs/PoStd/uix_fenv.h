
#ifndef __UIX_FENV_H
#define __UIX_FENV_H
/*
uix_fenv.h
*/
/* This is for only STDLIB */

//#include "uix_features.h"//??

#include "../sys/uix_types.h"

typedef uix_uint32_t uix_fexcept_t;
typedef struct { uix_uint32_t cw; uix_uint32_t sw; uix_uint32_t mxcsr; } uix_fenv_t;

#define UIX_FE_INVALID    0x01    // Invalid operation exception (0/0, sqrt(-1))
#define UIX_FE_DIVBYZERO  0x04   // Division by zero exception
#define UIX_FE_OVERFLOW   0x08   // Result too large to represent
#define UIX_FE_UNDERFLOW  0x10  // Result too small to represent
#define UIX_FE_INEXACT    0x20   // Result not exactly representable

/* All exception flags ORed */
#define UIX_FE_ALL_EXCEPT (UIX_FE_INVALID|UIX_FE_DIVBYZERO|UIX_FE_OVERFLOW|\
                           UIX_FE_UNDERFLOW|UIX_FE_INEXACT)

#define UIX_FE_TONEAREST  0x0000   // Round to nearest even
#define UIX_FE_DOWNWARD   0x0400   // Round toward negative infinity
#define UIX_FE_UPWARD     0x0800   // Round toward positive infinity
#define UIX_FE_TOWARDZERO 0x0C00   // Round toward zero (truncate)

#define UIX_FE_DFL_ENV    ((const uix_fenv_t *)(-1))     // Default floating-point environment

int uix_feclearexcept  (int excepts);                             // Clears FPU exception flags
int uix_fetestexcept   (int excepts);                             // Tests which exception flags are set
int uix_feraiseexcept  (int excepts);                             // Raises floating-point exception
int uix_fegetexceptflag(uix_fexcept_t *flagp, int excepts);
int uix_fesetexceptflag(const uix_fexcept_t *flagp, int excepts);
int uix_fegetround     (void);                                     // Gets current rounding mode
int uix_fesetround     (int round);                                // Sets rounding mode
int uix_fegetenv       (uix_fenv_t *envp);                         // Saves entire FPU state
int uix_fesetenv       (const uix_fenv_t *envp);                   // Restores FPU state
int uix_feholdexcept   (uix_fenv_t *envp);                         // Saves state and clears all exceptions
int uix_feupdateenv    (const uix_fenv_t *envp);                  //


#endif /* End of __UIX_FENV_H */
/* ***This is End of file, there is no more line should be added after this line*** */
