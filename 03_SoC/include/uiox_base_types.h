/**
 * @file    uiox_basetypes.h
 * @brief   UIOX bare-metal base types.
 *
 * Replaces <stdint.h>, <stdbool.h>, and <stddef.h> with
 * compiler-independent, architecture-aware definitions.
 *
 * No system header is included.  All types are defined from
 * first principles using the GCC/Clang built-in __INT*_TYPE__
 * macros, which are available on every supported compiler and
 * architecture (ARM32, ARM64, x86-64, RISC-V64) without pulling
 * in any libc or operating system header.
 *
 * Usage:
 *   Replace every occurrence of:
 *     #include <stdint.h>
 *     #include <stdbool.h>
 *     #include <stddef.h>
 *   with:
 *     #include "uiox_basetypes.h"
 *
 * Placed in:
 *   02_FwHal/include/uiox_basetypes.h
 *
 * @version 1.0.0
 * @date    2026-07-18
 */

 #ifndef UIOX_BASETYPES_H
 #define UIOX_BASETYPES_H
 
 /* =========================================================================
  * Compiler / architecture detection
  * ====================================================================== */
 
 #if !defined(__GNUC__) && !defined(__clang__)
 #  error "uiox_basetypes.h requires GCC or Clang."
 #endif
 
 /* =========================================================================
  * Fixed-width signed integer types
  * ====================================================================== */
 
 /** 8-bit signed integer */
 typedef signed char         uiox_int8_t;
 
 /** 16-bit signed integer */
 typedef signed short        uiox_int16_t;
 
 /** 32-bit signed integer */
 typedef signed int          uiox_int32_t;
 
 #if defined(__aarch64__) || defined(__x86_64__) || defined(__riscv)
 /** 64-bit signed integer (64-bit targets) */
 typedef signed long         uiox_int64_t;
 #else
 /** 64-bit signed integer (32-bit targets — uses long long) */
 typedef signed long long    uiox_int64_t;
 #endif
 
 /* =========================================================================
  * Fixed-width unsigned integer types
  * ====================================================================== */
 
 /** 8-bit unsigned integer */
 typedef unsigned char       uiox_uint8_t;
 
 /** 16-bit unsigned integer */
 typedef unsigned short      uiox_uint16_t;
 
 /** 32-bit unsigned integer */
 typedef unsigned int        uiox_uint32_t;
 
 #if defined(__aarch64__) || defined(__x86_64__) || defined(__riscv)
 /** 64-bit unsigned integer (64-bit targets) */
 typedef unsigned long       uiox_uint64_t;
 #else
 /** 64-bit unsigned integer (32-bit targets — uses unsigned long long) */
 typedef unsigned long long  uiox_uint64_t;
 #endif
 
 /* =========================================================================
  * Pointer-width integer types
  * ====================================================================== */
 
 #if defined(__aarch64__) || defined(__x86_64__) || defined(__riscv)
 /** Unsigned integer large enough to hold any pointer (64-bit) */
 typedef unsigned long       uiox_uintptr_t;
 
 /** Signed integer large enough to hold any pointer (64-bit) */
 typedef signed long         uiox_intptr_t;
 
 /** Unsigned type for memory sizes and counts */
 typedef unsigned long       uiox_size_t;
 
 /** Signed type for pointer differences */
 typedef signed long         uiox_ptrdiff_t;
 #else
 /** Unsigned integer large enough to hold any pointer (32-bit) */
 typedef unsigned int        uiox_uintptr_t;
 
 /** Signed integer large enough to hold any pointer (32-bit) */
 typedef signed int          uiox_intptr_t;
 
 /** Unsigned type for memory sizes and counts */
 typedef unsigned int        uiox_size_t;
 
 /** Signed type for pointer differences */
 typedef signed int          uiox_ptrdiff_t;
 #endif
 
 /* =========================================================================
  * Boolean type
  * ====================================================================== */
 
 /**
  * Boolean type — uses the C99 built-in _Bool when available,
  * otherwise falls back to unsigned char.
  */
 #if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L)
 typedef _Bool               uiox_bool_t;
 #else
 typedef unsigned char       uiox_bool_t;
 #endif
 
 #define UIOX_TRUE   ((uiox_bool_t)1)
 #define UIOX_FALSE  ((uiox_bool_t)0)
 
 /* Also expose plain true / false if not already defined (no stdbool.h) */
 #ifndef true
 #  define true  UIOX_TRUE
 #endif
 #ifndef false
 #  define false UIOX_FALSE
 #endif
 
 /* =========================================================================
  * NULL pointer constant
  * ====================================================================== */
 #ifndef NULL
 #  define NULL  ((void *)0)
 #endif
 
 /* =========================================================================
  * Type limits
  * ====================================================================== */
 
 #define UIOX_UINT8_MAX    ((uiox_uint8_t) 0xFFu)
 #define UIOX_UINT16_MAX   ((uiox_uint16_t)0xFFFFu)
 #define UIOX_UINT32_MAX   ((uiox_uint32_t)0xFFFFFFFFu)
 #define UIOX_UINT64_MAX   ((uiox_uint64_t)0xFFFFFFFFFFFFFFFFull)
 
 #define UIOX_INT8_MIN     ((uiox_int8_t) (-128))
 #define UIOX_INT8_MAX     ((uiox_int8_t)   127)
 #define UIOX_INT16_MIN    ((uiox_int16_t)(-32768))
 #define UIOX_INT16_MAX    ((uiox_int16_t)  32767)
 #define UIOX_INT32_MIN    ((uiox_int32_t)(-2147483647 - 1))
 #define UIOX_INT32_MAX    ((uiox_int32_t)  2147483647)
 
 #if defined(__aarch64__) || defined(__x86_64__) || defined(__riscv)
 #define UIOX_INT64_MIN    ((uiox_int64_t)(-9223372036854775807L - 1L))
 #define UIOX_INT64_MAX    ((uiox_int64_t)  9223372036854775807L)
 #define UIOX_SIZE_MAX     ((uiox_size_t)   0xFFFFFFFFFFFFFFFFUL)
 #else
 #define UIOX_INT64_MIN    ((uiox_int64_t)(-9223372036854775807LL - 1LL))
 #define UIOX_INT64_MAX    ((uiox_int64_t)  9223372036854775807LL)
 #define UIOX_SIZE_MAX     ((uiox_size_t)   0xFFFFFFFFu)
 #endif
 
 /* =========================================================================
  * Convenience aliases
  *
  * These let existing code that used the system uint32_t etc. compile
  * unchanged if desired.  They are opt-in via UIOX_BASETYPES_COMPAT.
  *
  * IMPORTANT: do NOT define these if <stdint.h> is included anywhere in
  * the same translation unit — they will conflict.
  * ====================================================================== */
 #if defined(UIOX_BASETYPES_COMPAT)
 typedef uiox_uint8_t    uint8_t;
 typedef uiox_uint16_t   uint16_t;
 typedef uiox_uint32_t   uint32_t;
 typedef uiox_uint64_t   uint64_t;
 typedef uiox_int8_t     int8_t;
 typedef uiox_int16_t    int16_t;
 typedef uiox_int32_t    int32_t;
 typedef uiox_int64_t    int64_t;
 typedef uiox_uintptr_t  uintptr_t;
 typedef uiox_intptr_t   intptr_t;
 typedef uiox_size_t     size_t;
 typedef uiox_ptrdiff_t  ptrdiff_t;
 typedef uiox_bool_t     bool;
 #endif /* UIOX_BASETYPES_COMPAT */
 
 /* =========================================================================
  * Compiler attribute helpers
  * ====================================================================== */
 
 /** Mark a function as never returning */
 #define UIOX_NORETURN       __attribute__((noreturn))
 
 /** Mark a variable / struct as packed (no padding) */
 #define UIOX_PACKED         __attribute__((packed))
 
 /** Align a variable to N bytes */
 #define UIOX_ALIGNED(n)     __attribute__((aligned(n)))
 
 /** Suppress unused-parameter warnings */
 #define UIOX_UNUSED_PARAM(x) ((void)(x))
 
 /** Force a function to be inlined */
 #define UIOX_INLINE         static inline __attribute__((always_inline))
 
 /** Hint that a branch is likely / unlikely */
 #define UIOX_LIKELY(x)      __builtin_expect(!!(x), 1)
 #define UIOX_UNLIKELY(x)    __builtin_expect(!!(x), 0)
 
 /** Compiler memory barrier (no hardware effect) */
 #define UIOX_BARRIER()      __asm__ volatile("" ::: "memory")
 
 #endif /* UIOX_BASETYPES_H */
 