
#ifndef __UIX_STDARG__H
#define __UIX_STDARG__H
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

#ifndef UIX_STDARG_H
#define UIX_STDARG_H

typedef __builtin_va_list uix_va_list;

#define uix_va_start(ap, last)  __builtin_va_start(ap, last)
#define uix_va_arg(ap, type)    __builtin_va_arg(ap, type)
#define uix_va_end(ap)          __builtin_va_end(ap)
#define uix_va_copy(dst, src)   __builtin_va_copy(dst, src)

#endif /* UIX_STDARG_H */


#endif /* End of __UIX_STDARG__H */
/* ***This is End of file, there is no more line should be added after this line*** */