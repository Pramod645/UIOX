
#ifndef __UIX_STDINT__H
#define __UIX_STDINT__H
/*
stddef.h
*/
/* This is for only STDLIB */

#include "features.h"


#include "uix_types.h"

/* Exact-width */  // Direct equivalents to C99 <stdint.h> int8_t..uint64_t — required by POSIX 2008
typedef uix_int8_t    uix_int8_t;
typedef uix_uint8_t   uix_uint8_t;
typedef uix_int16_t   uix_int16_t;
typedef uix_uint16_t  uix_uint16_t;
typedef uix_int32_t   uix_int32_t;
typedef uix_uint32_t  uix_uint32_t;
typedef uix_int64_t   uix_int64_t;
typedef uix_uint64_t  uix_uint64_t;

/* Least-width */ // int_least8_t etc — guaranteed minimum width types from C99
typedef uix_int8_t    uix_int_least8_t;
typedef uix_uint8_t   uix_uint_least8_t;
typedef uix_int16_t   uix_int_least16_t;
typedef uix_uint16_t  uix_uint_least16_t;
typedef uix_int32_t   uix_int_least32_t;
typedef uix_uint32_t  uix_uint_least32_t;
typedef uix_int64_t   uix_int_least64_t;
typedef uix_uint64_t  uix_uint_least64_t;

/* Fast */
typedef uix_int32_t   uix_int_fast8_t;
typedef uix_uint32_t  uix_uint_fast8_t;
typedef uix_int32_t   uix_int_fast16_t;
typedef uix_uint32_t  uix_uint_fast16_t;
typedef uix_int32_t   uix_int_fast32_t;
typedef uix_uint32_t  uix_uint_fast32_t;
typedef uix_int64_t   uix_int_fast64_t;
typedef uix_uint64_t  uix_uint_fast64_t;

/* Pointer-sized */
typedef uix_int64_t   uix_intptr_t; // Integer type large enough to hold a pointer — required by POSIX for mmap() returns
typedef uix_uint64_t  uix_uintptr_t2;
typedef uix_int64_t   uix_intmax_t;
typedef uix_uint64_t  uix_uintmax_t;

/* Limits */ // int_fast8_t etc — fastest types of at least N bits, implementation-defined
#define UIX_INT8_MIN    (-128)
#define UIX_INT8_MAX    127
#define UIX_UINT8_MAX   255U
#define UIX_INT16_MIN   (-32768)
#define UIX_INT16_MAX   32767
#define UIX_UINT16_MAX  65535U
#define UIX_INT32_MIN   (-2147483647-1) //Written this way to avoid overflow in signed arithmetic per C99 standard
#define UIX_INT32_MAX   2147483647
#define UIX_UINT32_MAX  4294967295U
#define UIX_INT64_MIN   (-9223372036854775807LL-1LL)
#define UIX_INT64_MAX   9223372036854775807LL
#define UIX_UINT64_MAX  18446744073709551615ULL
#define UIX_INTMAX_MAX  UIX_INT64_MAX
#define UIX_UINTMAX_MAX UIX_UINT64_MAX
#define UIX_INTPTR_MAX  UIX_INT64_MAX
#define UIX_UINTPTR_MAX UIX_UINT64_MAX

/* Constant macros */
#define UIX_INT8_C(v)   ((uix_int8_t)(v))
#define UIX_UINT8_C(v)  ((uix_uint8_t)(v))
#define UIX_INT16_C(v)  ((uix_int16_t)(v))
#define UIX_UINT16_C(v) ((uix_uint16_t)(v))
#define UIX_INT32_C(v)  (v)
#define UIX_UINT32_C(v) (v##U)
#define UIX_INT64_C(v)  (v##LL)
#define UIX_UINT64_C(v) (v##ULL) // Token-paste macro to create long long constants — matches glibc's INT64_C()
#define UIX_INTMAX_C(v) UIX_INT64_C(v)
#define UIX_UINTMAX_C(v) UIX_UINT64_C(v)


#endif /* End of __UIX_STDINT__H */
/* ***This is End of file, there is no more line should be added after this line*** */
