
#ifndef __STDDEF__H
#define __STDDEF__H
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

#ifndef UIX_STDDEF_H
#define UIX_STDDEF_H

#include "uix_types.h"

#ifndef NULL
#  define NULL ((void *)0)
#endif

#define UIX_OFFSETOF(type, member)  __builtin_offsetof(type, member)
#define UIX_SIZEOF_MEMBER(type, m)  sizeof(((type *)0)->m)

typedef uix_ptrdiff_t uix_ptrdiff_t;
typedef uix_size_t    uix_size_t;

#ifndef __cplusplus
typedef int uix_wchar_t;
#endif

#endif /* UIX_STDDEF_H */


#endif /* End of __STDDEF__H */
/* ***This is End of file, there is no more line should be added after this line*** */