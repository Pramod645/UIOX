#ifndef ARM64_REGISTERS_H
#define ARM64_REGISTERS_H
/*
 * arm64_registers.h — AArch64 register file
 * Reference: ARMv8-A Architecture Reference Manual, Section C1.2
 *
 * AArch64 registers (vs ARM32):
 *   - 31 general-purpose 64-bit registers X0–X30
 *   - W0–W30 are the lower 32 bits of X0–X30
 *   - X30 = Link Register (LR)
 *   - Register 31 = XZR (zero) in data instructions
 *                 = SP  (stack pointer) in load/store
 *   - Separate 64-bit SP per Exception Level (SP_EL0..SP_EL3)
 *   - PC not directly accessible as a GPR
 *   - No banked registers — modes replaced by Exception Levels
 */

#include "arm64_types.h"

/* ── Register indices ────────────────────────────────────── */
#define ARM64_REG_X0    0
#define ARM64_REG_X1    1
#define ARM64_REG_X2    2
#define ARM64_REG_X3    3
#define ARM64_REG_X4    4
#define ARM64_REG_X5    5
#define ARM64_REG_X6    6
#define ARM64_REG_X7    7
#define ARM64_REG_X8    8    /* Indirect result / syscall number */
#define ARM64_REG_X9    9
#define ARM64_REG_X10   10
#define ARM64_REG_X11   11
#define ARM64_REG_X12   12
#define ARM64_REG_X13   13
#define ARM64_REG_X14   14
#define ARM64_REG_X15   15
#define ARM64_REG_X16   16   /* IP0 — intra-procedure call scratch */
#define ARM64_REG_X17   17   /* IP1 — intra-procedure call scratch */
#define ARM64_REG_X18   18   /* Platform register                  */
#define ARM64_REG_X19   19
#define ARM64_REG_X20   20
#define ARM64_REG_X21   21
#define ARM64_REG_X22   22
#define ARM64_REG_X23   23
#define ARM64_REG_X24   24
#define ARM64_REG_X25   25
#define ARM64_REG_X26   26
#define ARM64_REG_X27   27
#define ARM64_REG_X28   28
#define ARM64_REG_X29   29   /* Frame Pointer (FP)               */
#define ARM64_REG_X30   30   /* Link Register (LR)               */
#define ARM64_REG_XZR   31   /* Zero Register (read=0, write=NOP)*/
#define ARM64_REG_SP    31   /* Stack Pointer (context-dependent)*/

/* Total GPR count (not including XZR/SP) */
#define ARM64_NUM_GPRS  31

/* ── Exception Levels ────────────────────────────────────── */
#define ARM64_EL0       0    /* User (Application) level         */
#define ARM64_EL1       1    /* Kernel (OS) level                */
#define ARM64_EL2       2    /* Hypervisor level                 */
#define ARM64_EL3       3    /* Secure Monitor level             */

/* ── Register file structure ─────────────────────────────── */
typedef struct arm64_regfile {
    arm64_xreg_t  x[31];       /* X0–X30 (64-bit)                */
    arm64_xreg_t  sp_el0;      /* EL0 stack pointer              */
    arm64_xreg_t  sp_el1;      /* EL1 stack pointer              */
    arm64_xreg_t  sp_el2;      /* EL2 stack pointer              */
    arm64_xreg_t  sp_el3;      /* EL3 stack pointer              */
    arm64_addr_t  pc;           /* Program Counter (64-bit)       */
} arm64_regfile_t;

/* ── Accessor macros ─────────────────────────────────────── */
/* Read 64-bit Xn (XZR returns 0) */
#define ARM64_READ_X(rf, n) \
    ((n) == ARM64_REG_XZR ? (arm64_xreg_t)0 : (rf)->x[(n)])

/* Write 64-bit Xn (XZR write is discarded) */
#define ARM64_WRITE_X(rf, n, val) \
    do { if ((n) != ARM64_REG_XZR) (rf)->x[(n)] = (val); } while(0)

/* Read 32-bit Wn (upper 32 bits of Xn are zeroed on write) */
#define ARM64_READ_W(rf, n) \
    ((arm64_wreg_t)ARM64_READ_X(rf, n))

/* Write 32-bit Wn (zero-extends to 64 bits) */
#define ARM64_WRITE_W(rf, n, val) \
    ARM64_WRITE_X(rf, n, (arm64_xreg_t)(arm64_uint32_t)(val))

#endif /* ARM64_REGISTERS_H */
