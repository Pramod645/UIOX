#ifndef ARM64_REGISTERS_H
#define ARM64_REGISTERS_H
/*
 * arm64_registers.h — AArch64 register definitions
 * Reference: ARM DDI 0487, Chapter C1
 *
 * AArch64 has 31 general-purpose 64-bit registers X0-X30.
 * Register 31 is context-sensitive: XZR (zero) or SP.
 * The lower 32-bit views are W0-W30 / WSP / WZR.
 */
#include "arm64_types.h"

/* ── General-purpose register indices ───────────────────── */
#define ARM64_REG_X0    0
#define ARM64_REG_X1    1
#define ARM64_REG_X2    2
#define ARM64_REG_X3    3
#define ARM64_REG_X4    4
#define ARM64_REG_X5    5
#define ARM64_REG_X6    6
#define ARM64_REG_X7    7
#define ARM64_REG_X8    8    /* Indirect result / syscall number     */
#define ARM64_REG_X9    9
#define ARM64_REG_X10   10
#define ARM64_REG_X11   11
#define ARM64_REG_X12   12
#define ARM64_REG_X13   13
#define ARM64_REG_X14   14
#define ARM64_REG_X15   15
#define ARM64_REG_X16   16   /* IP0 — intra-procedure-call scratch   */
#define ARM64_REG_X17   17   /* IP1 — intra-procedure-call scratch   */
#define ARM64_REG_X18   18   /* Platform register                    */
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
#define ARM64_REG_X29   29   /* FP — frame pointer                   */
#define ARM64_REG_X30   30   /* LR — link register                   */
#define ARM64_REG_XZR   31   /* Zero register (read) / SP (write)    */
#define ARM64_REG_SP    31   /* Stack pointer (in stack context)      */
#define ARM64_NUM_REGS  32

/* ── Processor exception levels ─────────────────────────── */
#define ARM64_EL0   0   /* User mode                               */
#define ARM64_EL1   1   /* OS kernel mode                          */
#define ARM64_EL2   2   /* Hypervisor mode                         */
#define ARM64_EL3   3   /* Secure monitor mode                     */

/* ── Register file ───────────────────────────────────────── */
typedef struct arm64_regfile {
    arm64_word_t x[31];     /* X0–X30 (general-purpose)            */
    arm64_word_t sp;        /* Stack pointer (banked per EL)        */
    arm64_word_t pc;        /* Program counter                      */
    arm64_word_t sp_el[4];  /* SP_EL0–SP_EL3 banked stacks         */
} arm64_regfile_t;

/* Convenience aliases */
#define ARM64_FP(rf)  ((rf)->x[ARM64_REG_X29])
#define ARM64_LR(rf)  ((rf)->x[ARM64_REG_X30])
#define ARM64_SP(rf)  ((rf)->sp)
#define ARM64_PC(rf)  ((rf)->pc)

#endif /* ARM64_REGISTERS_H */
