/**
 * @file  uiox_fw_types.h
 * @brief UIOX Firmware — base integer types, error codes, magic numbers.
 *        No libc dependency — safe for use before OS init.
 * @version 1.0.0
 * @date    2026-06-21
 */

 #ifndef UIOX_FW_TYPES_H
 #define UIOX_FW_TYPES_H

 
 //===========================================
 /* ── Firmware image layout constants ────────────────────────
 * These match the firmware linker scripts in 02_FwHal/linker/.
 * Adjust the values to match your actual linker script ORIGIN.
 * ====================================================================== */

/* Replace the existing UIOX_FW_LOAD_PA / UIOX_FW_IMAGE_SIZE block
   with this version — uses cast instead of ULL suffix               */

   #if defined(__aarch64__)
   #  define UIOX_FW_LOAD_PA     ((uintptr_t)0x40000000u)
   #  define UIOX_FW_IMAGE_SIZE  ((uint32_t) 0x00080000u)
   #elif defined(__arm__)
   #  define UIOX_FW_LOAD_PA     ((uintptr_t)0x00100000u)
   #  define UIOX_FW_IMAGE_SIZE  ((uint32_t) 0x00040000u)
   #else
   #  define UIOX_FW_LOAD_PA     ((uintptr_t)0x00100000u)
   #  define UIOX_FW_IMAGE_SIZE  ((uint32_t) 0x00080000u)
   #endif
   

 /* =========================================================================
  * Portable integer types
  * ====================================================================== */
 typedef unsigned char       uint8_t;
 typedef unsigned short      uint16_t;
 typedef unsigned int        uint32_t;
 typedef unsigned long long  uint64_t;
 typedef signed   char       int8_t;
 typedef signed   short      int16_t;
 typedef signed   int        int32_t;
 typedef signed   long long  int64_t;
 
 #if defined(__aarch64__) || defined(__x86_64__)
 typedef uint64_t uintptr_t;
 typedef int64_t  intptr_t;
 typedef uint64_t size_t;
 #else
 typedef uint32_t uintptr_t;
 typedef int32_t  intptr_t;
 typedef uint32_t size_t;
 #endif
 
 typedef _Bool    bool;
 #define true     1
 #define false    0
 #define NULL     ((void *)0)
 
 /* =========================================================================
  * Architecture identifier
  * ====================================================================== */
 typedef enum {
     UIOX_FW_ARCH_ARM64  = 0,
     UIOX_FW_ARCH_ARM32  = 1,
     UIOX_FW_ARCH_X86_64 = 2,
 } uiox_fw_arch_t;
 //==
 /* Additional error codes needed by new modules */
#define UIOX_FW_ERR_POST       -10   /* POST test failure             */
#define UIOX_FW_ERR_SECBOOT    -11   /* Secure boot verification fail */
#define UIOX_FW_ERR_SECURITY   -12   /* Security policy violation     */
#define UIOX_FW_ERR_FULL       -13   /* table / buffer full           */
#define UIOX_FW_ERR_NOTSUP     -14   /* feature not supported on arch */

 /* =========================================================================
  * Error codes
  * ====================================================================== */
 typedef enum {
     UIOX_FW_OK           =  0,
     UIOX_FW_ERR_GENERIC  = -1,
     UIOX_FW_ERR_INVAL    = -2,
     UIOX_FW_ERR_NOMEM    = -3,
     UIOX_FW_ERR_IO       = -4,
     UIOX_FW_ERR_TIMEOUT  = -5,
     UIOX_FW_ERR_BUSY     = -6,
     UIOX_FW_ERR_NODEV    = -7,
     UIOX_FW_ERR_UNSUP    = -8,
     UIOX_FW_ERR_PERM     = -9,
     UIOX_FW_ERR_OVERFLOW = -10,
     UIOX_FW_ERR_BADMAGIC = -11,
 } uiox_fw_err_t;
 
 /* =========================================================================
  * Magic numbers
  * ====================================================================== */
 #define UIOX_FW_MAGIC           0x55494F58u  /**< "UIOX"                  */
 #define UIOX_FW_DEVSW_MAGIC     0x44455357u  /**< "DESW"                  */
 #define UIOX_FW_VERSION         0x00010000u  /**< v1.0.0                  */
 
 /* =========================================================================
  * Utility macros
  * ====================================================================== */
 #define UIOX_FW_UNUSED(x)       ((void)(x))
 #define UIOX_FW_ARRAY_SIZE(a)   (sizeof(a) / sizeof((a)[0]))
 #define UIOX_FW_ALIGN_UP(v,a)   (((v) + ((a)-1u)) & ~((a)-1u))
 #define UIOX_FW_ALIGN_DN(v,a)   ((v) & ~((a)-1u))
 #define UIOX_FW_MIN(a,b)        ((a)<(b)?(a):(b))
 #define UIOX_FW_MAX(a,b)        ((a)>(b)?(a):(b))
 #define UIOX_FW_BIT(n)          (1u << (n))
 
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
 
 #endif /* UIOX_FW_TYPES_H */
 