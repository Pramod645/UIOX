
#ifndef __UIX_STDARG__H
#define __UIX_STDARG__H
/*
uix_stdargh
*/
/* This is for only STDLIB */

//#include "uix_features.h"///???


typedef __builtin_va_list uix_va_list; // Variadic argument list type — compiler-intrinsic, maps to va_list in <stdarg.h>

#define uix_va_start(ap, last)  __builtin_va_start(ap, last) // Initializes va_list to first variadic argument after last
#define uix_va_arg(ap, type)    __builtin_va_arg(ap, type) // Retrieves next argument of type from va_list
#define uix_va_end(ap)          __builtin_va_end(ap) // Cleans up va_list — required after use to avoid undefined behavior
#define uix_va_copy(dst, src)   __builtin_va_copy(dst, src) // Copies va_list state — required for functions that traverse args multiple times


#endif /* End of __UIX_STDARG__H */
/* ***This is End of file, there is no more line should be added after this line*** */
