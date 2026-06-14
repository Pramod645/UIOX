#ifndef UIOX_BOOT_TYPES_H
#define UIOX_BOOT_TYPES_H
/*
 * uiox_boot_types.h - Base types for the UIOX bootloader.
 * No libc dependency — all types defined from scratch.
 */

typedef unsigned char       uboot_u8_t;
typedef signed   char       uboot_s8_t;
typedef unsigned short      uboot_u16_t;
typedef signed   short      uboot_s16_t;
typedef unsigned int        uboot_u32_t;
typedef signed   int        uboot_s32_t;
typedef unsigned long long  uboot_u64_t;
typedef signed   long long  uboot_s64_t;

#if defined(__aarch64__) || defined(__x86_64__)
typedef uboot_u64_t         uboot_addr_t;
typedef uboot_u64_t         uboot_size_t;
#else
typedef uboot_u32_t         uboot_addr_t;
typedef uboot_u32_t         uboot_size_t;
#endif

typedef uboot_u32_t         uboot_bool_t;
#define UBOOT_TRUE   1u
#define UBOOT_FALSE  0u

#ifndef NULL
#define NULL ((void*)0)
#endif

/* -- Error codes -------------------------------------------- */
#define UBOOT_OK        0
#define UBOOT_EINVAL   -1
#define UBOOT_ENODEV   -2
#define UBOOT_ENOENT   -3
#define UBOOT_ENOMEM   -4
#define UBOOT_EBADIMG  -5
#define UBOOT_EVERIFY  -6
#define UBOOT_ENOTSUP  -7

/* -- Architecture IDs --------------------------------------- */
#define UBOOT_ARCH_ARM64   0x64u
#define UBOOT_ARCH_ARM32   0x32u
#define UBOOT_ARCH_X86_64  0x86u

/* -- Magic numbers ------------------------------------------ */
#define UIOX_BOOT_MAGIC    0x55494F58u   /* "UIOX"             */
#define UIOX_KIMG_MAGIC    0x554B524Eu   /* "UKRN"             */
#define ELF_MAGIC          0x464C457Fu   /* "\x7FELF"          */

/* -- Key physical addresses --------------------------------- */
#define UIOX_KERNEL_LOAD_ARM64   0x40080000ULL
#define UIOX_KERNEL_LOAD_ARM32   0x00200000UL
#define UIOX_KERNEL_LOAD_X86     0x00200000ULL
#define UIOX_BOOT_ARGS_PHYS_ARM64 0x40070000ULL
#define UIOX_BOOT_ARGS_PHYS_ARM32 0x00100000UL
#define UIOX_BOOT_ARGS_PHYS_X86   0x00090000ULL

/* -- Boot config -------------------------------------------- */
#define UIOX_BOOT_HEAP_SIZE     (256u * 1024u)
#define UIOX_BOOT_CMDLINE       512u
#define UIOX_BOOT_MEM_REGIONS   32u
#define UIOX_KERNEL_FILENAME    "uiox.kbin"
#define UIOX_CMDLINE_DEFAULT    "root=/dev/mmcblk0p2 rw quiet"
#define UIOX_UART_DEFAULT_BAUD  115200u

#endif /* UIOX_BOOT_TYPES_H */
