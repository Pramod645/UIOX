#ifndef ARM64_PSR_H
#define ARM64_PSR_H
/*
 * arm64_psr.h — AArch64 PSTATE / SPSR bits and condition codes
 * Reference: ARM DDI 0487, Section C5.2
 *
 * In AArch64 the condition flags live in PSTATE:
 *   Bit 31  N — Negative flag
 *   Bit 30  Z — Zero flag
 *   Bit 29  C — Carry flag
 *   Bit 28  V — Overflow flag
 *   Bit 9   D — Debug mask
 *   Bit 8   A — SError mask
 *   Bit 7   I — IRQ mask
 *   Bit 6   F — FIQ mask
 *   Bits 3:2 EL — Exception level
 *   Bit 0   SP — Stack pointer select (0=SP_EL0, 1=SP_ELn)
 */
#include "arm64_types.h"

/* ── PSTATE / SPSR bit masks ─────────────────────────────── */
#define ARM64_PSR_N    (1u << 31)   /* Negative flag              */
#define ARM64_PSR_Z    (1u << 30)   /* Zero flag                  */
#define ARM64_PSR_C    (1u << 29)   /* Carry flag                 */
#define ARM64_PSR_V    (1u << 28)   /* Overflow flag              */
#define ARM64_PSR_SS   (1u << 21)   /* Software step              */
#define ARM64_PSR_IL   (1u << 20)   /* Illegal execution state    */
#define ARM64_PSR_D    (1u <<  9)   /* Debug exception mask       */
#define ARM64_PSR_A    (1u <<  8)   /* SError interrupt mask      */
#define ARM64_PSR_I    (1u <<  7)   /* IRQ interrupt mask         */
#define ARM64_PSR_F    (1u <<  6)   /* FIQ interrupt mask         */
#define ARM64_PSR_EL   (3u <<  2)   /* Exception level [3:2]      */
#define ARM64_PSR_SP   (1u <<  0)   /* Stack pointer select       */

/* ── Flag extract helpers ─────────────────────────────────── */
#define ARM64_PSR_GET_N(psr)  (((psr) & ARM64_PSR_N) ? 1 : 0)
#define ARM64_PSR_GET_Z(psr)  (((psr) & ARM64_PSR_Z) ? 1 : 0)
#define ARM64_PSR_GET_C(psr)  (((psr) & ARM64_PSR_C) ? 1 : 0)
#define ARM64_PSR_GET_V(psr)  (((psr) & ARM64_PSR_V) ? 1 : 0)
#define ARM64_PSR_GET_EL(psr) (((psr) & ARM64_PSR_EL) >> 2)

/* ── Exception level encode ───────────────────────────────── */
#define ARM64_PSR_EL0h  0x00u   /* EL0, SP0                      */
#define ARM64_PSR_EL1t  0x04u   /* EL1, SP0                      */
#define ARM64_PSR_EL1h  0x05u   /* EL1, SP1                      */
#define ARM64_PSR_EL2h  0x09u   /* EL2, SP2                      */
#define ARM64_PSR_EL3h  0x0Du   /* EL3, SP3                      */

/* ── Condition codes (cond field [31:28] in instructions) ── */
#define ARM64_COND_EQ   0x0   /* Equal               Z=1         */
#define ARM64_COND_NE   0x1   /* Not equal           Z=0         */
#define ARM64_COND_CS   0x2   /* Carry set           C=1         */
#define ARM64_COND_CC   0x3   /* Carry clear         C=0         */
#define ARM64_COND_MI   0x4   /* Minus / negative    N=1         */
#define ARM64_COND_PL   0x5   /* Plus / positive     N=0         */
#define ARM64_COND_VS   0x6   /* Overflow            V=1         */
#define ARM64_COND_VC   0x7   /* No overflow         V=0         */
#define ARM64_COND_HI   0x8   /* Higher              C=1 && Z=0  */
#define ARM64_COND_LS   0x9   /* Lower or same       C=0 || Z=1  */
#define ARM64_COND_GE   0xA   /* Greater or equal    N=V         */
#define ARM64_COND_LT   0xB   /* Less than           N!=V        */
#define ARM64_COND_GT   0xC   /* Greater than        Z=0 && N=V  */
#define ARM64_COND_LE   0xD   /* Less or equal       Z=1 || N!=V */
#define ARM64_COND_AL   0xE   /* Always                          */
#define ARM64_COND_NV   0xF   /* Never (reserved, acts as AL)    */

/* ── PSTATE struct ────────────────────────────────────────── */
typedef struct arm64_psr {
    arm64_uint32_t pstate;    /* Current PSTATE value             */
    arm64_uint32_t spsr[4];   /* SPSR_EL1 – SPSR_EL3 + EL0 pad   */
    arm64_uint64_t elr[4];    /* ELR_EL1  – ELR_EL3              */
} arm64_psr_t;

/* ── Extract condition from instruction word [31:28] ──────── */
#define ARM64_INSTR_COND(instr) (((instr) >> 28) & 0xF)

#endif /* ARM64_PSR_H */
