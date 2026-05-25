#ifndef ARM64_PSTATE_H
#define ARM64_PSTATE_H

/*
 * arm64_pstate.h — AArch64 Process State (PSTATE)
 * Reference: ARM DDI 0487, Section D1 (AArch64 System Registers)
 *
 * PSTATE replaces ARM32 CPSR/SPSR.
 * Condition flags are in NZCV (bits [31:28] of PSTATE).
 *
 * Bit layout of PSTATE (accessible fields):
 *   [31] N   Negative result flag
 *   [30] Z   Zero result flag
 *   [29] C   Carry flag
 *   [28] V   Overflow flag
 *   [21] SS  Software Step
 *   [20] IL  Illegal Execution State
 *   [9]  D   Debug mask bit
 *   [8]  A   SError interrupt mask
 *   [7]  I   IRQ interrupt mask
 *   [6]  F   FIQ interrupt mask
 *   [4]  M[4] Execution state (0=AArch64, 1=AArch32 when in EL0)
 *   [3:2] EL  Exception level (0-3)
 *   [0]  SP   Stack pointer select (0=SP_EL0, 1=SP_ELx)
 */

#include "arm64_types.h"

/* ── PSTATE bit masks ────────────────────────────────────── */
#define ARM64_PSTATE_N    (1u << 31)  /* Negative flag          */
#define ARM64_PSTATE_Z    (1u << 30)  /* Zero flag              */
#define ARM64_PSTATE_C    (1u << 29)  /* Carry flag             */
#define ARM64_PSTATE_V    (1u << 28)  /* Overflow flag          */
#define ARM64_PSTATE_SS   (1u << 21)  /* Software step          */
#define ARM64_PSTATE_IL   (1u << 20)  /* Illegal exec state     */
#define ARM64_PSTATE_D    (1u <<  9)  /* Debug mask             */
#define ARM64_PSTATE_A    (1u <<  8)  /* SError mask            */
#define ARM64_PSTATE_I    (1u <<  7)  /* IRQ mask               */
#define ARM64_PSTATE_F    (1u <<  6)  /* FIQ mask               */
#define ARM64_PSTATE_EL   (0x3u << 2) /* Exception level [3:2]  */
#define ARM64_PSTATE_SP   (1u <<  0)  /* SP select              */

/* ── NZCV combined mask ──────────────────────────────────── */
#define ARM64_PSTATE_NZCV (ARM64_PSTATE_N | ARM64_PSTATE_Z | \
                           ARM64_PSTATE_C | ARM64_PSTATE_V)

/* ── Exception Level extraction ─────────────────────────── */
#define ARM64_PSTATE_EL_SHIFT  2
#define ARM64_PSTATE_GET_EL(p) (((p) >> ARM64_PSTATE_EL_SHIFT) & 0x3u)
#define ARM64_PSTATE_SET_EL(p, el) \
    (((p) & ~ARM64_PSTATE_EL) | (((el) & 0x3u) << ARM64_PSTATE_EL_SHIFT))

/* ── PSTATE structure ────────────────────────────────────── */
typedef struct arm64_pstate {
    arm64_wrd32_t  pstate;    /* Synthetic PSTATE register        */
    arm64_word_t   spsr_el[4];/* SPSR_EL1..EL3 (index = EL)      */
    arm64_word_t   elr_el[4]; /* ELR_EL1..EL3  exception link reg*/
} arm64_pstate_t;

/* ── Condition codes ─────────────────────────────────────── */
#define ARM64_COND_EQ  0x0  /* Z=1          Equal               */
#define ARM64_COND_NE  0x1  /* Z=0          Not equal           */
#define ARM64_COND_CS  0x2  /* C=1          Carry set / HS      */
#define ARM64_COND_CC  0x3  /* C=0          Carry clear / LO    */
#define ARM64_COND_MI  0x4  /* N=1          Minus / negative    */
#define ARM64_COND_PL  0x5  /* N=0          Plus / positive     */
#define ARM64_COND_VS  0x6  /* V=1          Overflow            */
#define ARM64_COND_VC  0x7  /* V=0          No overflow         */
#define ARM64_COND_HI  0x8  /* C=1 && Z=0   Unsigned higher     */
#define ARM64_COND_LS  0x9  /* C=0 || Z=1   Unsigned lower/same */
#define ARM64_COND_GE  0xA  /* N=V          Signed >=           */
#define ARM64_COND_LT  0xB  /* N!=V         Signed <            */
#define ARM64_COND_GT  0xC  /* Z=0 && N=V   Signed >            */
#define ARM64_COND_LE  0xD  /* Z=1 || N!=V  Signed <=           */
#define ARM64_COND_AL  0xE  /* Always                           */
#define ARM64_COND_NV  0xF  /* Never / UNDEFINED in A64         */

/* Extract condition from A64 instruction word [31:28] */
#define ARM64_INSTR_COND(instr)   (((instr) >> 28) & 0xFu)

/* ── PSTATE flag helpers ─────────────────────────────────── */
#define ARM64_FLAG_N(p)   (!!((p)->pstate & ARM64_PSTATE_N))
#define ARM64_FLAG_Z(p)   (!!((p)->pstate & ARM64_PSTATE_Z))
#define ARM64_FLAG_C(p)   (!!((p)->pstate & ARM64_PSTATE_C))
#define ARM64_FLAG_V(p)   (!!((p)->pstate & ARM64_PSTATE_V))

#define ARM64_SET_N(p,v)  ((v) ? ((p)->pstate |= ARM64_PSTATE_N) \
                                : ((p)->pstate &= ~ARM64_PSTATE_N))
#define ARM64_SET_Z(p,v)  ((v) ? ((p)->pstate |= ARM64_PSTATE_Z) \
                                : ((p)->pstate &= ~ARM64_PSTATE_Z))
#define ARM64_SET_C(p,v)  ((v) ? ((p)->pstate |= ARM64_PSTATE_C) \
                                : ((p)->pstate &= ~ARM64_PSTATE_C))
#define ARM64_SET_V(p,v)  ((v) ? ((p)->pstate |= ARM64_PSTATE_V) \
                                : ((p)->pstate &= ~ARM64_PSTATE_V))

#endif /* ARM64_PSTATE_H */
