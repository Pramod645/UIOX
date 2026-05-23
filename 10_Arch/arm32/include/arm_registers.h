#ifndef ARM_REGISTERS_H
#define ARM_REGISTERS_H

/*
 * arm_registers.h — ARM register definitions
 * Reference: ARM Instruction Set, Section 2
 *
 * ARM has 16 general-purpose registers visible at any time:
 * R0-R12  general purpose
 * R13     Stack Pointer (SP)
 * R14     Link Register (LR)
 * R15     Program Counter (PC)
 */

#include "arm_types.h"

/* ── Register numbers ────────────────────────────────────── */
#define ARM_REG_R0   0
#define ARM_REG_R1   1
#define ARM_REG_R2   2
#define ARM_REG_R3   3
#define ARM_REG_R4   4
#define ARM_REG_R5   5
#define ARM_REG_R6   6
#define ARM_REG_R7   7
#define ARM_REG_R8   8
#define ARM_REG_R9   9
#define ARM_REG_R10  10
#define ARM_REG_R11  11
#define ARM_REG_R12  12
#define ARM_REG_R13  13   /* SP — Stack Pointer          */
#define ARM_REG_R14  14   /* LR — Link Register          */
#define ARM_REG_R15  15   /* PC — Program Counter        */

#define ARM_REG_SP   ARM_REG_R13
#define ARM_REG_LR   ARM_REG_R14
#define ARM_REG_PC   ARM_REG_R15

/* ── Processor modes ─────────────────────────────────────── */
#define ARM_MODE_USR  0x10   /* User mode                  */
#define ARM_MODE_FIQ  0x11   /* Fast IRQ mode              */
#define ARM_MODE_IRQ  0x12   /* IRQ mode                   */
#define ARM_MODE_SVC  0x13   /* Supervisor mode            */
#define ARM_MODE_ABT  0x17   /* Abort mode                 */
#define ARM_MODE_UND  0x1B   /* Undefined mode             */
#define ARM_MODE_SYS  0x1F   /* System mode                */

/* ── Register file (banked registers per mode) ───────────── */
typedef struct arm_regfile {
    arm_word_t r[16];       /* R0–R15 current view         */
    arm_word_t r_fiq[7];    /* R8–R14 FIQ banked           */
    arm_word_t r_irq[2];    /* R13–R14 IRQ banked          */
    arm_word_t r_svc[2];    /* R13–R14 SVC banked          */
    arm_word_t r_abt[2];    /* R13–R14 ABT banked          */
    arm_word_t r_und[2];    /* R13–R14 UND banked          */
} arm_regfile_t;

/* Macros to access named registers */
#define ARM_SP(rf)  ((rf)->r[ARM_REG_SP])
#define ARM_LR(rf)  ((rf)->r[ARM_REG_LR])
#define ARM_PC(rf)  ((rf)->r[ARM_REG_PC])

#endif /* ARM_REGISTERS_H */
