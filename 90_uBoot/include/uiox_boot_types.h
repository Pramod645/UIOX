/**
 * @file  uiox_boot_types.h
 * @brief UIOX Bootloader — base integer types, magic numbers, error codes.
 *
 * This is the only header that may be included by every other bootloader
 * module without risk of circular dependencies.
 *
 * @version 1.0.0
 * @date    2026-06-12
 */

 #ifndef UIOX_BOOT_TYPES_H
 #define UIOX_BOOT_TYPES_H
 
 /* =========================================================================
  * Portable integer types (no libc in boot environment)
  * ====================================================================== */
 
 typedef unsigned char       uint8_t;
 typedef unsigned short      uint16_t;
 typedef unsigned int        uint32_t;
 typedef unsigned long long  uint64_t;
 typedef signed char         int8_t;
 typedef signed short        int16_t;
 typedef signed int          int32_t;
 typedef signed long long    int64_t;
 
 #if defined(__aarch64__) || defined(__x86_64__)
 typedef uint64_t uintptr_t;
 typedef int64_t  intptr_t;
 typedef uint64_t size_t;
 #else
 typedef uint32_t uintptr_t;
 typedef int32_t  intptr_t;
 typedef uint32_t size_t;
 #endif
 
 typedef _Bool bool;
 #define true  1
 #define false 0
 #define NULL  ((void *)0)
 
 /* =========================================================================
  * Architecture identifier
  * ====================================================================== */
 
 typedef enum {
     UIOX_ARCH_ARM64  = 0,
     UIOX_ARCH_ARM32  = 1,
     UIOX_ARCH_X86_64 = 2,
 } uiox_arch_t;
 
 /* =========================================================================
  * Error codes
  * ====================================================================== */
 
 typedef enum {
     UIOX_BOOT_OK            =  0,
     UIOX_BOOT_ERR_GENERIC   = -1,
     UIOX_BOOT_ERR_NOMEM     = -2,
     UIOX_BOOT_ERR_NOTFOUND  = -3,
     UIOX_BOOT_ERR_IO        = -4,
     UIOX_BOOT_ERR_BADMAGIC  = -5,
     UIOX_BOOT_ERR_BADCSUM   = -6,
     UIOX_BOOT_ERR_OVERFLOW  = -7,
     UIOX_BOOT_ERR_INVAL     = -8,
     UIOX_BOOT_ERR_UNSUP     = -9,
 } uiox_boot_err_t;
 
 /* =========================================================================
  * Magic numbers
  * ====================================================================== */
 
 #define UIOX_IMAGE_MAGIC        0x55494F58u  /**< "UIOX"                  */
 #define UIOX_BOOT_ARGS_MAGIC    0x55415247u  /**< "UARG"                  */
 #define UIOX_DTB_MAGIC          0xD00DFEEDu /**< FDT / DTB               */
 #define UIOX_MULTIBOOT2_MAGIC   0xE85250D6u  /**< Multiboot2 header magic */
 #define UIOX_MULTIBOOT2_LOADER  0x36D76289u  /**< Bootloader magic        */
 
 /* =========================================================================
  * UIOX kernel image header (prepended to kernel binary)
  * ====================================================================== */
 
 #define UIOX_IMAGE_HDR_VERSION  1u
 #define UIOX_IMAGE_CMDLINE_MAX  256u
 
 typedef struct {
     uint32_t magic;          /**< UIOX_IMAGE_MAGIC                        */
     uint32_t version;        /**< Header version (1)                      */
     uint32_t arch;           /**< uiox_arch_t                             */
     uint32_t hdr_size;       /**< sizeof(uiox_image_hdr_t) padded to 64   */
     uint64_t load_addr;      /**< Physical load address                   */
     uint64_t entry_point;    /**< Kernel entry point (physical)           */
     uint64_t image_size;     /**< Total image size (bytes, excl. header)  */
     uint8_t  sha256[32];     /**< SHA-256 of image bytes (excl. header)   */
     char     cmdline[UIOX_IMAGE_CMDLINE_MAX]; /**< Default kernel cmdline  */
     uint8_t  _pad[60];       /**< Padding to 512-byte alignment           */
 } uiox_image_hdr_t;
 
 /* =========================================================================
  * Utility macros
  * ====================================================================== */
 
 #define UIOX_ALIGN_UP(v, a)     (((v) + ((a) - 1u)) & ~((a) - 1u))
 #define UIOX_ALIGN_DN(v, a)     ((v) & ~((a) - 1u))
 #define UIOX_ARRAY_SIZE(a)      (sizeof(a) / sizeof((a)[0]))
 #define UIOX_UNUSED(x)          ((void)(x))
 #define UIOX_MIN(a, b)          ((a) < (b) ? (a) : (b))
 #define UIOX_MAX(a, b)          ((a) > (b) ? (a) : (b))
 
 #endif /* UIOX_BOOT_TYPES_H */
 