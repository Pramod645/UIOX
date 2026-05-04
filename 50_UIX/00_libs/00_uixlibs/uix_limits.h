
#ifndef __UIX_LIMITS__H
#define __UIX_LIMITS__H
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

#ifndef UIX_LIMITS_H
#define UIX_LIMITS_H

#define UIX_CHAR_BIT    8
#define UIX_SCHAR_MIN   (-128)
#define UIX_SCHAR_MAX   127
#define UIX_UCHAR_MAX   255
#define UIX_CHAR_MIN    UIX_SCHAR_MIN
#define UIX_CHAR_MAX    UIX_SCHAR_MAX
#define UIX_SHRT_MIN    (-32768)
#define UIX_SHRT_MAX    32767
#define UIX_USHRT_MAX   65535U
#define UIX_INT_MIN     (-2147483647 - 1)
#define UIX_INT_MAX     2147483647
#define UIX_UINT_MAX    4294967295U
#define UIX_LONG_MIN    (-9223372036854775807L - 1L)
#define UIX_LONG_MAX    9223372036854775807L
#define UIX_ULONG_MAX   18446744073709551615UL
#define UIX_LLONG_MIN   (-9223372036854775807LL - 1LL)
#define UIX_LLONG_MAX   9223372036854775807LL
#define UIX_ULLONG_MAX  18446744073709551615ULL

#define UIX_PATH_MAX        4096
#define UIX_NAME_MAX        255
#define UIX_ARG_MAX         131072
#define UIX_CHILD_MAX       64
#define UIX_OPEN_MAX        128
#define UIX_PIPE_BUF        512
#define UIX_HOST_NAME_MAX   64
#define UIX_LOGIN_NAME_MAX  256
#define UIX_LINE_MAX        4096
#define UIX_NGROUPS_MAX     65536

#define UIX_FLT_MAX       3.40282347e+38F
#define UIX_FLT_MIN       1.17549435e-38F
#define UIX_DBL_MAX       1.7976931348623157e+308
#define UIX_DBL_MIN       2.2250738585072014e-308
#define UIX_FLT_EPSILON   1.19209290e-07F
#define UIX_DBL_EPSILON   2.2204460492503131e-16
#define UIX_FLT_DIG       6
#define UIX_DBL_DIG       15

#endif /* UIX_LIMITS_H */


#endif /* End of __UIX_LIMITS__H */
/* ***This is End of file, there is no more line should be added after this line*** */