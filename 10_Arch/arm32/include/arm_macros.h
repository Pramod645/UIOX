#ifndef ARM_MACROS_H
#define ARM_MACROS_H

/*
 * arm_macros.h — Per-instruction address macros
 * Reference: ARM Instruction Set (ARM DDI 0100)
 *
 * Each macro takes a base address and returns the address
 * of the instruction at that location, cast to the
 * appropriate instruction struct pointer so fields can
 * be accessed directly.
 *
 * Usage:
 *   arm_instr_dp_t *p = ARM_INSTR_AT_ADD(base_addr);
 *   p->Rd  = 0;   // destination = R0
 *   p->Rn  = 1;   // source = R1
 *   p->opcode = ARM_OP_ADD;
 */

#include "arm_instr_format.h"

/* ── Generic raw word access ─────────────────────────────── */
#define ARM_WORD_AT(addr) \
    ((arm_instr_t *)(arm_addr_t)(addr))

#define ARM_INSTR_AT(addr) \
    ((arm_instr_u *)(arm_addr_t)(addr))

/* ── Data Processing instructions ───────────────────────── */
#define ARM_INSTR_AT_AND(addr) \
    ((arm_instr_dp_t *)(arm_addr_t)(addr))   /* AND Rd,Rn,Op2  */

#define ARM_INSTR_AT_EOR(addr) \
    ((arm_instr_dp_t *)(arm_addr_t)(addr))   /* EOR Rd,Rn,Op2  */

#define ARM_INSTR_AT_SUB(addr) \
    ((arm_instr_dp_t *)(arm_addr_t)(addr))   /* SUB Rd,Rn,Op2  */

#define ARM_INSTR_AT_RSB(addr) \
    ((arm_instr_dp_t *)(arm_addr_t)(addr))   /* RSB Rd,Rn,Op2  */

#define ARM_INSTR_AT_ADD(addr) \
    ((arm_instr_dp_t *)(arm_addr_t)(addr))   /* ADD Rd,Rn,Op2  */

#define ARM_INSTR_AT_ADC(addr) \
    ((arm_instr_dp_t *)(arm_addr_t)(addr))   /* ADC Rd,Rn,Op2  */

#define ARM_INSTR_AT_SBC(addr) \
    ((arm_instr_dp_t *)(arm_addr_t)(addr))   /* SBC Rd,Rn,Op2  */

#define ARM_INSTR_AT_RSC(addr) \
    ((arm_instr_dp_t *)(arm_addr_t)(addr))   /* RSC Rd,Rn,Op2  */

#define ARM_INSTR_AT_TST(addr) \
    ((arm_instr_dp_t *)(arm_addr_t)(addr))   /* TST Rn,Op2     */

#define ARM_INSTR_AT_TEQ(addr) \
    ((arm_instr_dp_t *)(arm_addr_t)(addr))   /* TEQ Rn,Op2     */

#define ARM_INSTR_AT_CMP(addr) \
    ((arm_instr_dp_t *)(arm_addr_t)(addr))   /* CMP Rn,Op2     */

#define ARM_INSTR_AT_CMN(addr) \
    ((arm_instr_dp_t *)(arm_addr_t)(addr))   /* CMN Rn,Op2     */

#define ARM_INSTR_AT_ORR(addr) \
    ((arm_instr_dp_t *)(arm_addr_t)(addr))   /* ORR Rd,Rn,Op2  */

#define ARM_INSTR_AT_MOV(addr) \
    ((arm_instr_dp_t *)(arm_addr_t)(addr))   /* MOV Rd,Op2     */

#define ARM_INSTR_AT_BIC(addr) \
    ((arm_instr_dp_t *)(arm_addr_t)(addr))   /* BIC Rd,Rn,Op2  */

#define ARM_INSTR_AT_MVN(addr) \
    ((arm_instr_dp_t *)(arm_addr_t)(addr))   /* MVN Rd,Op2     */

/* ── Multiply instructions ───────────────────────────────── */
#define ARM_INSTR_AT_MUL(addr) \
    ((arm_instr_mul_t *)(arm_addr_t)(addr))  /* MUL Rd,Rm,Rs   */

#define ARM_INSTR_AT_MLA(addr) \
    ((arm_instr_mul_t *)(arm_addr_t)(addr))  /* MLA Rd,Rm,Rs,Rn*/

#define ARM_INSTR_AT_UMULL(addr) \
    ((arm_instr_mull_t *)(arm_addr_t)(addr)) /* UMULL RdLo,RdHi,Rm,Rs */

#define ARM_INSTR_AT_UMLAL(addr) \
    ((arm_instr_mull_t *)(arm_addr_t)(addr)) /* UMLAL RdLo,RdHi,Rm,Rs */

#define ARM_INSTR_AT_SMULL(addr) \
    ((arm_instr_mull_t *)(arm_addr_t)(addr)) /* SMULL RdLo,RdHi,Rm,Rs */

#define ARM_INSTR_AT_SMLAL(addr) \
    ((arm_instr_mull_t *)(arm_addr_t)(addr)) /* SMLAL RdLo,RdHi,Rm,Rs */

/* ── Data Swap instructions ──────────────────────────────── */
#define ARM_INSTR_AT_SWP(addr) \
    ((arm_instr_swp_t *)(arm_addr_t)(addr))  /* SWP  Rd,Rm,[Rn]*/

#define ARM_INSTR_AT_SWPB(addr) \
    ((arm_instr_swp_t *)(arm_addr_t)(addr))  /* SWPB Rd,Rm,[Rn]*/

/* ── Branch instructions ─────────────────────────────────── */
#define ARM_INSTR_AT_B(addr) \
    ((arm_instr_branch_t *)(arm_addr_t)(addr))  /* B  label  */

#define ARM_INSTR_AT_BL(addr) \
    ((arm_instr_branch_t *)(arm_addr_t)(addr))  /* BL label  */

#define ARM_INSTR_AT_BX(addr) \
    ((arm_instr_bx_t *)(arm_addr_t)(addr))      /* BX Rn     */

/* ── Load/Store instructions ─────────────────────────────── */
#define ARM_INSTR_AT_LDR(addr) \
    ((arm_instr_sdt_t *)(arm_addr_t)(addr))  /* LDR  Rd,[Rn]  */

#define ARM_INSTR_AT_STR(addr) \
    ((arm_instr_sdt_t *)(arm_addr_t)(addr))  /* STR  Rd,[Rn]  */

#define ARM_INSTR_AT_LDRB(addr) \
    ((arm_instr_sdt_t *)(arm_addr_t)(addr))  /* LDRB Rd,[Rn]  */

#define ARM_INSTR_AT_STRB(addr) \
    ((arm_instr_sdt_t *)(arm_addr_t)(addr))  /* STRB Rd,[Rn]  */

#define ARM_INSTR_AT_LDRH(addr) \
    ((arm_instr_hwdt_t *)(arm_addr_t)(addr)) /* LDRH Rd,[Rn]  */

#define ARM_INSTR_AT_STRH(addr) \
    ((arm_instr_hwdt_t *)(arm_addr_t)(addr)) /* STRH Rd,[Rn]  */

#define ARM_INSTR_AT_LDRSB(addr) \
    ((arm_instr_hwdt_t *)(arm_addr_t)(addr)) /* LDRSB Rd,[Rn] */

#define ARM_INSTR_AT_LDRSH(addr) \
    ((arm_instr_hwdt_t *)(arm_addr_t)(addr)) /* LDRSH Rd,[Rn] */

/* ── Block Transfer instructions ─────────────────────────── */
#define ARM_INSTR_AT_LDM(addr) \
    ((arm_instr_bdt_t *)(arm_addr_t)(addr))  /* LDM  Rn,{list}*/

#define ARM_INSTR_AT_STM(addr) \
    ((arm_instr_bdt_t *)(arm_addr_t)(addr))  /* STM  Rn,{list}*/

#define ARM_INSTR_AT_LDMIA(addr) \
    ((arm_instr_bdt_t *)(arm_addr_t)(addr))  /* LDMIA — post-inc */

#define ARM_INSTR_AT_LDMIB(addr) \
    ((arm_instr_bdt_t *)(arm_addr_t)(addr))  /* LDMIB — pre-inc  */

#define ARM_INSTR_AT_LDMDA(addr) \
    ((arm_instr_bdt_t *)(arm_addr_t)(addr))  /* LDMDA — post-dec */

#define ARM_INSTR_AT_LDMDB(addr) \
    ((arm_instr_bdt_t *)(arm_addr_t)(addr))  /* LDMDB — pre-dec  */

#define ARM_INSTR_AT_STMIA(addr) \
    ((arm_instr_bdt_t *)(arm_addr_t)(addr))  /* STMIA — post-inc */

#define ARM_INSTR_AT_STMIB(addr) \
    ((arm_instr_bdt_t *)(arm_addr_t)(addr))  /* STMIB — pre-inc  */

#define ARM_INSTR_AT_STMDA(addr) \
    ((arm_instr_bdt_t *)(arm_addr_t)(addr))  /* STMDA — post-dec */

#define ARM_INSTR_AT_STMDB(addr) \
    ((arm_instr_bdt_t *)(arm_addr_t)(addr))  /* STMDB — pre-dec  */

/* ── PSR Transfer instructions ───────────────────────────── */
#define ARM_INSTR_AT_MRS(addr) \
    ((arm_instr_psr_t *)(arm_addr_t)(addr))  /* MRS Rd,CPSR/SPSR */

#define ARM_INSTR_AT_MSR(addr) \
    ((arm_instr_psr_t *)(arm_addr_t)(addr))  /* MSR CPSR/SPSR,Op */

/* ── Software Interrupt ──────────────────────────────────── */
#define ARM_INSTR_AT_SWI(addr) \
    ((arm_instr_swi_t *)(arm_addr_t)(addr))  /* SWI <imm24>   */

/* ── Coprocessor instructions ────────────────────────────── */
#define ARM_INSTR_AT_CDP(addr) \
    ((arm_instr_cdp_t *)(arm_addr_t)(addr))  /* CDP  cpN,...  */

#define ARM_INSTR_AT_LDC(addr) \
    ((arm_instr_cdt_t *)(arm_addr_t)(addr))  /* LDC  cpN,...  */

#define ARM_INSTR_AT_STC(addr) \
    ((arm_instr_cdt_t *)(arm_addr_t)(addr))  /* STC  cpN,...  */

#define ARM_INSTR_AT_MCR(addr) \
    ((arm_instr_crt_t *)(arm_addr_t)(addr))  /* MCR  cpN,...  */

#define ARM_INSTR_AT_MRC(addr) \
    ((arm_instr_crt_t *)(arm_addr_t)(addr))  /* MRC  cpN,...  */

/* ════════════════════════════════════════════════════════════
   INSTRUCTION BUILDER MACROS
   Build a complete encoded instruction word from fields.
   ════════════════════════════════════════════════════════════ */

/* Build data processing instruction word */
#define ARM_BUILD_DP(cond,I,opcode,S,Rn,Rd,op2) \
    ( (((arm_uint32_t)(cond))   << 28) | \
      (((arm_uint32_t)(I))      << 25) | \
      (((arm_uint32_t)(opcode)) << 21) | \
      (((arm_uint32_t)(S))      << 20) | \
      (((arm_uint32_t)(Rn))     << 16) | \
      (((arm_uint32_t)(Rd))     << 12) | \
      (((arm_uint32_t)(op2))    &  0xFFF) )

/* Build branch instruction word */
#define ARM_BUILD_B(cond,L,offset24) \
    ( (((arm_uint32_t)(cond))     << 28) | \
      (0x5u                       << 25) | \
      (((arm_uint32_t)(L))        << 24) | \
      (((arm_uint32_t)(offset24)) & 0x00FFFFFFu) )

/* Build BX instruction word */
#define ARM_BUILD_BX(cond,Rn) \
    ( (((arm_uint32_t)(cond)) << 28) | \
      (0x012FFF10u)           |         \
      (((arm_uint32_t)(Rn))  & 0xFu) )

/* Build LDR/STR instruction word */
#define ARM_BUILD_SDT(cond,I,P,U,B,W,L,Rn,Rd,offset) \
    ( (((arm_uint32_t)(cond))   << 28) | \
      (1u                       << 26) | \
      (((arm_uint32_t)(I))      << 25) | \
      (((arm_uint32_t)(P))      << 24) | \
      (((arm_uint32_t)(U))      << 23) | \
      (((arm_uint32_t)(B))      << 22) | \
      (((arm_uint32_t)(W))      << 21) | \
      (((arm_uint32_t)(L))      << 20) | \
      (((arm_uint32_t)(Rn))     << 16) | \
      (((arm_uint32_t)(Rd))     << 12) | \
      (((arm_uint32_t)(offset)) & 0xFFFu) )

/* Build LDM/STM instruction word */
#define ARM_BUILD_BDT(cond,P,U,S,W,L,Rn,reg_list) \
    ( (((arm_uint32_t)(cond))     << 28) | \
      (4u                         << 25) | \
      (((arm_uint32_t)(P))        << 24) | \
      (((arm_uint32_t)(U))        << 23) | \
      (((arm_uint32_t)(S))        << 22) | \
      (((arm_uint32_t)(W))        << 21) | \
      (((arm_uint32_t)(L))        << 20) | \
      (((arm_uint32_t)(Rn))       << 16) | \
      (((arm_uint32_t)(reg_list)) & 0xFFFFu) )

/* Build MUL instruction word */
#define ARM_BUILD_MUL(cond,A,S,Rd,Rn,Rs,Rm) \
    ( (((arm_uint32_t)(cond)) << 28) | \
      (((arm_uint32_t)(A))    << 21) | \
      (((arm_uint32_t)(S))    << 20) | \
      (((arm_uint32_t)(Rd))   << 16) | \
      (((arm_uint32_t)(Rn))   << 12) | \
      (((arm_uint32_t)(Rs))   <<  8) | \
      (0x9u                   <<  4) | \
      (((arm_uint32_t)(Rm))   & 0xFu) )

/* Build SWI instruction word */
#define ARM_BUILD_SWI(cond,imm24) \
    ( (((arm_uint32_t)(cond))   << 28) | \
      (0xFu                     << 24) | \
      (((arm_uint32_t)(imm24))  & 0x00FFFFFFu) )

/* Build MRS instruction word */
#define ARM_BUILD_MRS(cond,P,Rd) \
    ( (((arm_uint32_t)(cond)) << 28) | \
      (0x10u                  << 23) | \
      (((arm_uint32_t)(P))    << 22) | \
      (0xFu                   << 16) | \
      (((arm_uint32_t)(Rd))   << 12) )

/* Build MSR (register) instruction word */
#define ARM_BUILD_MSR_REG(cond,P,field,Rm) \
    ( (((arm_uint32_t)(cond))  << 28) | \
      (0x12u                   << 23) | \
      (((arm_uint32_t)(P))     << 22) | \
      (((arm_uint32_t)(field)) << 16) | \
      (0xFu                    << 12) | \
      (((arm_uint32_t)(Rm))    & 0xFu) )

/* Build MCR/MRC instruction word */
#define ARM_BUILD_CRT(cond,CP_opc,L,CRn,Rd,cp_num,CP,CRm) \
    ( (((arm_uint32_t)(cond))   << 28) | \
      (0xEu                     << 24) | \
      (((arm_uint32_t)(CP_opc)) << 21) | \
      (((arm_uint32_t)(L))      << 20) | \
      (((arm_uint32_t)(CRn))    << 16) | \
      (((arm_uint32_t)(Rd))     << 12) | \
      (((arm_uint32_t)(cp_num)) <<  8) | \
      (((arm_uint32_t)(CP))     <<  5) | \
      (1u                       <<  4) | \
      (((arm_uint32_t)(CRm))    & 0xFu) )

/* ════════════════════════════════════════════════════════════
   CONDITION SHORTHAND MACROS
   Build common condition variants of instructions
   ════════════════════════════════════════════════════════════ */

/* ALways condition — most common */
#define ARM_AL  ARM_COND_AL

/* Condition-prefixed DP builders */
#define ARM_BUILD_MOVEQ(Rd,op2) ARM_BUILD_DP(ARM_COND_EQ,1,ARM_OP_MOV,0,0,Rd,op2)
#define ARM_BUILD_MOVNE(Rd,op2) ARM_BUILD_DP(ARM_COND_NE,1,ARM_OP_MOV,0,0,Rd,op2)
#define ARM_BUILD_MOVAL(Rd,op2) ARM_BUILD_DP(ARM_COND_AL,1,ARM_OP_MOV,0,0,Rd,op2)
#define ARM_BUILD_ADDAL(Rd,Rn,op2) ARM_BUILD_DP(ARM_COND_AL,0,ARM_OP_ADD,0,Rn,Rd,op2)
#define ARM_BUILD_SUBAL(Rd,Rn,op2) ARM_BUILD_DP(ARM_COND_AL,0,ARM_OP_SUB,0,Rn,Rd,op2)

/* Condition-prefixed branch builders */
#define ARM_BUILD_BEQ(off24)  ARM_BUILD_B(ARM_COND_EQ,0,off24)
#define ARM_BUILD_BNE(off24)  ARM_BUILD_B(ARM_COND_NE,0,off24)
#define ARM_BUILD_BAL(off24)  ARM_BUILD_B(ARM_COND_AL,0,off24)
#define ARM_BUILD_BLAL(off24) ARM_BUILD_B(ARM_COND_AL,1,off24)
#define ARM_BUILD_BGT(off24)  ARM_BUILD_B(ARM_COND_GT,0,off24)
#define ARM_BUILD_BLT(off24)  ARM_BUILD_B(ARM_COND_LT,0,off24)
#define ARM_BUILD_BGE(off24)  ARM_BUILD_B(ARM_COND_GE,0,off24)
#define ARM_BUILD_BLE(off24)  ARM_BUILD_B(ARM_COND_LE,0,off24)

/* Common LDR/STR shorthands */
#define ARM_BUILD_LDRAL(Rd,Rn,off) \
    ARM_BUILD_SDT(ARM_COND_AL,0,1,1,0,0,1,Rn,Rd,off)
#define ARM_BUILD_STRAL(Rd,Rn,off) \
    ARM_BUILD_SDT(ARM_COND_AL,0,1,1,0,0,0,Rn,Rd,off)
#define ARM_BUILD_LDRBAL(Rd,Rn,off) \
    ARM_BUILD_SDT(ARM_COND_AL,0,1,1,1,0,1,Rn,Rd,off)
#define ARM_BUILD_STRBAL(Rd,Rn,off) \
    ARM_BUILD_SDT(ARM_COND_AL,0,1,1,1,0,0,Rn,Rd,off)

/* PUSH/POP (STMDB SP! and LDMIA SP!) shorthands */
#define ARM_BUILD_PUSH(reg_list) \
    ARM_BUILD_BDT(ARM_COND_AL,1,0,0,1,0,ARM_REG_SP,reg_list)
#define ARM_BUILD_POP(reg_list) \
    ARM_BUILD_BDT(ARM_COND_AL,0,1,0,1,1,ARM_REG_SP,reg_list)

/* NOP = MOV R0,R0 */
#define ARM_BUILD_NOP() \
    ARM_BUILD_DP(ARM_COND_AL,0,ARM_OP_MOV,0,0,0,0)

#endif /* ARM_MACROS_H */
