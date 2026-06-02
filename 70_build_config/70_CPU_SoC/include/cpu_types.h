#ifndef CPU_TYPES_H
#define CPU_TYPES_H
/*
 * cpu_types.h - Base types for CPU/SoC subsystem
 * Covers ARM Cortex-A76, x86-64, RISC-V RV64GC
 */

typedef unsigned char       cpu_u8_t;
typedef signed   char       cpu_s8_t;
typedef unsigned short      cpu_u16_t;
typedef signed   short      cpu_s16_t;
typedef unsigned int        cpu_u32_t;
typedef signed   int        cpu_s32_t;
typedef unsigned long long  cpu_u64_t;
typedef signed   long long  cpu_s64_t;

typedef cpu_u64_t  cpu_addr_t;   /* physical / virtual address   */
typedef cpu_u64_t  cpu_reg_t;    /* native register width        */
typedef cpu_u32_t  cpu_bool_t;

#define CPU_TRUE   1u
#define CPU_FALSE  0u

#ifndef NULL
#define NULL ((void*)0)
#endif

/* -- Architecture tag --------------------------------------- */
typedef enum cpu_arch {
    CPU_ARCH_ARM_CORTEX_A76 = 0,
    CPU_ARCH_X86_64         = 1,
    CPU_ARCH_RISCV64        = 2,
    CPU_ARCH_UNKNOWN        = 0xFF,
} cpu_arch_t;

/* -- Return codes ------------------------------------------- */
#define CPU_OK      0
#define CPU_ERR    -1
#define CPU_ENOSUP -2   /* feature not supported on this arch   */
#define CPU_ETIMEOUT -3

/* -- Alignment helper --------------------------------------- */
#define CPU_ALIGN_UP(x, a)   (((x) + (a) - 1u) & ~((a) - 1u))
#define CPU_ALIGN_DOWN(x, a) ((x) & ~((a) - 1u))

/* -- Cache line sizes --------------------------------------- */
#define CPU_CACHE_LINE_ARM64   64u
#define CPU_CACHE_LINE_X86_64  64u
#define CPU_CACHE_LINE_RISCV64 64u

/* -- Page sizes --------------------------------------------- */
#define CPU_PAGE_SIZE_4K    0x1000ULL
#define CPU_PAGE_SIZE_2M    0x200000ULL
#define CPU_PAGE_SIZE_1G    0x40000000ULL

#endif /* CPU_TYPES_H */
