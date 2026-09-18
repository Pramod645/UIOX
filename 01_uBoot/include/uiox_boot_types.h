/**
 * @file  uiox_boot_types.h
 * @brief UIOX Bootloader — base integer types, magic numbers, error codes.
 *
 * This is the only header that may be included by every other bootloader
 * module without risk of circular dependencies.
 *
 * @version 1.0.0  (1.0.0 + RISC-V RV64GC additions — 2026-07-12)
 * @date    2026-07-12
 */

 #ifndef UIOX_BOOT_TYPES_H
#define UIOX_BOOT_TYPES_H

#ifndef UIOX_FW_TYPES_H

typedef unsigned char       uint8_t;
typedef unsigned short      uint16_t;
typedef unsigned int        uint32_t;
typedef unsigned long long  uint64_t;
typedef signed char         int8_t;
typedef signed short        int16_t;
typedef signed int          int32_t;
typedef signed long long    int64_t;

/* 64-bit pointer targets: aarch64, x86-64, and RV64.
 * RISC-V was missing here, which made uintptr_t 32-bit on rv64 and
 * produced -Wint-to-pointer-cast / -Wpointer-to-int-cast everywhere
 * an address crossed uintptr_t. */
#if defined(__aarch64__) || defined(__x86_64__) \
 || defined(__riscv) || defined(__riscv_xlen) || defined(__riscv64)
typedef uint64_t uintptr_t;
typedef int64_t  intptr_t;
typedef uint64_t size_t;
#else
typedef uint32_t uintptr_t;
typedef int32_t  intptr_t;
typedef uint32_t size_t;
#endif

typedef _Bool bool;

#ifndef true
# define true  1
#endif
#ifndef false
# define false 0
#endif
#ifndef NULL
# define NULL  ((void *)0)
#endif

#else
/* uiox_fw_types.h already included — bool/true/false/NULL defined there */
#endif /* UIOX_FW_TYPES_H */

typedef enum {
    UIOX_ARCH_ARM64  = 0,
    UIOX_ARCH_ARM32  = 1,
    UIOX_ARCH_X86_64 = 2,
    UIOX_ARCH_RV64   = 3,
} uiox_arch_t;

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

#define UIOX_IMAGE_MAGIC        0x55494F58u
#define UIOX_BOOT_ARGS_MAGIC    0x55415247u
#define UIOX_DTB_MAGIC          0xD00DFEEDu
#define UIOX_MULTIBOOT2_MAGIC   0xE85250D6u
#define UIOX_MULTIBOOT2_LOADER  0x36D76289u

#define UIOX_IMAGE_HDR_VERSION  1u
#define UIOX_IMAGE_CMDLINE_MAX  256u

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t arch;
    uint32_t hdr_size;
    uint64_t load_addr;
    uint64_t entry_point;
    uint64_t image_size;
    uint8_t  sha256[32];
    char     cmdline[UIOX_IMAGE_CMDLINE_MAX];
    uint8_t  _pad[60];
} uiox_image_hdr_t;

#ifndef UIOX_ALIGN_UP
# define UIOX_ALIGN_UP(v, a)    (((v) + ((a) - 1u)) & ~((a) - 1u))
#endif
#ifndef UIOX_ALIGN_DN
# define UIOX_ALIGN_DN(v, a)    ((v) & ~((a) - 1u))
#endif
#ifndef UIOX_ARRAY_SIZE
# define UIOX_ARRAY_SIZE(a)     (sizeof(a) / sizeof((a)[0]))
#endif
#ifndef UIOX_UNUSED
# define UIOX_UNUSED(x)         ((void)(x))
#endif
#ifndef UIOX_MIN
# define UIOX_MIN(a, b)         ((a) < (b) ? (a) : (b))
#endif
#ifndef UIOX_MAX
# define UIOX_MAX(a, b)         ((a) > (b) ? (a) : (b))
#endif

#endif /* UIOX_BOOT_TYPES_H */
