#ifndef ARM_PSR_H
#define ARM_PSR_H

/*
 * arm_psr.h — Program Status Register (CPSR/SPSR)
 * Reference: ARM Instruction Set, Section 2.4
 *
 * Bit layout of CPSR:
 *  [31] N  Negative / less than
 *  [30] Z  Zero
 *  [29] C  Carry / borrow / extend
 *  [28] V  Overflow
 *  [27] Q  Sticky overflow (ARMv5+)
 *  [7]  I  IRQ disable
 *  [6]  F  FIQ disable
 *  [5]  T  Thumb state
 *  [4:0]M  Processor mode
 */

#include "arm_types.h"

/* ── PSR bit masks ───────────────────────────────────────── */
#define ARM_PSR_N    (1u << 31)   /* Negative flag           */
#define ARM_PSR_Z    (1u << 30)   /* Zero flag               */
#define ARM_PSR_C    (1u << 29)   /* Carry flag              */
#define ARM_PSR_V    (1u << 28)   /* Overflow flag           */
#define ARM_PSR_Q    (1u << 27)   /* Sticky overflow         */
#define ARM_PSR_I    (1u <<  7)   /* IRQ disable             */
#define ARM_PSR_F    (1u <<  6)   /* FIQ disable             */
#define ARM_PSR_T    (1u <<  5)   /* Thumb state             */
#define ARM_PSR_MODE (0x1Fu)      /* Mode bits [4:0]         */

/* ── PSR structure ───────────────────────────────────────── */
typedef struct arm_psr {
    arm_word_t cpsr;    /* Current Program Status Register  */
    arm_word_t spsr;    /* Saved Program Status Register    */
} arm_psr_t;

/* ── PSR access macros ───────────────────────────────────── */
#define ARM_PSR_GET_N(psr)    (((psr) & ARM_PSR_N) != 0)
#define ARM_PSR_GET_Z(psr)    (((psr) & ARM_PSR_Z) != 0)
#define ARM_PSR_GET_C(psr)    (((psr) & ARM_PSR_C) != 0)
#define ARM_PSR_GET_V(psr)    (((psr) & ARM_PSR_V) != 0)
#define ARM_PSR_GET_T(psr)    (((psr) & ARM_PSR_T) != 0)
#define ARM_PSR_GET_MODE(psr) ((psr) & ARM_PSR_MODE)

#define ARM_PSR_SET_N(psr)    ((psr) |= ARM_PSR_N)
#define ARM_PSR_CLR_N(psr)    ((psr) &= ~ARM_PSR_N)
#define ARM_PSR_SET_Z(psr)    ((psr) |= ARM_PSR_Z)
#define ARM_PSR_CLR_Z(psr)    ((psr) &= ~ARM_PSR_Z)
#define ARM_PSR_SET_C(psr)    ((psr) |= ARM_PSR_C)
#define ARM_PSR_CLR_C(psr)    ((psr) &= ~ARM_PSR_C)
#define ARM_PSR_SET_V(psr)    ((psr) |= ARM_PSR_V)
#define ARM_PSR_CLR_V(psr)    ((psr) &= ~ARM_PSR_V)

/* ── Condition codes [31:28] ─────────────────────────────── */
#define ARM_COND_EQ  0x0   /* Z=1            Equal           */
#define ARM_COND_NE  0x1   /* Z=0            Not equal       */
#define ARM_COND_CS  0x2   /* C=1            Carry set       */
#define ARM_COND_CC  0x3   /* C=0            Carry clear     */
#define ARM_COND_MI  0x4   /* N=1            Minus/negative  */
#define ARM_COND_PL  0x5   /* N=0            Plus/positive   */
#define ARM_COND_VS  0x6   /* V=1            Overflow        */
#define ARM_COND_VC  0x7   /* V=0            No overflow     */
#define ARM_COND_HI  0x8   /* C=1 && Z=0     Unsigned high   */
#define ARM_COND_LS  0x9   /* C=0 || Z=1     Unsigned low    */
#define ARM_COND_GE  0xA   /* N=V            Signed >=       */
#define ARM_COND_LT  0xB   /* N!=V           Signed <        */
#define ARM_COND_GT  0xC   /* Z=0 && N=V     Signed >        */
#define ARM_COND_LE  0xD   /* Z=1 || N!=V    Signed <=       */
#define ARM_COND_AL  0xE   /* Always                         */
#define ARM_COND_NV  0xF   /* Never (deprecated)            */

/* Extract condition from instruction word */
#define ARM_INSTR_COND(instr) (((instr) >> 28) & 0xF)

#endif /* ARM_PSR_H */
