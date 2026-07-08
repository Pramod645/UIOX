/**
 * @file  uix_syscall.c
 * @brief UIOX POSIX — syscall() variadic implementation.
 *
 * Implements the POSIX syscall(2) interface:
 *   long syscall(long number, ...);
 *
 * Up to 6 arguments are extracted from the variadic list and routed
 * to the correct my_syscallN macro via uix_syscall.h.
 * Uses uix_stdarg.h (already in PoStd) — no libc dependency.
 */

 #include "uix_syscall.h"
 #include "uix_stdarg.h"
 
 long uix_syscall(long number, ...)
 {
     va_list ap;
     va_start(ap, number);
     long a1 = va_arg(ap, long);
     long a2 = va_arg(ap, long);
     long a3 = va_arg(ap, long);
     long a4 = va_arg(ap, long);
     long a5 = va_arg(ap, long);
     long a6 = va_arg(ap, long);
     va_end(ap);
     return uix_syscall6(number, a1, a2, a3, a4, a5, a6);
 }
 