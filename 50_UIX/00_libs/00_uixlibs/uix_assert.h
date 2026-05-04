
#ifndef __UIX_ASSERT__H
#define __UIX_ASSERT__H
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

#ifndef UIX_ASSERT_H
#define UIX_ASSERT_H

#include "uix_stdio.h"
#include "uix_stdlib.h"

#ifdef UIX_NDEBUG
#  define uix_assert(expr)  ((void)0)
#else
#  define uix_assert(expr)                                        \
     ((expr) ? (void)0                                            \
             : (uix_fprintf(uix_stderr,                           \
                "Assertion failed: %s, file %s, line %d\n",       \
                #expr, __FILE__, __LINE__), uix_abort()))
#endif

#define uix_static_assert(expr, msg) \
    typedef char uix_static_assert_##__LINE__[(expr) ? 1 : -1]

#endif /* UIX_ASSERT_H */


#endif /* End of __UIX_ASSERT__H */
/* ***This is End of file, there is no more line should be added after this line*** */