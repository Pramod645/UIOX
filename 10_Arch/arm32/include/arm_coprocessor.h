#ifndef ARM_COPROCESSOR_H
#define ARM_COPROCESSOR_H

/*
 * arm_coprocessor.h — ARM coprocessor interface
 * Reference: ARM Instruction Set, Chapter 5
 *
 * ARM supports coprocessors CP0-CP15.
 * CP15 = System Control Coprocessor (cache, MMU, etc.)
 */

#include "arm_types.h"
#include "arm_instr_format.h"
#include "arm_macros.h"

/* ── Coprocessor numbers ─────────────────────────────────── */
#define ARM_CP0    0
#define ARM_CP1    1    /* FPA floating point (legacy)      */
#define ARM_CP2    2
#define ARM_CP10   10   /* VFP                              */
#define ARM_CP11   11   /* VFP double precision             */
#define ARM_CP14   14   /* Debug                            */
#define ARM_CP15   15   /* System control                   */

/* ── CP15 register numbers ───────────────────────────────── */
#define ARM_CP15_ID       0   /* ID register                    */
#define ARM_CP15_CTRL     1   /* Control register               */
#define ARM_CP15_TTBR     2   /* Translation Table Base         */
#define ARM_CP15_DAC      3   /* Domain Access Control          */
#define ARM_CP15_FSR      5   /* Fault Status Register          */
#define ARM_CP15_FAR      6   /* Fault Address Register         */
#define ARM_CP15_CACHECR  7   /* Cache control                  */
#define ARM_CP15_TLBCR    8   /* TLB control                    */
#define ARM_CP15_CACHETP  9   /* Cache lockdown                 */
#define ARM_CP15_TLBTP   10   /* TLB lockdown                   */
#define ARM_CP15_PID     13   /* Process ID                     */

/* ── CP15 Control register bits ─────────────────────────── */
#define ARM_CP15_CTRL_M  (1u <<  0)   /* MMU enable               */
#define ARM_CP15_CTRL_A  (1u <<  1)   /* Alignment fault enable   */
#define ARM_CP15_CTRL_C  (1u <<  2)   /* Data cache enable        */
#define ARM_CP15_CTRL_W  (1u <<  3)   /* Write buffer enable      */
#define ARM_CP15_CTRL_P  (1u <<  4)   /* 32-bit exception handlers*/
#define ARM_CP15_CTRL_D  (1u <<  5)   /* 32-bit data address      */
#define ARM_CP15_CTRL_I  (1u << 12)   /* Instruction cache enable */
#define ARM_CP15_CTRL_V  (1u << 13)   /* High vectors             */
#define ARM_CP15_CTRL_RR (1u << 14)   /* Round-robin replacement  */

/* ── Build MCR to CP15 ───────────────────────────────────── */
#define ARM_BUILD_MCR_CP15(CRn, Rd, op2) \
    ARM_BUILD_CRT(ARM_COND_AL, 0, 0, CRn, Rd, ARM_CP15, op2, 0)

/* ── Build MRC from CP15 ─────────────────────────────────── */
#define ARM_BUILD_MRC_CP15(CRn, Rd, op2) \
    ARM_BUILD_CRT(ARM_COND_AL, 0, 1, CRn, Rd, ARM_CP15, op2, 0)

/* ── Coprocessor register file ───────────────────────────── */
typedef struct arm_coproc {
    arm_word_t regs[16];    /* 16 registers per coprocessor   */
} arm_coproc_t;

/* ── Coprocessor operation function type ─────────────────── */
typedef int (*arm_cp_op_fn)(arm_coproc_t *cp,
                              const arm_instr_cdp_t *instr);

#endif /* ARM_COPROCESSOR_H */
