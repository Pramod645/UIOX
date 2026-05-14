
#ifndef __UIX_STDDEF__H
#define __UIX_STDDEF__H
/*
stddef.h
*/
/* This is for only STDLIB */

//#include "uix_features.h"


#include "../sys/uix_types.h"

#ifndef NULL
#  define NULL ((void *)0)
#endif

#define UIX_OFFSETOF(type, member)  __builtin_offsetof(type, member) // Maps to offsetof() from <stddef.h> C99/POSIX — gives byte offset of struct member
#define UIX_SIZEOF_MEMBER(type, m)  sizeof(((type *)0)->m)

typedef uix_ptrdiff_t uix_ptrdiff_t; // Maps to ptrdiff_t — signed result of pointer subtraction, required by C99
typedef uix_size_t    uix_size_t;

#ifndef __cplusplus
typedef int uix_wchar_t; // Wide character type — POSIX wchar_t, used in wcslen(), wcscpy()
#endif


#endif /* End of __UIX_STDDEF__H */
/* ***This is End of file, there is no more line should be added after this line*** */
