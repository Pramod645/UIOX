#ifndef UIOX_LD_TYPES_H
#define UIOX_LD_TYPES_H
/*
 * uiox_ld_types.h - UIOX linker base types
 */

typedef unsigned char           uld_u8_t;
typedef signed   char           uld_s8_t;
typedef unsigned short          uld_u16_t;
typedef signed   short          uld_s16_t;
typedef unsigned int            uld_u32_t;
typedef signed   int            uld_s32_t;
typedef unsigned long long      uld_u64_t;
typedef signed   long long      uld_s64_t;

/* address and file-offset types */
typedef uld_u64_t  uld_addr_t;   /* virtual address               */
typedef uld_u64_t  uld_off_t;    /* file offset                   */
typedef uld_u32_t  uld_sz32_t;   /* 32-bit size                   */
typedef uld_u64_t  uld_sz64_t;   /* 64-bit size                   */

/* boolean */
typedef int uld_bool_t;
#define ULD_TRUE  1
#define ULD_FALSE 0

#ifndef NULL
#define NULL ((void*)0)
#endif

/* limits */
#define ULD_MAX_SECTIONS   128
#define ULD_MAX_SYMBOLS    65536
#define ULD_MAX_RELOCS     131072
#define ULD_MAX_OBJECTS    512
#define ULD_MAX_ARCHIVES   64
#define ULD_NAME_MAX       256
#define ULD_PATH_MAX       512

/* target architecture */
typedef enum uld_arch {
    ULD_ARCH_X86_64 = 0,
    ULD_ARCH_ARM64  = 1,
    ULD_ARCH_ARM32  = 2,
} uld_arch_t;

/* output format */
typedef enum uld_fmt {
    ULD_FMT_ELF64 = 0,
    ULD_FMT_ELF32 = 1,
    ULD_FMT_FLAT  = 2,
    ULD_FMT_IHEX  = 3,
    ULD_FMT_SREC  = 4,
} uld_fmt_t;

/* endianness */
typedef enum uld_endian {
    ULD_ENDIAN_LITTLE = 0,
    ULD_ENDIAN_BIG    = 1,
} uld_endian_t;

#endif /* UIOX_LD_TYPES_H */
