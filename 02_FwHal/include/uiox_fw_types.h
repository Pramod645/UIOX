/**
 * @file  uiox_fw_types.h
 * @brief UIOX Firmware HAL -- all primitive types, error codes, macros.
 *        SELF-CONTAINED: uses only compiler built-ins, zero system headers.
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
# define true  ((bool)1)
# define false ((bool)0)
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


/* Compiler attributes */
#if defined(__GNUC__) || defined(__clang__)
# define UIOX_UNUSED      __attribute__((unused))
# define UIOX_WEAK        __attribute__((weak))
# define UIOX_PACKED      __attribute__((packed))
# define UIOX_NORETURN    __attribute__((noreturn))
# define UIOX_INLINE      __attribute__((always_inline)) static inline
# define UIOX_LIKELY(x)   __builtin_expect(!!(x),1)
# define UIOX_UNLIKELY(x) __builtin_expect(!!(x),0)
#else
# define UIOX_UNUSED
# define UIOX_WEAK
# define UIOX_PACKED
# define UIOX_NORETURN
# define UIOX_INLINE      static inline
# define UIOX_LIKELY(x)   (x)
# define UIOX_UNLIKELY(x) (x)
#endif

/* Suppress unused-variable warning */
#define UIOX_FW_UNUSED(x)   ((void)(x))
#define UIOX_UNUSED_VAR(x)  ((void)(x))

/* Bit macro */
#define UIOX_FW_BIT(n)  (1u << (n))

/* Integer limits */
#define UIOX_UINT8_MAX   ((uint8_t) 0xFFu)
#define UIOX_UINT16_MAX  ((uint16_t)0xFFFFu)
#define UIOX_UINT32_MAX  ((uint32_t)0xFFFFFFFFu)
#define UIOX_INT32_MIN   ((int32_t)(-2147483647-1))
#define UIOX_INT32_MAX   ((int32_t) 2147483647)

/* Utilities */
#define UIOX_ARRAY_SIZE(a)   (sizeof(a)/sizeof((a)[0]))
#define UIOX_MIN(a,b)        ((a)<(b)?(a):(b))
#define UIOX_MAX(a,b)        ((a)>(b)?(a):(b))
#define UIOX_ALIGN_UP(x,a)   (((uintptr_t)(x)+((a)-1u))&~((uintptr_t)((a)-1u)))

/* MMIO accessors */
static inline uint32_t uiox_rd32(uintptr_t a){return *(volatile uint32_t*)a;}
static inline void     uiox_wr32(uintptr_t a,uint32_t v){*(volatile uint32_t*)a=v;}
static inline uint64_t uiox_rd64(uintptr_t a){return *(volatile uint64_t*)a;}
static inline void     uiox_wr64(uintptr_t a,uint64_t v){*(volatile uint64_t*)a=v;}

 /* =========================================================================
  * MMIO helpers
  * ====================================================================== */
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

/* Memory barriers */
#if defined(__aarch64__)
# define UIOX_DSB() __asm__ volatile("dsb sy" :::"memory")
# define UIOX_ISB() __asm__ volatile("isb"    :::"memory")
# define UIOX_DMB() __asm__ volatile("dmb ish":::"memory")
#elif defined(__arm__)
# define UIOX_DSB() __asm__ volatile("dsb"    :::"memory")
# define UIOX_ISB() __asm__ volatile("isb"    :::"memory")
# define UIOX_DMB() __asm__ volatile("dmb"    :::"memory")
#elif defined(__riscv)
# define UIOX_DSB() __asm__ volatile("fence iorw,iorw":::"memory")
# define UIOX_ISB() __asm__ volatile("fence.i"        :::"memory")
# define UIOX_DMB() __asm__ volatile("fence rw,rw"    :::"memory")
#else
# define UIOX_DSB() __asm__ volatile("mfence":::"memory")
# define UIOX_ISB() __asm__ volatile("lfence":::"memory")
# define UIOX_DMB() __asm__ volatile("mfence":::"memory")
#endif

/* No-libc memory helpers */
static inline void *uiox_memset(void *d,int c,size_t n)
{uint8_t*p=(uint8_t*)d;while(n--)*p++=(uint8_t)c;return d;}
static inline void *uiox_memcpy(void *d,const void *s,size_t n)
{uint8_t*dd=(uint8_t*)d;const uint8_t*ss=(const uint8_t*)s;
 while(n--)*dd++=*ss++;return d;}
static inline int uiox_memcmp(const void *a,const void *b,size_t n)
{const uint8_t*p=(const uint8_t*)a,*q=(const uint8_t*)b;
 while(n--){if(*p!=*q)return(int)*p-(int)*q;p++;q++;}return 0;}

#endif /* UIOX_FW_TYPES_H */
