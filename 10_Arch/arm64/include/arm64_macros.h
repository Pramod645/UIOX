#ifndef ARM64_MACROS_H
#define ARM64_MACROS_H
/*
 * arm64_macros.h — Per-instruction address macros
 * Reference: ARM DDI 0487
 *
 * Each macro takes a base address and returns a pointer to the
 * appropriate instruction struct so fields can be accessed directly.
 *
 * Usage:
 *   arm64_instr_addsub_imm_t *p = ARM64_INSTR_AT_ADD(base);
 *   p->Rd  = 0;          // destination = X0
 *   p->Rn  = 1;          // source      = X1
 *   p->imm12 = 16;       // immediate   = 16
 *   p->sf  = 1;          // 64-bit
 */
#include "arm64_instr_format.h"

/* ── Generic raw word access ─────────────────────────────── */
#define ARM64_WORD_AT(addr) \
    ((arm64_instr_t *)(arm64_addr_t)(addr))

#define ARM64_INSTR_AT(addr) \
    ((arm64_instr_u *)(arm64_addr_t)(addr))

/* ── Data Processing — Register ─────────────────────────── */
#define ARM64_INSTR_AT_AND(addr) \
    ((arm64_instr_dp_reg_t *)(arm64_addr_t)(addr))   /* AND  Rd,Rn,Rm  */
#define ARM64_INSTR_AT_ORR(addr) \
    ((arm64_instr_dp_reg_t *)(arm64_addr_t)(addr))   /* ORR  Rd,Rn,Rm  */
#define ARM64_INSTR_AT_EOR(addr) \
    ((arm64_instr_dp_reg_t *)(arm64_addr_t)(addr))   /* EOR  Rd,Rn,Rm  */
#define ARM64_INSTR_AT_ADD(addr) \
    ((arm64_instr_dp_reg_t *)(arm64_addr_t)(addr))   /* ADD  Rd,Rn,Rm  */
#define ARM64_INSTR_AT_SUB(addr) \
    ((arm64_instr_dp_reg_t *)(arm64_addr_t)(addr))   /* SUB  Rd,Rn,Rm  */
#define ARM64_INSTR_AT_MOV(addr) \
    ((arm64_instr_dp_reg_t *)(arm64_addr_t)(addr))   /* MOV  Rd,Rm     */

/* ── Data Processing — Immediate ────────────────────────── */
#define ARM64_INSTR_AT_ADD_IMM(addr) \
    ((arm64_instr_addsub_imm_t *)(arm64_addr_t)(addr)) /* ADD Rd,Rn,#imm */
#define ARM64_INSTR_AT_SUB_IMM(addr) \
    ((arm64_instr_addsub_imm_t *)(arm64_addr_t)(addr)) /* SUB Rd,Rn,#imm */
#define ARM64_INSTR_AT_CMP_IMM(addr) \
    ((arm64_instr_addsub_imm_t *)(arm64_addr_t)(addr)) /* CMP Rn,#imm    */
#define ARM64_INSTR_AT_MOVZ(addr) \
    ((arm64_instr_movwide_t *)(arm64_addr_t)(addr))    /* MOVZ Rd,#imm16 */
#define ARM64_INSTR_AT_MOVN(addr) \
    ((arm64_instr_movwide_t *)(arm64_addr_t)(addr))    /* MOVN Rd,#imm16 */
#define ARM64_INSTR_AT_MOVK(addr) \
    ((arm64_instr_movwide_t *)(arm64_addr_t)(addr))    /* MOVK Rd,#imm16 */

/* ── Load / Store ────────────────────────────────────────── */
#define ARM64_INSTR_AT_LDR(addr) \
    ((arm64_instr_ldst_uimm_t *)(arm64_addr_t)(addr)) /* LDR  Xt,[Xn,#] */
#define ARM64_INSTR_AT_STR(addr) \
    ((arm64_instr_ldst_uimm_t *)(arm64_addr_t)(addr)) /* STR  Xt,[Xn,#] */
#define ARM64_INSTR_AT_LDP(addr) \
    ((arm64_instr_ldst_pair_t *)(arm64_addr_t)(addr)) /* LDP  Xt,Xt2,[Xn]*/
#define ARM64_INSTR_AT_STP(addr) \
    ((arm64_instr_ldst_pair_t *)(arm64_addr_t)(addr)) /* STP  Xt,Xt2,[Xn]*/

/* ── Branch ──────────────────────────────────────────────── */
#define ARM64_INSTR_AT_B(addr) \
    ((arm64_instr_branch_imm_t *)(arm64_addr_t)(addr))  /* B   label    */
#define ARM64_INSTR_AT_BL(addr) \
    ((arm64_instr_branch_imm_t *)(arm64_addr_t)(addr))  /* BL  label    */
#define ARM64_INSTR_AT_BR(addr) \
    ((arm64_instr_branch_reg_t *)(arm64_addr_t)(addr))  /* BR  Xn       */
#define ARM64_INSTR_AT_BLR(addr) \
    ((arm64_instr_branch_reg_t *)(arm64_addr_t)(addr))  /* BLR Xn       */
#define ARM64_INSTR_AT_RET(addr) \
    ((arm64_instr_branch_reg_t *)(arm64_addr_t)(addr))  /* RET          */
#define ARM64_INSTR_AT_BCOND(addr) \
    ((arm64_instr_branch_cond_t *)(arm64_addr_t)(addr)) /* B.cond label */
#define ARM64_INSTR_AT_CBZ(addr) \
    ((arm64_instr_cbz_t *)(arm64_addr_t)(addr))         /* CBZ  Xt,lbl  */
#define ARM64_INSTR_AT_CBNZ(addr) \
    ((arm64_instr_cbz_t *)(arm64_addr_t)(addr))         /* CBNZ Xt,lbl  */
#define ARM64_INSTR_AT_TBZ(addr) \
    ((arm64_instr_tbz_t *)(arm64_addr_t)(addr))         /* TBZ  Xt,#,lb */
#define ARM64_INSTR_AT_TBNZ(addr) \
    ((arm64_instr_tbz_t *)(arm64_addr_t)(addr))         /* TBNZ Xt,#,lb */

/* ── Exception / System ─────────────────────────────────── */
#define ARM64_INSTR_AT_SVC(addr) \
    ((arm64_instr_exception_t *)(arm64_addr_t)(addr))  /* SVC #imm16   */
#define ARM64_INSTR_AT_BRK(addr) \
    ((arm64_instr_exception_t *)(arm64_addr_t)(addr))  /* BRK #imm16   */
#define ARM64_INSTR_AT_MRS(addr) \
    ((arm64_instr_sysreg_t *)(arm64_addr_t)(addr))     /* MRS Xt,sysreg*/
#define ARM64_INSTR_AT_MSR(addr) \
    ((arm64_instr_sysreg_t *)(arm64_addr_t)(addr))     /* MSR sysreg,Xt*/

/* ── Builder macros ──────────────────────────────────────── */
/* Build a 64-bit ADD (register) word: ADD Xd, Xn, Xm */
#define ARM64_BUILD_ADD_REG(Rd, Rn, Rm) \
    ((arm64_uint32_t)( \
        ((1u)   << 31) |   /* sf=1 (64-bit) */ \
        ((0u)   << 29) |   /* opc=00 ADD    */ \
        ((0x0Bu)<< 24) |   /* fixed=01011   */ \
        ((0u)   << 22) |   /* shift=LSL     */ \
        ((0u)   << 21) |   /* N=0           */ \
        (((Rm) & 0x1F) << 16) | \
        ((0u)   << 10) |   /* imm6=0        */ \
        (((Rn) & 0x1F) <<  5) | \
        (((Rd) & 0x1F) <<  0)  ))

/* Build a 64-bit MOVZ word: MOVZ Xd, #imm16 */
#define ARM64_BUILD_MOVZ(Rd, imm16) \
    ((arm64_uint32_t)( \
        ((1u)    << 31) |  /* sf=1 (64-bit)  */ \
        ((0x2u)  << 29) |  /* opc=10 MOVZ    */ \
        ((0x25u) << 23) |  /* fixed=100101   */ \
        ((0u)    << 21) |  /* hw=0 (shift 0) */ \
        (((imm16) & 0xFFFF) << 5) | \
        (((Rd) & 0x1F) <<  0)  ))

/* Build a 64-bit B (unconditional branch) word */
#define ARM64_BUILD_B(imm26) \
    ((arm64_uint32_t)( \
        ((0u)  << 31) |    /* op=0 (B, not BL)  */ \
        ((0x05u) << 26) |  /* fixed=00101        */ \
        ((imm26) & 0x03FFFFFFu) ))

/* Build a BL word */
#define ARM64_BUILD_BL(imm26) \
    ((arm64_uint32_t)( \
        ((1u)  << 31) |    /* op=1 (BL)          */ \
        ((0x05u) << 26) |  /* fixed=00101         */ \
        ((imm26) & 0x03FFFFFFu) ))

/* Build RET X30 */
#define ARM64_BUILD_RET() \
    ((arm64_uint32_t)( \
        ((0x6Bu) << 25) |  /* fixed=1101011       */ \
        ((0x2u)  << 23) |  /* opc=10 (RET)        */ \
        ((0x3u)  << 21) |  /* op2=11              */ \
        ((ARM64_REG_X30) << 5) ))

/* Build SVC #imm16 */
#define ARM64_BUILD_SVC(imm16) \
    ((arm64_uint32_t)( \
        ((0xD4u) << 24) |  /* fixed=11010100      */ \
        ((0x0u)  << 21) |  /* opc=000 (SVC)       */ \
        (((imm16) & 0xFFFF) << 5) | \
        ((0x1u)  <<  0) )) /* LL=01 (EL1)         */

/* PC-relative branch target (from PC, signed imm in instruction units) */
#define ARM64_BRANCH_TARGET(pc, imm26) \
    ((arm64_addr_t)((pc) + (arm64_int64_t)((imm26) << 2)))

#define ARM64_BCOND_TARGET(pc, imm19) \
    ((arm64_addr_t)((pc) + (arm64_int64_t)((imm19) << 2)))

#endif /* ARM64_MACROS_H */
