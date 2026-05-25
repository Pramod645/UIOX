#ifndef ARM64_PSR_H
#define ARM64_PSR_H
/*
 * arm64_psr.h — AArch64 Process State (PSTATE) and System Registers
 * Reference: ARMv8-A Architecture Reference Manual, Section C5.2
 *
 * AArch64 replaces ARM32 CPSR with PSTATE:
 *   - PSTATE fields: N, Z, C, V, SS, IL, D, A, I, F, nRW, EL, SP
 *   - No single CPSR register — fields are individual system registers
 *   - SPSR_ELn stores PSTATE on exception entry (one per EL)
 *   - 64-bit condition flags still use bits [63:28] conceptually
 *     but are accessible via NZCV pseudo-register
 */

#include "arm64_types.h"

/* ── PSTATE / NZCV flag masks ────────────────────────────── */
#define ARM64_PSTATE_N    (1u << 31)  /* Negative flag              */
#define ARM64_PSTATE_Z    (1u << 30)  /* Zero flag                  */
#define ARM64_PSTATE_C    (1u << 29)  /* Carry flag                 */
#define ARM64_PSTATE_V    (1u << 28)  /* Overflow flag              */

/* ── PSTATE exception mask bits ─────────────────────────── */
#define ARM64_PSTATE_D    (1u <<  9)  /* Debug exception mask       */
#define ARM64_PSTATE_A    (1u <<  8)  /* SError / Abort mask        */
#define ARM64_PSTATE_I    (1u <<  7)  /* IRQ mask                   */
#define ARM64_PSTATE_F    (1u <<  6)  /* FIQ mask                   */

/* ── PSTATE other fields ─────────────────────────────────── */
#define ARM64_PSTATE_SS   (1u << 21)  /* Software Step              */
#define ARM64_PSTATE_IL   (1u << 20)  /* Illegal Execution State    */
#define ARM64_PSTATE_SP   (1u <<  0)  /* Stack Pointer select (0=SP_EL0) */

/* ── Exception Level field [3:2] ────────────────────────── */
#define ARM64_PSTATE_EL_MASK  (0x3u << 2)
#define ARM64_PSTATE_EL0      (0x0u << 2)
#define ARM64_PSTATE_EL1      (0x1u << 2)
#define ARM64_PSTATE_EL2      (0x2u << 2)
#define ARM64_PSTATE_EL3      (0x3u << 2)

/* ── Condition codes (same encoding as ARM32) ────────────── */
#define ARM64_COND_EQ   0x0   /* Z=1            Equal             */
#define ARM64_COND_NE   0x1   /* Z=0            Not equal         */
#define ARM64_COND_CS   0x2   /* C=1            Carry set (HS)    */
#define ARM64_COND_CC   0x3   /* C=0            Carry clear (LO)  */
#define ARM64_COND_MI   0x4   /* N=1            Minus/negative    */
#define ARM64_COND_PL   0x5   /* N=0            Plus/positive     */
#define ARM64_COND_VS   0x6   /* V=1            Overflow          */
#define ARM64_COND_VC   0x7   /* V=0            No overflow       */
#define ARM64_COND_HI   0x8   /* C=1 && Z=0     Unsigned high     */
#define ARM64_COND_LS   0x9   /* C=0 || Z=1     Unsigned low/same */
#define ARM64_COND_GE   0xA   /* N=V            Signed >=         */
#define ARM64_COND_LT   0xB   /* N!=V           Signed <          */
#define ARM64_COND_GT   0xC   /* Z=0 && N=V     Signed >          */
#define ARM64_COND_LE   0xD   /* Z=1 || N!=V    Signed <=         */
#define ARM64_COND_AL   0xE   /* Always                           */
#define ARM64_COND_NV   0xF   /* Never (SBFM/BFM encoding alias)  */

/* ── PSTATE structure ────────────────────────────────────── */
typedef struct arm64_pstate {
    arm64_uint32_t  nzcv;         /* N,Z,C,V flags (bits [31:28])  */
    arm64_uint32_t  daif;         /* D,A,I,F interrupt masks        */
    arm64_uint32_t  el;           /* Current exception level 0..3   */
    arm64_uint32_t  sp_sel;       /* SP selector: 0=SP_EL0,1=SP_ELn*/
    /* Saved PSTATE per exception level */
    arm64_uint64_t  spsr_el1;     /* Saved PSTATE on EL1 entry      */
    arm64_uint64_t  spsr_el2;     /* Saved PSTATE on EL2 entry      */
    arm64_uint64_t  spsr_el3;     /* Saved PSTATE on EL3 entry      */
    /* Exception Link Register per EL */
    arm64_addr_t    elr_el1;      /* Return address from EL1 exception*/
    arm64_addr_t    elr_el2;      /* Return address from EL2 exception*/
    arm64_addr_t    elr_el3;      /* Return address from EL3 exception*/
} arm64_pstate_t;

/* ── NZCV helpers ────────────────────────────────────────── */
#define ARM64_GET_N(ps)  (((ps)->nzcv >> 31) & 1)
#define ARM64_GET_Z(ps)  (((ps)->nzcv >> 30) & 1)
#define ARM64_GET_C(ps)  (((ps)->nzcv >> 29) & 1)
#define ARM64_GET_V(ps)  (((ps)->nzcv >> 28) & 1)

/* Extract condition field from AArch64 instruction [3:0] */
#define ARM64_INSTR_COND(instr)   ((instr) & 0xFu)

#endif /* ARM64_PSR_H */
