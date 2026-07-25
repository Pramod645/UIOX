/**
 * @file  uiox_fw_types.h
 * @brief UIOX Firmware HAL -- all primitive types, error codes, macros.
 *        SELF-CONTAINED: uses only compiler built-ins, zero system headers.
 *
 * FIX: All macro definitions now wrapped with #ifndef guards so this header
 * can be included after uiox_base_types.h or uiox_boot_types.h without
 * triggering -Werror=redefine.  The typedef block is unchanged (already
 * protected by the outer UIOX_FW_TYPES_H guard).
 *
 * FIX: uiox_memcpy() body reformatted to eliminate -Werror=misleading-indentation.
 */
#ifndef UIOX_FW_TYPES_H
#define UIOX_FW_TYPES_H

typedef __INT8_TYPE__    int8_t;
typedef __INT16_TYPE__   int16_t;
typedef __INT32_TYPE__   int32_t;
typedef __INT64_TYPE__   int64_t;
typedef __UINT8_TYPE__   uint8_t;
typedef __UINT16_TYPE__  uint16_t;
typedef __UINT32_TYPE__  uint32_t;
typedef __UINT64_TYPE__  uint64_t;
typedef __INTPTR_TYPE__  intptr_t;
typedef __UINTPTR_TYPE__ uintptr_t;
typedef __SIZE_TYPE__    size_t;
typedef __PTRDIFF_TYPE__ ptrdiff_t;

#ifndef __cplusplus
typedef _Bool bool;
# ifndef true
#  define true  ((bool)1)
# endif
# ifndef false
#  define false ((bool)0)
# endif
#endif

#ifndef NULL
# define NULL ((void *)0)
#endif

/* =========================================================================
 * Magic numbers
 * ====================================================================== */
#define UIOX_FW_MAGIC           0x55494F58u  /**< "UIOX"                  */
#define UIOX_FW_DEVSW_MAGIC     0x44455357u  /**< "DESW"                  */
#define UIOX_FW_VERSION         0x00010000u  /**< v1.0.0                  */

/* Error type */
typedef int32_t uiox_fw_err_t;
#define UIOX_FW_OK                ((uiox_fw_err_t)  0)
#define UIOX_FW_ERR_GENERIC       ((uiox_fw_err_t) -1)
#define UIOX_FW_ERR_INVAL         ((uiox_fw_err_t) -2)
#define UIOX_FW_ERR_NOMEM         ((uiox_fw_err_t) -3)
#define UIOX_FW_ERR_TIMEOUT       ((uiox_fw_err_t) -4)
#define UIOX_FW_ERR_BUSY          ((uiox_fw_err_t) -5)
#define UIOX_FW_ERR_IO            ((uiox_fw_err_t) -6)
#define UIOX_FW_ERR_NODEV         ((uiox_fw_err_t) -7)
#define UIOX_FW_ERR_NOINIT        ((uiox_fw_err_t) -8)
#define UIOX_FW_ERR_OVERFLOW      ((uiox_fw_err_t) -9)
#define UIOX_FW_ERR_UNDERFLOW     ((uiox_fw_err_t)-10)
#define UIOX_FW_ERR_NOT_SUPPORTED ((uiox_fw_err_t)-11)
#define UIOX_FW_ERR_UNSUP         UIOX_FW_ERR_NOT_SUPPORTED
#define UIOX_FW_ERR_PERM          ((uiox_fw_err_t)-12)
#define UIOX_FW_ERR_NACK          ((uiox_fw_err_t)-13)
#define UIOX_FW_ERR_ARB_LOST      ((uiox_fw_err_t)-14)
#define UIOX_FW_ERR_CRC           ((uiox_fw_err_t)-15)
#define UIOX_FW_ERR_BADMAGIC      ((uiox_fw_err_t)-16)

/* =========================================================================
 * Compiler attributes — all guarded with #ifndef so a prior header
 * (uiox_base_types.h, uiox_boot_types.h) wins without a redefinition error.
 * ====================================================================== */
#if defined(__GNUC__) || defined(__clang__)
# ifndef UIOX_UNUSED
#  define UIOX_UNUSED      __attribute__((unused))
# endif
# ifndef UIOX_WEAK
#  define UIOX_WEAK        __attribute__((weak))
# endif
# ifndef UIOX_PACKED
#  define UIOX_PACKED      __attribute__((packed))
# endif
# ifndef UIOX_NORETURN
#  define UIOX_NORETURN    __attribute__((noreturn))
# endif
# ifndef UIOX_INLINE
#  define UIOX_INLINE      __attribute__((always_inline)) static inline
# endif
# ifndef UIOX_LIKELY
#  define UIOX_LIKELY(x)   __builtin_expect(!!(x),1)
# endif
# ifndef UIOX_UNLIKELY
#  define UIOX_UNLIKELY(x) __builtin_expect(!!(x),0)
# endif
#else
# ifndef UIOX_UNUSED
#  define UIOX_UNUSED
# endif
# ifndef UIOX_WEAK
#  define UIOX_WEAK
# endif
# ifndef UIOX_PACKED
#  define UIOX_PACKED
# endif
# ifndef UIOX_NORETURN
#  define UIOX_NORETURN
# endif
# ifndef UIOX_INLINE
#  define UIOX_INLINE      static inline
# endif
# ifndef UIOX_LIKELY
#  define UIOX_LIKELY(x)   (x)
# endif
# ifndef UIOX_UNLIKELY
#  define UIOX_UNLIKELY(x) (x)
# endif
#endif

/* Suppress unused-variable warning */
#define UIOX_FW_UNUSED(x)   ((void)(x))
#define UIOX_UNUSED_VAR(x)  ((void)(x))

/* Bit macro */
#define UIOX_FW_BIT(n)  (1u << (n))

/* =========================================================================
 * Integer limits — guarded so uiox_base_types.h definitions win if earlier.
 * ====================================================================== */
#ifndef UIOX_UINT8_MAX
# define UIOX_UINT8_MAX   ((uint8_t) 0xFFu)
#endif
#ifndef UIOX_UINT16_MAX
# define UIOX_UINT16_MAX  ((uint16_t)0xFFFFu)
#endif
#ifndef UIOX_UINT32_MAX
# define UIOX_UINT32_MAX  ((uint32_t)0xFFFFFFFFu)
#endif
#ifndef UIOX_INT32_MIN
# define UIOX_INT32_MIN   ((int32_t)(-2147483647-1))
#endif
#ifndef UIOX_INT32_MAX
# define UIOX_INT32_MAX   ((int32_t) 2147483647)
#endif

/* =========================================================================
 * Utility macros — guarded so boot/soc headers win if already defined.
 * ====================================================================== */
#ifndef UIOX_ARRAY_SIZE
# define UIOX_ARRAY_SIZE(a)   (sizeof(a)/sizeof((a)[0]))
#endif
#ifndef UIOX_MIN
# define UIOX_MIN(a,b)        ((a)<(b)?(a):(b))
#endif
#ifndef UIOX_MAX
# define UIOX_MAX(a,b)        ((a)>(b)?(a):(b))
#endif
#ifndef UIOX_ALIGN_UP
# define UIOX_ALIGN_UP(x,a)   (((uintptr_t)(x)+((a)-1u))&~((uintptr_t)((a)-1u)))
#endif

/* =========================================================================
 * MMIO accessors
 * ====================================================================== */
static inline uint32_t uiox_rd32(uintptr_t a)
{ return *(volatile uint32_t *)a; }
static inline void uiox_wr32(uintptr_t a, uint32_t v)
{ *(volatile uint32_t *)a = v; }
static inline uint64_t uiox_rd64(uintptr_t a)
{ return *(volatile uint64_t *)a; }
static inline void uiox_wr64(uintptr_t a, uint64_t v)
{ *(volatile uint64_t *)a = v; }

static inline void fw_mmio_write32(uintptr_t addr, uint32_t val)
{ *((volatile uint32_t *)addr) = val; }
static inline uint32_t fw_mmio_read32(uintptr_t addr)
{ return *((volatile uint32_t *)addr); }
static inline void fw_mmio_write8(uintptr_t addr, uint8_t val)
{ *((volatile uint8_t *)addr) = val; }
static inline uint8_t fw_mmio_read8(uintptr_t addr)
{ return *((volatile uint8_t *)addr); }
static inline void fw_mmio_write64(uintptr_t addr, uint64_t val)
{ *((volatile uint64_t *)addr) = val; }
static inline uint64_t fw_mmio_read64(uintptr_t addr)
{ return *((volatile uint64_t *)addr); }

/* =========================================================================
 * Memory barriers
 * ====================================================================== */
#if defined(__aarch64__)
# define UIOX_DSB() __asm__ volatile("dsb sy"  :::"memory")
# define UIOX_ISB() __asm__ volatile("isb"     :::"memory")
# define UIOX_DMB() __asm__ volatile("dmb ish" :::"memory")
#elif defined(__arm__)
# define UIOX_DSB() __asm__ volatile("dsb"     :::"memory")
# define UIOX_ISB() __asm__ volatile("isb"     :::"memory")
# define UIOX_DMB() __asm__ volatile("dmb"     :::"memory")
#elif defined(__riscv)
# define UIOX_DSB() __asm__ volatile("fence iorw,iorw" :::"memory")
# define UIOX_ISB() __asm__ volatile("fence.i"         :::"memory")
# define UIOX_DMB() __asm__ volatile("fence rw,rw"     :::"memory")
#else
# define UIOX_DSB() __asm__ volatile("mfence" :::"memory")
# define UIOX_ISB() __asm__ volatile("lfence" :::"memory")
# define UIOX_DMB() __asm__ volatile("mfence" :::"memory")
#endif

/* =========================================================================
 * No-libc memory helpers
 * FIX: uiox_memcpy body split onto separate lines to fix
 *      -Werror=misleading-indentation (return after while on same line).
 * ====================================================================== */
static inline void *uiox_memset(void *d, int c, size_t n)
{
    uint8_t *p = (uint8_t *)d;
    while (n--) *p++ = (uint8_t)c;
    return d;
}

static inline void *uiox_memcpy(void *d, const void *s, size_t n)
{
    uint8_t       *dd = (uint8_t *)d;
    const uint8_t *ss = (const uint8_t *)s;
    while (n--) *dd++ = *ss++;
    return d;
}

static inline int uiox_memcmp(const void *a, const void *b, size_t n)
{
    const uint8_t *p = (const uint8_t *)a;
    const uint8_t *q = (const uint8_t *)b;
    while (n--) {
        if (*p != *q) return (int)*p - (int)*q;
        p++; q++;
    }
    return 0;
}

#endif /* UIOX_FW_TYPES_H */
