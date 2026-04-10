
#ifndef __LIMITS__H
#define __LIMITS__H
/*
limits.h is one of the core headers in the C standard library, and it defines implementation-specific limits for the 
fundamental data types (like char, int, long, etc.).  
These macros tell you the minimum and maximum values that each integer type can hold on your system.

*/
/* This is for only POXIS */

#include "features.h"

#if  (define __POSIX  || define __GLIBC)

#ifdef _cplusplus
extern "C" {
#endif

// Number of bits in a byte /
#define CHARBIT 8

// Signed char limits /
#define SCHARMIN (-128)
#define SCHARMAX 127

// Unsigned char limits /
#define UCHARMAX 255

// char may be signed or unsigned depending on implementation /
#ifdef CHARUNSIGNED
define CHARMIN 0
define CHARMAX 255
#else
define CHARMIN (-128)
define CHARMAX 127
#endif

// Short integer limits /
#define SHRTMIN (-32768)
#define SHRTMAX 32767
#define USHRTMAX 65535

// Integer limits (typically 32-bit) /
#define INTMIN (-2147483647 - 1)
#define INTMAX 2147483647
#define UINTMAX 4294967295U

// Long integer limits (typically 64-bit on LP64 systems) /
#define LONGMIN (-9223372036854775807L - 1L)
#define LONGMAX 9223372036854775807L
#define ULONGMAX 18446744073709551615UL

// Long long integer limits (C99 and later) /
#define LLONGMIN (-9223372036854775807LL - 1LL)
#define LLONGMAX 9223372036854775807LL
#define ULLONGMAX 18446744073709551615ULL

#ifdef cplusplus
}
#endif


#endif /* End  of POXIS and STDLIB*/

#endif /* End of __LIMITS__H */
/* ***This is End of file, there is no more line should be added after this line*** */