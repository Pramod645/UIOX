/*
 *  30_KIX/32_FS/include/uiox_klibc.h
 *
 *  Freestanding C runtime replacement for the UIOX kernel.
 *  Identical in content to 33_PCS/include/uiox_klibc.h — maintained
 *  as a separate copy so 32_FS has no dependency on 33_PCS paths.
 *
 *  ── what changed in v1.1.0 ────────────────────────────────────────────
 *
 *  1. true / false are now guarded.  uiox_base_types.h defines them as
 *     UIOX_TRUE / UIOX_FALSE and this file defined them again, which
 *     -Werror reported as a redefinition.  #ifndef each.
 *
 *  2. The five memory/string functions and uiox_printf now YIELD to
 *     10_BSP/03_SoC when the SoC headers are present.  The two layers
 *     implemented the same names with different signatures:
 *
 *         uiox_klibc.h          uiox_soc_string.h        note
 *         ──────────────        ──────────────────       ────────────────
 *         size_t n              uiox_size_t n            different types
 *         uiox_printf -> int    uiox_printf -> void      different returns
 *
 *     uiox_size_t is `unsigned long` and this file's size_t is
 *     `unsigned long long` — distinct types in C, so no reconciliation
 *     by typedef is possible.  One side must not define them at all.
 *
 *     The SoC layer is the LOWER layer: it is what the kernel calls down
 *     into, and 33_PCS is not available to 32_FS.  So klibc yields, and
 *     the $9 aliases below still map memset/memcpy/... onto the SoC
 *     implementations — callers compile unchanged.
 *
 *     When the SoC headers are NOT included, klibc's own versions are
 *     used and nothing changes for a host-side or unit-test build.
 *
 *  Sections:
 *    §1  Integer types  (uint8_t … uint64_t, intptr_t, uintptr_t, limits)
 *    §2  Boolean        (bool, true, false)
 *    §3  NULL / size_t / ssize_t / ptrdiff_t / offsetof
 *    §4  Time           (clock_t = uint64_t, time_t, uiox_timespec_t, uiox_timeval_t)
 *    §5  Memory         (uiox_memset, uiox_memcpy, uiox_memmove, uiox_memcmp)
 *    §6  String         (uiox_strlen, uiox_strcmp, uiox_strncmp, uiox_strcpy,
 *                        uiox_strncpy, uiox_strchr)
 *    §7  I/O            (uiox_printf — implemented by BSP SoC stdio)
 *    §8  Math           (uiox_min, uiox_max, uiox_abs32/64, uiox_ilog2 — integer only)
 *    §9  Aliases        (memset/memcpy/memmove/memcmp/printf map to uiox_* above)
 *
 *  @version 1.1.0  @date 2026-09-27
 */
#ifndef UIOX_KLIBC_H
#define UIOX_KLIBC_H

/* ── §1  Integer types ──────────────────────────────────────── */
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

#define UINT8_MAX   0xFFU
#define UINT16_MAX  0xFFFFU
#define UINT32_MAX  0xFFFFFFFFU
#define UINT64_MAX  0xFFFFFFFFFFFFFFFFULL
#define INT8_MIN    (-128)
#define INT8_MAX    127
#define INT16_MIN   (-32768)
#define INT16_MAX   32767
#define INT32_MIN   (-2147483648)
#define INT32_MAX   2147483647
#define INT64_MIN   (-9223372036854775807LL - 1)
#define INT64_MAX   9223372036854775807LL

/* ── §2  Boolean ──────────────────────────────────────────────
 * uiox_base_types.h defines true/false as UIOX_TRUE/UIOX_FALSE before
 * this file is reached, so define them only if nobody has. */
#ifndef __cplusplus
#ifndef true
#define true   1
#endif
#ifndef false
#define false  0
#endif
#ifndef __bool_true_false_are_defined
#define __bool_true_false_are_defined 1
#endif
#endif

/* ── §3  Pointer / size types ───────────────────────────────── */
#ifndef NULL
#define NULL ((void *)0)
#endif

typedef uint64_t  size_t;
typedef int64_t   ssize_t;
typedef int64_t   ptrdiff_t;

#ifndef offsetof
#define offsetof(type, member) __builtin_offsetof(type, member)
#endif

/* ── §4  Time types ─────────────────────────────────────────── */
typedef uint64_t clock_t;   /* monotonic tick counter               */
typedef int64_t  time_t;    /* Unix wall-clock seconds              */

typedef struct { int64_t tv_sec; int32_t tv_nsec; } uiox_timespec_t;
typedef struct { int64_t tv_sec; int32_t tv_usec; } uiox_timeval_t;

/* ═════════════════════════════════════════════════════════════════════
 * §5 / §6 / §7  — memory, string and I/O
 *
 * Which implementation is used depends on whether the SoC layer is in
 * the translation unit already.  UIOX_SOC_STRING_H is the guard in
 * 10_BSP/03_SoC/include/uiox_soc_string.h; UIOX_SOC_STDIO_H is the one
 * in uiox_soc_stdio.h.  Both are set by those headers themselves, so
 * this file needs no ordering rule imposed on the caller.
 *
 * SoC present  ->  its versions are used; nothing here is defined.
 * SoC absent   ->  the self-contained versions below are used.
 *
 * The two size_t spellings are NOT interchangeable (unsigned long vs
 * unsigned long long), which is why this is a definition switch rather
 * than a cast or a typedef.
 * ═════════════════════════════════════════════════════════════════════ */

#ifdef UIOX_SOC_STRING_H
  /* The SoC header owns uiox_memset / memcpy / memcmp / strlen / strncpy.
   * Nothing is redeclared here; the §9 aliases below point at them. */
#else

/* ── §5  Memory operations (inline, no FPU, no libc) ────────── */
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
static inline int uiox_memcmp(const void *a, const void *b, size_t n)
{
    const unsigned char *p = (const unsigned char *)a;
    const unsigned char *q = (const unsigned char *)b;
    while (n--) { if (*p != *q) return (int)*p - (int)*q; p++; q++; }
    return 0;
}
#endif /* UIOX_SOC_STRING_H */

/* uiox_memmove and the whole string set other than strlen/strncpy are
 * NOT in the SoC header, so they are always defined here. */
static inline void *uiox_memmove(void *dst, const void *src, size_t n)
{
    unsigned char       *d = (unsigned char *)dst;
    const unsigned char *s = (const unsigned char *)src;
    if (d < s) { while (n--) *d++ = *s++; }
    else if (d > s) { d += n; s += n; while (n--) *--d = *--s; }
    return dst;
}

/* ── §6  String operations ──────────────────────────────────── */
#ifdef UIOX_SOC_STRING_H
  /* uiox_strlen and uiox_strncpy belong to the SoC header. */
#else
static inline size_t uiox_strlen(const char *s)
    { size_t n = 0; while (*s++) n++; return n; }
static inline char *uiox_strncpy(char *d, const char *s, size_t n)
    { char *r = d; while (n-- && (*d++ = *s++)); while (n-- > 0) *d++ = 0; return r; }
#endif /* UIOX_SOC_STRING_H */

static inline int uiox_strcmp(const char *a, const char *b)
    { while (*a && *a == *b) { a++; b++; } return (unsigned char)*a - (unsigned char)*b; }
static inline int uiox_strncmp(const char *a, const char *b, size_t n)
    { while (n-- && *a && *a == *b) { a++; b++; } return n == (size_t)-1 ? 0 : (unsigned char)*a - (unsigned char)*b; }
static inline char *uiox_strcpy(char *d, const char *s)
    { char *r = d; while ((*d++ = *s++)); return r; }
static inline const char *uiox_strchr(const char *s, int c)
    { while (*s) { if (*s == (char)c) return s; s++; } return (c == 0) ? s : NULL; }
static inline char *uiox_strsep(char **sp, char sep)
{
    char *start = *sp;
    if (!start) return NULL;
    char *p = start;
    while (*p && *p != sep) p++;
    if (*p) { *p = '\0'; *sp = p + 1; } else { *sp = NULL; }
    return start;
}

/* ── §7  I/O — DECLARED BY THE SoC LAYER, NOT HERE ────────────────────
 * uiox_printf, uiox_puts, uiox_putc and uiox_snprintf all belong to
 * 10_BSP/03_SoC/include/uiox_soc_stdio.h, which declares
 *
 *     void uiox_printf(const char *fmt, ...)
 *
 * and defines the printf / snprintf / puts macros itself.
 *
 * This file must NOT declare it.  A guard keyed on UIOX_SOC_STDIO_H is
 * wrong here, because the ORDER is not guaranteed: bcache.h includes
 * this header, and this header may therefore be reached BEFORE
 * uiox_soc_stdio.h.  The guard would not yet be set, this file's
 * declaration would win, and the later SoC one would conflict —
 *
 *     uiox_klibc.h:164: conflicting types for 'uiox_printf';
 *       have 'int(const char *, ...)'
 *     uiox_soc_stdio.h:34: previous declaration with type
 *       'void(const char *, ...)'
 *
 * which is exactly the error this comment replaces.
 *
 * The two return types cannot be reconciled: int vs void is not a
 * typedef difference.  And it is not klibc's to choose — the SoC layer
 * is the lower layer, it is what the kernel calls down into, and it
 * owns the name outright.
 *
 * So: nothing is declared here, and the §9 printf alias is gone too,
 * because uiox_soc_stdio.h already has
 *     #define printf(...) uiox_printf(__VA_ARGS__)
 * A second alias would re-define an existing macro. */

/* ── §8  Integer math (no FPU) ─────────────────────────────── */
#define uiox_min(a, b)    ((a) < (b) ? (a) : (b))
#define uiox_max(a, b)    ((a) > (b) ? (a) : (b))
#define uiox_abs32(x)     ((int32_t)(x) < 0 ? -(int32_t)(x) : (int32_t)(x))
#define uiox_abs64(x)     ((int64_t)(x) < 0 ? -(int64_t)(x) : (int64_t)(x))
static inline int uiox_ilog2(uint64_t v)
    { int n = 0; while (v >>= 1) n++; return n; }

/* ── §9  Aliases (zero-cost: macros → uiox_* above) ──────────
 * Whichever side defined the functions, the bare C names land on the
 * uiox_ ones, so callers compile unchanged either way.
 *
 * printf / snprintf / puts are NOT aliased here — uiox_soc_stdio.h
 * defines those macros itself. */
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
/* stderr / fprintf do not exist in a freestanding build.
 * Any fprintf(stderr,...) must be replaced with printf(...).   */

#endif /* UIOX_KLIBC_H */
