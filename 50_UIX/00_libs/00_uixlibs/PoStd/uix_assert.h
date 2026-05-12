
#ifndef __UIX_ASSERT__H
#define __UIX_ASSERT__H
/*
uix_assert.h
*/
/* This is for only STDLIB */

#include "uix_stdio.h"
#include "uix_stdlib.h"

#ifdef UIX_NDEBUG //If defined, assertions compile away to nothing — same as standard NDEBUG
#  define uix_assert(expr)  ((void)0)
#else
/*Runtime assertion — maps to POSIX assert() from <assert.h>. On failure prints file/line and calls abort()*/
#  define uix_assert(expr)                                        \
     ((expr) ? (void)0                                            \
             : (uix_fprintf(uix_stderr,                           \
                "Assertion failed: %s, file %s, line %d\n",       \
                #expr, __FILE__, __LINE__), uix_abort()))
#endif
/*Compile-time assertion — C11 _Static_assert(), zero-size array trick for older compilers*/
#define uix_static_assert(expr, msg) \
    typedef char uix_static_assert_##__LINE__[(expr) ? 1 : -1]


#endif /* End of __UIX_ASSERT__H */
/* ***This is End of file, there is no more line should be added after this line*** */
