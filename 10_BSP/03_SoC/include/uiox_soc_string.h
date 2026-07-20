/**
 * @file    uiox_soc_string.h
 * @brief   UIOX SoC — bare-metal string function replacements.
 *
 * Replaces #include <string.h> in all SoC backend files.
 * No libc dependency.
 *
 * Provided functions (same signatures as their libc counterparts):
 *   uiox_strncpy  — bounded string copy, always NUL-terminates
 *   uiox_strlen   — string length
 *   uiox_memcpy   — byte copy
 *   uiox_memset   — byte fill
 *   uiox_memcmp   — byte compare
 *   uiox_memcpy_u32 — 32-bit-wide copy (for CPUID vendor string)
 *
 * Drop-in macros remap the libc names so no source edits are needed
 * beyond replacing the #include.
 *
 * @version 1.0.0
 * @date    2026-07-18
 */

 #ifndef UIOX_SOC_STRING_H
 #define UIOX_SOC_STRING_H
 
 #include "uiox_base_types.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* ── Implementations ─────────────────────────────────────────────────── */
 
 /**
  * Copy at most @n-1 bytes from @src to @dst and always NUL-terminate.
  * Returns @dst.
  */
 static inline char *uiox_strncpy(char *dst, const char *src, uiox_size_t n)
 {
     uiox_size_t i = 0u;
     if (n == 0u) return dst;
     while (i < n - 1u && src[i]) { dst[i] = src[i]; i++; }
     dst[i] = '\0';
     return dst;
 }
 
 /**
  * Return the number of characters in @s before the terminating NUL.
  */
 static inline uiox_size_t uiox_strlen(const char *s)
 {
     uiox_size_t n = 0u;
     while (*s++) n++;
     return n;
 }
 
 /**
  * Copy @n bytes from @src to @dst (no overlap checking).
  * Returns @dst.
  */
 static inline void *uiox_memcpy(void *dst, const void *src, uiox_size_t n)
 {
     uiox_uint8_t       *d = (uiox_uint8_t *)dst;
     const uiox_uint8_t *s = (const uiox_uint8_t *)src;
     while (n--) *d++ = *s++;
     return dst;
 }
 
 /**
  * Fill @n bytes of @dst with byte value @c.
  * Returns @dst.
  */
 static inline void *uiox_memset(void *dst, int c, uiox_size_t n)
 {
     uiox_uint8_t *d = (uiox_uint8_t *)dst;
     while (n--) *d++ = (uiox_uint8_t)c;
     return dst;
 }
 
 /**
  * Compare @n bytes of @a and @b.
  * Returns 0 if equal, <0 if a < b, >0 if a > b.
  */
 static inline int uiox_memcmp(const void *a, const void *b, uiox_size_t n)
 {
     const uiox_uint8_t *p = (const uiox_uint8_t *)a;
     const uiox_uint8_t *q = (const uiox_uint8_t *)b;
     while (n--) {
         if (*p != *q) return (int)*p - (int)*q;
         p++; q++;
     }
     return 0;
 }
 
 /**
  * Copy 4 bytes from a uint32_t into a char array at @offset.
  * Used by x86 backend to build the CPUID vendor string.
  */
 static inline void uiox_memcpy_u32(char *dst, const uiox_uint32_t *src)
 {
     uiox_memcpy(dst, src, 4u);
 }
 
 /* ── Drop-in macros — remap libc names ──────────────────────────────── */
 #define strncpy(d, s, n)   uiox_strncpy((d), (s), (n))
 #define strlen(s)          uiox_strlen(s)
 #define memcpy(d, s, n)    uiox_memcpy((d), (s), (n))
 #define memset(d, c, n)    uiox_memset((d), (c), (n))
 #define memcmp(a, b, n)    uiox_memcmp((a), (b), (n))
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_SOC_STRING_H */
 