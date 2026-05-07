#ifndef UIX_FEATURES_H
#define UIX_FEATURES_H

/* POSIX version */
#define _UIX_POSIX_VERSION     200809L
#define _UIX_POSIX2_VERSION    200809L
#define _UIX_XOPEN_VERSION     700

/* Feature-test macros */
#define UIX_POSIX_C_SOURCE     200809L
#define UIX_XOPEN_SOURCE       700

/* Compiler helpers */
#define UIX_LIKELY(x)    __builtin_expect(!!(x), 1)
#define UIX_UNLIKELY(x)  __builtin_expect(!!(x), 0)
#define UIX_UNUSED(x)    ((void)(x))
#define UIX_PACKED       __attribute__((packed))
#define UIX_ALIGNED(n)   __attribute__((aligned(n)))
#define UIX_NORETURN     __attribute__((noreturn))
#define UIX_WEAK         __attribute__((weak))
#define UIX_INLINE       static inline
#define UIX_RESTRICT     __restrict__

/* Endianness */
#define UIX_LITTLE_ENDIAN 1234
#define UIX_BIG_ENDIAN    4321
#define UIX_BYTE_ORDER    UIX_LITTLE_ENDIAN

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
