#ifndef ARM64_TYPES_H
#define ARM64_TYPES_H
/*
 * arm64_types.h — Base types for AArch64 instruction set encoding
 * Reference: ARM Architecture Reference Manual (ARMv8-A) DDI 0487
 *
 * AArch64 key differences from ARM32:
 *   - 64-bit general-purpose registers (X0–X30)
 *   - 32-bit views of same registers (W0–W30)
 *   - Dedicated zero register (XZR/WZR) and stack pointer (SP)
 *   - No register banking by mode
 *   - Fixed 32-bit instruction encoding (no Thumb)
 *   - 64-bit virtual address space
 */

typedef unsigned char       arm64_uint8_t;
typedef signed   char       arm64_int8_t;
typedef unsigned short      arm64_uint16_t;
typedef signed   short      arm64_int16_t;
typedef unsigned int        arm64_uint32_t;
typedef signed   int        arm64_int32_t;
typedef unsigned long long  arm64_uint64_t;
typedef signed   long long  arm64_int64_t;

/* AArch64 word size = 64 bits */
typedef arm64_uint64_t  arm64_xreg_t;   /* 64-bit X register       */
typedef arm64_uint32_t  arm64_wreg_t;   /* 32-bit W register view  */
typedef arm64_uint64_t  arm64_addr_t;   /* 64-bit virtual address  */
typedef arm64_uint32_t  arm64_instr_t;  /* Fixed 32-bit instruction */
typedef arm64_uint64_t  arm64_imm_t;    /* Immediate value          */

/* Register number: 0–30 (31 = XZR/SP depending on context) */
typedef arm64_uint8_t   arm64_reg_t;

/* Shift amount: 0–63 for 64-bit, 0–31 for 32-bit */
typedef arm64_uint8_t   arm64_shift_t;

/* Boolean */
typedef int arm64_bool_t;
#define ARM64_TRUE   1
#define ARM64_FALSE  0

#ifndef NULL
#define NULL ((void*)0)
#endif

#endif /* ARM64_TYPES_H */
