/*
 * 30_KIX/33_PCS/include/uiox_klibc.h
 *
 * Freestanding kernel C library — single header replacing ALL system
 * headers used across 33_PCS.
 *
 * Replaces
 * ────────
 *   <stdint.h>   §1  fixed-width integer typedefs
 *   <stdbool.h>  §2  bool / true / false
 *   <stddef.h>   §3  NULL, size_t, ptrdiff_t, offsetof
 *   <time.h>     §4  clock_t, time_t
 *   <string.h>   §5-6  memset, memcpy, memmove, memcmp,
 *                      strlen, strcmp, strncmp, strcpy, strncpy
 *   <stdlib.h>   §3  NULL (no heap; kernel uses static pools)
 *   <stdio.h>    §7  uiox_printf() → BSP SoC stdio
 *   <math.h>     §8  integer-only min/max/abs (no FPU)
 *
 * Usage
 * ─────
 *   Replace every system #include with one line:
 *     #include "uiox_klibc.h"
 *
 *   Compatibility macros are provided so existing code compiles
 *   unchanged:  memset→uiox_memset, printf→uiox_printf, etc.
 *
 * Build
 * ─────
 *   Add -nostdinc and -I<path/to/33_PCS/include> to CFLAGS.
 *
 * @version 1.0.0  @date 2026-07-23
 */
#ifndef UIOX_KLIBC_H
#define UIOX_KLIBC_H

#ifdef __cplusplus
extern "C" {
#endif

/* ══════════════════════════════════════════════════════════════════════
 * §1  Fixed-width integer types                     (replaces <stdint.h>)
 * ══════════════════════════════════════════════════════════════════════ */

typedef signed   char        int8_t;
typedef signed   short       int16_t;
typedef signed   int         int32_t;
typedef signed   long long   int64_t;

typedef unsigned char        uint8_t;
typedef unsigned short       uint16_t;
typedef unsigned int         uint32_t;
typedef unsigned long long   uint64_t;

/* Pointer-width types — detect arch at compile time */
#if defined(__LP64__) || defined(_LP64)      || \
    defined(__aarch64__)                     || \
    defined(__x86_64__)                      || \
    (defined(__riscv) && __riscv_xlen == 64)
  typedef unsigned long long  uintptr_t;
  typedef signed   long long  intptr_t;
  typedef unsigned long long  uintmax_t;
  typedef signed   long long  intmax_t;
  #define UINTPTR_MAX  UINT64_MAX
  #define INTPTR_MAX   INT64_MAX
  #define INTPTR_MIN   INT64_MIN
#else
  typedef unsigned int        uintptr_t;
  typedef signed   int        intptr_t;
  typedef unsigned int        uintmax_t;
  typedef signed   int        intmax_t;
  #define UINTPTR_MAX  UINT32_MAX
  #define INTPTR_MAX   INT32_MAX
  #define INTPTR_MIN   INT32_MIN
#endif

/* Limits */
#define INT8_MIN    (-128)
#define INT8_MAX    (127)
#define UINT8_MAX   (255)
#define INT16_MIN   (-32768)
#define INT16_MAX   (32767)
#define UINT16_MAX  (65535)
#define INT32_MIN   (-2147483647 - 1)
#define INT32_MAX   (2147483647)
#define UINT32_MAX  (4294967295U)
#define INT64_MIN   (-9223372036854775807LL - 1LL)
#define INT64_MAX   (9223372036854775807LL)
#define UINT64_MAX  (18446744073709551615ULL)
#define SIZE_MAX    UINTPTR_MAX

/* ══════════════════════════════════════════════════════════════════════
 * §2  Boolean type                                (replaces <stdbool.h>)
 * ══════════════════════════════════════════════════════════════════════ */
#ifndef __cplusplus
  typedef _Bool bool;
  #define true  ((bool)1)
  #define false ((bool)0)
#endif
#define __bool_true_false_are_defined 1

/* ══════════════════════════════════════════════════════════════════════
 * §3  Core language helpers                        (replaces <stddef.h>)
 * ══════════════════════════════════════════════════════════════════════ */
#ifndef NULL
#  ifdef __cplusplus
#    define NULL nullptr
#  else
#    define NULL ((void *)0)
#  endif
#endif

typedef uintptr_t   size_t;
typedef intptr_t    ssize_t;
typedef intptr_t    ptrdiff_t;

#define offsetof(type, member)  __builtin_offsetof(type, member)

/* ══════════════════════════════════════════════════════════════════════
 * §4  Time types                                     (replaces <time.h>)
 *     clock_t = kernel tick counter (jiffies-compatible)
 * ══════════════════════════════════════════════════════════════════════ */
typedef uint64_t    clock_t;    /* kernel tick counter                  */
typedef int64_t     time_t;     /* seconds since epoch                  */

typedef struct uiox_timespec {
    time_t   tv_sec;
    int32_t  tv_nsec;
} uiox_timespec_t;

typedef struct uiox_timeval {
    time_t   tv_sec;
    uint32_t tv_usec;
} uiox_timeval_t;

#define CLOCKS_PER_SEC  1000ULL  /* matches HZ=1000 in sched_types.h */

/* ══════════════════════════════════════════════════════════════════════
 * §5  Memory operations                           (replaces <string.h>)
 * ══════════════════════════════════════════════════════════════════════ */

static inline void *uiox_memset(void *dst, int c, size_t n)
{
    unsigned char *d = (unsigned char *)dst;
    while (n--) *d++ = (unsigned char)c;
    return dst;
}

static inline void *uiox_memcpy(void *dst, const void *src, size_t n)
{
    unsigned char       *d = (unsigned char *)dst;
    const unsigned char *s = (const unsigned char *)src;
    while (n--) *d++ = *s++;
    return dst;
}

static inline void *uiox_memmove(void *dst, const void *src, size_t n)
{
    unsigned char       *d = (unsigned char *)dst;
    const unsigned char *s = (const unsigned char *)src;
    if (d < s || d >= s + n) {
        while (n--) *d++ = *s++;
    } else {
        d += n; s += n;
        while (n--) *--d = *--s;
    }
    return dst;
}

static inline int uiox_memcmp(const void *a, const void *b, size_t n)
{
    const unsigned char *pa = (const unsigned char *)a;
    const unsigned char *pb = (const unsigned char *)b;
    while (n--) {
        if (*pa != *pb) return (int)*pa - (int)*pb;
        pa++; pb++;
    }
    return 0;
}

/* ══════════════════════════════════════════════════════════════════════
 * §6  String operations                           (replaces <string.h>)
 * ══════════════════════════════════════════════════════════════════════ */

static inline size_t uiox_strlen(const char *s)
{
    const char *p = s;
    while (*p) p++;
    return (size_t)(p - s);
}

static inline size_t uiox_strnlen(const char *s, size_t max)
{
    size_t n = 0;
    while (n < max && s[n]) n++;
    return n;
}

static inline int uiox_strcmp(const char *a, const char *b)
{
    while (*a && *a == *b) { a++; b++; }
    return (unsigned char)*a - (unsigned char)*b;
}

static inline int uiox_strncmp(const char *a, const char *b, size_t n)
{
    while (n && *a && *a == *b) { a++; b++; n--; }
    if (!n) return 0;
    return (unsigned char)*a - (unsigned char)*b;
}

static inline char *uiox_strcpy(char *dst, const char *src)
{
    char *d = dst;
    while ((*d++ = *src++));
    return dst;
}

static inline char *uiox_strncpy(char *dst, const char *src, size_t n)
{
    size_t i;
    for (i = 0; i < n && src[i]; i++) dst[i] = src[i];
    for (; i < n; i++) dst[i] = '\0';
    return dst;
}

static inline const char *uiox_strchr(const char *s, int c)
{
    while (*s) { if (*s == (char)c) return s; s++; }
    return ((char)c == '\0') ? s : (const char *)NULL;
}

/* ══════════════════════════════════════════════════════════════════════
 * §7  Kernel printf                                (replaces <stdio.h>)
 *
 *     uiox_printf() — declared here, implemented in BSP SoC stdio
 *     (10BSP/03SoC/src/uioxsocstdio.c, already freestanding).
 *     The #define printf alias means no source edits are needed.
 * ══════════════════════════════════════════════════════════════════════ */
int uiox_printf(const char *fmt, ...)
    __attribute__((format(printf, 1, 2)));

int uiox_snprintf(char *buf, size_t size, const char *fmt, ...)
    __attribute__((format(printf, 3, 4)));

/* Alias — existing printf() calls compile unchanged */
#ifndef printf
#  define printf  uiox_printf
#endif
#ifndef snprintf
#  define snprintf uiox_snprintf
#endif

/* ══════════════════════════════════════════════════════════════════════
 * §8  Integer math helpers                          (replaces <math.h>)
 *     Integer-only — no FPU dependency in freestanding kernel builds.
 * ══════════════════════════════════════════════════════════════════════ */

static inline int32_t uiox_abs32(int32_t x) { return x < 0 ? -x : x; }
static inline int64_t uiox_abs64(int64_t x) { return x < 0 ? -x : x; }

/* Type-safe min/max via GCC statement expressions */
#define uiox_min(a, b) \
    ({ __typeof__(a) _a = (a); __typeof__(b) _b = (b); _a < _b ? _a : _b; })
#define uiox_max(a, b) \
    ({ __typeof__(a) _a = (a); __typeof__(b) _b = (b); _a > _b ? _a : _b; })

static inline uint32_t uiox_ilog2(uint64_t v)
{
    uint32_t r = 0;
    while (v >>= 1) r++;
    return r;
}

/* ══════════════════════════════════════════════════════════════════════
 * §9  Compatibility macro aliases
 *     Existing .c files use bare names.  These macros route them to the
 *     freestanding implementations above — no source edits required.
 * ══════════════════════════════════════════════════════════════════════ */
#undef memset
#define memset    uiox_memset
#undef memcpy
#define memcpy    uiox_memcpy
#undef memmove
#define memmove   uiox_memmove
#undef memcmp
#define memcmp    uiox_memcmp
#undef strlen
#define strlen    uiox_strlen
#undef strnlen
#define strnlen   uiox_strnlen
#undef strcmp
#define strcmp    uiox_strcmp
#undef strncmp
#define strncmp   uiox_strncmp
#undef strcpy
#define strcpy    uiox_strcpy
#undef strncpy
#define strncpy   uiox_strncpy
#undef strchr
#define strchr    uiox_strchr
#undef abs
#define abs(x)    uiox_abs32((int32_t)(x))

#ifdef __cplusplus
}
#endif
#endif /* UIOX_KLIBC_H */
