/*
 *  30_KIX/common/uiox_klibc.h
 *
 *  Single shared freestanding C runtime replacement for the UIOX kernel.
 *  Placed in 30_KIX/common/ so every subsystem references the same file.
 *
 *  All subsystem Makefiles add -I$(MFDIR)../common  (or -I$(COMMONDIR))
 *  to their COMMON_INCLUDES.  Individual subsystem copies are no longer
 *  needed — delete 20_DriverInterfaces/include/uiox_klibc.h etc.
 *
 *  Replaces:  <stdint.h>  <stdbool.h>  <stddef.h>
 *             <string.h>  <stdlib.h>   <stdio.h>
 *             <errno.h>   <time.h>
 *
 *  §1   Integer types
 *  §2   Boolean
 *  §3   NULL / size_t / ssize_t / ptrdiff_t / offsetof
 *  §4   Time  (clock_t, time_t)
 *  §5   Memory  (memset, memcpy, memmove, memcmp)
 *  §6   String  (strlen, strcmp, strncmp, strcpy, strncpy, strchr, strsep)
 *  §7   I/O     (printf → uiox_printf, implemented by BSP SoC stdio)
 *  §8   Math    (min, max, abs — integer only, no FPU)
 *  §9   Error codes  (replaces <errno.h>)
 *  §10  Aliases (memset/memcpy/printf/… map to uiox_* above)
 */
#ifndef UIOX_KLIBC_H
#define UIOX_KLIBC_H

#define EPERM     1   /* Operation not permitted     */
#define ENOENT    2   /* No such file or directory   */
#define EIO       5   /* I/O error                   */
#define ENXIO     6   /* No such device or address   */
#define ENOMEM   12   /* Out of memory               */
#define EACCES   13   /* Permission denied           */
#define EFAULT   14   /* Bad address                 */
#define EBUSY    16   /* Device or resource busy     */
#define EEXIST   17   /* File exists                 */
#define ENODEV   19   /* No such device              */
#define EINVAL   22   /* Invalid argument            */
#define ENOSPC   28   /* No space left on device     */
#define EPIPE    32   /* Broken pipe                 */
#define ERANGE   34   /* Math result not representable */
#define EAGAIN   11   /* Try again                   */
#define EWOULDBLOCK EAGAIN
#define ENOSYS   38   /* Function not implemented    */
#define ENOTSUP  95   /* Operation not supported     */
#define ETIMEDOUT 110 /* Connection timed out        */
#define EBADMSG 74
#define ENETDOWN 100
#define ENOBUFS 105
#define EPROTO 71

/* ── §1  Integer types ──────────────────────────────────────────────────────── */
typedef unsigned char       uint8_t;
typedef unsigned short      uint16_t;
typedef unsigned int        uint32_t;
typedef unsigned long long  uint64_t;
typedef signed char         int8_t;
typedef short               int16_t;
typedef int                 int32_t;
typedef long long           int64_t;
typedef uint64_t            uintptr_t;
typedef int64_t             intptr_t;
typedef uint64_t            uintmax_t;
typedef int64_t             intmax_t;

#define UINT8_MAX    0xFFU
#define UINT16_MAX   0xFFFFU
#define UINT32_MAX   0xFFFFFFFFU
#define UINT64_MAX   0xFFFFFFFFFFFFFFFFULL
#define INT8_MIN     (-128)
#define INT8_MAX     127
#define INT16_MIN    (-32768)
#define INT16_MAX    32767
#define INT32_MIN    (-2147483648)
#define INT32_MAX    2147483647
#define INT64_MIN    (-9223372036854775807LL - 1)
#define INT64_MAX    9223372036854775807LL

/* ── §2  Boolean ────────────────────────────────────────────────────────────── */
#ifndef __cplusplus
typedef _Bool bool;
#define true  1
#define false 0
#endif

/* ── §3  Pointer / size types ───────────────────────────────────────────────── */
#ifndef NULL
#define NULL ((void *)0)
#endif

typedef uint64_t  size_t;
typedef int64_t   ssize_t;
typedef int64_t   ptrdiff_t;

#define offsetof(type, member) __builtin_offsetof(type, member)

/* ── §4  Time types ─────────────────────────────────────────────────────────── */
typedef uint64_t clock_t;
typedef int64_t  time_t;

typedef struct { int64_t tv_sec; int32_t tv_nsec; } uiox_timespec_t;
typedef struct { int64_t tv_sec; int32_t tv_usec; } uiox_timeval_t;

/* ── §5  Memory operations (inline, freestanding) ───────────────────────────── */
static inline void *uiox_memset(void *s, int c, size_t n)
{
    unsigned char *p = (unsigned char *)s;
    while (n--) *p++ = (unsigned char)c;
    return s;
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
    if (d < s) { while (n--) *d++ = *s++; }
    else if (d > s) { d += n; s += n; while (n--) *--d = *--s; }
    return dst;
}
static inline int uiox_memcmp(const void *a, const void *b, size_t n)
{
    const unsigned char *p = (const unsigned char *)a;
    const unsigned char *q = (const unsigned char *)b;
    while (n--) { if (*p != *q) return (int)*p - (int)*q; p++; q++; }
    return 0;
}

/* ── §6  String operations ──────────────────────────────────────────────────── */
static inline size_t uiox_strlen(const char *s)
    { size_t n = 0; while (*s++) n++; return n; }
static inline int uiox_strcmp(const char *a, const char *b)
    { while (*a && *a == *b) { a++; b++; }
      return (unsigned char)*a - (unsigned char)*b; }
static inline int uiox_strncmp(const char *a, const char *b, size_t n)
    { while (n-- && *a && *a == *b) { a++; b++; }
      return n == (size_t)-1 ? 0 : (unsigned char)*a - (unsigned char)*b; }
static inline char *uiox_strcpy(char *d, const char *s)
    { char *r = d; while ((*d++ = *s++)); return r; }
static inline char *uiox_strncpy(char *d, const char *s, size_t n)
    { char *r = d; while (n-- && (*d++ = *s++)); while (n-- > 0) *d++ = 0; return r; }
static inline const char *uiox_strchr(const char *s, int c)
    { while (*s) { if (*s == (char)c) return s; s++; }
      return (c == 0) ? s : NULL; }
static inline char *uiox_strsep(char **sp, char sep)
{
    char *start = *sp;
    if (!start) return NULL;
    char *p = start;
    while (*p && *p != sep) p++;
    if (*p) { *p = '\0'; *sp = p + 1; } else { *sp = NULL; }
    return start;
}

/* ── §7  I/O ────────────────────────────────────────────────────────────────── */
/* Implemented by the BSP SoC stdio layer. */
extern int uiox_printf(const char *fmt, ...)
    __attribute__((format(printf, 1, 2)));

/* stderr / fprintf do not exist in a freestanding build.
 * Any fprintf(stderr,...) must be replaced with printf(...)         */

/* ── §8  Integer math (no FPU) ──────────────────────────────────────────────── */
#define uiox_min(a, b)    ((a) < (b) ? (a) : (b))
#define uiox_max(a, b)    ((a) > (b) ? (a) : (b))
#define uiox_abs32(x)     ((int32_t)(x) < 0 ? -(int32_t)(x) : (int32_t)(x))
#define uiox_abs64(x)     ((int64_t)(x) < 0 ? -(int64_t)(x) : (int64_t)(x))
static inline int uiox_ilog2(uint64_t v)
    { int n = 0; while (v >>= 1) n++; return n; }

/* ── §9  Error codes (replaces <errno.h>) ───────────────────────────────────── */
#ifndef EPERM
#define EPERM     1   /* Operation not permitted          */
#define ENOENT    2   /* No such file or directory        */
#define EIO       5   /* I/O error                        */
#define ENXIO     6   /* No such device or address        */
#define ENOMEM   12   /* Out of memory                    */
#define EACCES   13   /* Permission denied                */
#define EFAULT   14   /* Bad address                      */
#define EBUSY    16   /* Device or resource busy          */
#define EEXIST   17   /* File exists                      */
#define ENODEV   19   /* No such device                   */
#define EINVAL   22   /* Invalid argument                 */
#define ENOSPC   28   /* No space left on device          */
#define EPIPE    32   /* Broken pipe                      */
#define ERANGE   34   /* Math result not representable    */
#define EAGAIN   11   /* Try again                        */
#define EWOULDBLOCK EAGAIN
#define ENOSYS   38   /* Function not implemented         */
#define ENOTSUP  95   /* Operation not supported          */
#define ETIMEDOUT 110 /* Connection timed out             */
#endif /* EPERM */

/* ── §10  Aliases (zero-cost macro redirects) ───────────────────────────────── */
#undef  memset
#define memset   uiox_memset
#undef  memcpy
#define memcpy   uiox_memcpy
#undef  memmove
#define memmove  uiox_memmove
#undef  memcmp
#define memcmp   uiox_memcmp
#undef  strlen
#define strlen   uiox_strlen
#undef  strcmp
#define strcmp   uiox_strcmp
#undef  strncmp
#define strncmp  uiox_strncmp
#undef  strcpy
#define strcpy   uiox_strcpy
#undef  strncpy
#define strncpy  uiox_strncpy
#undef  printf
#define printf   uiox_printf

#endif /* UIOX_KLIBC_H */
