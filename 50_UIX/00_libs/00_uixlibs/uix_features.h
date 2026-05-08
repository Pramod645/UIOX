#ifndef UIX_FEATURES_H
#define UIX_FEATURES_H

/* POSIX version */
#define _UIX_POSIX_VERSION     200809L // Declares POSIX.1-2008 compliance — glibc uses _POSIX_VERSION similarly
#define _UIX_POSIX2_VERSION    200809L
#define _UIX_XOPEN_VERSION     700

/* Feature-test macros */
#define UIX_POSIX_C_SOURCE     200809L
#define UIX_XOPEN_SOURCE       700

/* Compiler helpers */
#define UIX_LIKELY(x)    __builtin_expect(!!(x), 1) // GCC branch-prediction hint — marks condition as likely true, used throughout glibc
#define UIX_UNLIKELY(x)  __builtin_expect(!!(x), 0) // Marks condition as unlikely — used in error path optimization
#define UIX_UNUSED(x)    ((void)(x))
#define UIX_PACKED       __attribute__((packed)) // Removes struct padding — used in network protocol structs per POSIX socket headers
#define UIX_ALIGNED(n)   __attribute__((aligned(n))) // Forces memory alignment — required for SIMD and hardware register access
#define UIX_NORETURN     __attribute__((noreturn)) // Marks function as never returning — POSIX requires this on exit(), abort()
#define UIX_WEAK         __attribute__((weak))
#define UIX_INLINE       static inline
#define UIX_RESTRICT     __restrict__

/* Endianness */
#define UIX_LITTLE_ENDIAN 1234 // Byte order constant matching glibc's <endian.h>
#define UIX_BIG_ENDIAN    4321
#define UIX_BYTE_ORDER    UIX_LITTLE_ENDIAN // Runtime endianness selection — affects htons(), ntohl() behavior

/* Architecture */
#if defined(__x86_64__)
#  define UIX_ARCH_X86_64  1
#  define UIX_WORDSIZE      64
#elif defined(__i386__)
#  define UIX_ARCH_X86     1
#  define UIX_WORDSIZE      32
#elif defined(__aarch64__)
#  define UIX_ARCH_ARM64   1
#  define UIX_WORDSIZE      64
#endif

#endif /* UIX_FEATURES_H */
