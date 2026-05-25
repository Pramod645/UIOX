#ifndef ARM64_OPCODES_H
#define ARM64_OPCODES_H
/*
 * arm64_opcodes.h — AArch64 instruction opcode constants
 * Reference: ARM DDI 0487, Chapter C3 (A64 Instruction Set)
 *
 * A64 encoding: bits [28:25] identify the top-level class.
 */
#include "arm64_types.h"

/* ── Instruction class bits [28:25] ───────────────────────── */
#define ARM64_CLASS_RESERVED    0x0   /* Reserved                  */
#define ARM64_CLASS_SME         0x1   /* SME encodings             */
#define ARM64_CLASS_UNALLOC     0x2   /* Unallocated               */
#define ARM64_CLASS_SVE         0x3   /* SVE encodings             */
#define ARM64_CLASS_DPIMM       0x8   /* Data processing immediate */
#define ARM64_CLASS_BRANCH      0xA   /* Branches, exceptions, sys */
#define ARM64_CLASS_LDST        0xC   /* Load/store                */
#define ARM64_CLASS_DPREG       0xD   /* Data processing register  */
#define ARM64_CLASS_FPSIMD      0xE   /* FP / SIMD                 */

/* ── Data processing (immediate) opcodes ─────────────────── */
#define ARM64_OP_ADD_IMM   0x11   /* ADD  Rd, Rn, #imm12          */
#define ARM64_OP_ADDS_IMM  0x31   /* ADDS Rd, Rn, #imm12          */
#define ARM64_OP_SUB_IMM   0x51   /* SUB  Rd, Rn, #imm12          */
#define ARM64_OP_SUBS_IMM  0x71   /* SUBS Rd, Rn, #imm12          */
#define ARM64_OP_MOV_WIDE  0x12   /* MOVZ/MOVN/MOVK               */
#define ARM64_OP_BITFIELD  0x13   /* SBFM/BFM/UBFM                */
#define ARM64_OP_EXTRACT   0x13   /* EXTR                         */
#define ARM64_OP_LOGICAL_IMM 0x12 /* AND/ORR/EOR/ANDS imm         */

/* ── Data processing (register) opcodes ──────────────────── */
#define ARM64_OP_AND   0x0   /* AND  Rd, Rn, Rm{, shift}         */
#define ARM64_OP_BIC   0x1   /* BIC  Rd, Rn, Rm{, shift}         */
#define ARM64_OP_ORR   0x2   /* ORR  Rd, Rn, Rm{, shift}         */
#define ARM64_OP_ORN   0x3   /* ORN  Rd, Rn, Rm{, shift}         */
#define ARM64_OP_EOR   0x4   /* EOR  Rd, Rn, Rm{, shift}         */
#define ARM64_OP_EON   0x5   /* EON  Rd, Rn, Rm{, shift}         */
#define ARM64_OP_ANDS  0x6   /* ANDS Rd, Rn, Rm{, shift}         */
#define ARM64_OP_BICS  0x7   /* BICS Rd, Rn, Rm{, shift}         */
#define ARM64_OP_ADD   0x8   /* ADD  Rd, Rn, Rm{, shift}         */
#define ARM64_OP_ADDS  0x9   /* ADDS Rd, Rn, Rm{, shift}  (CMN)  */
#define ARM64_OP_SUB   0xA   /* SUB  Rd, Rn, Rm{, shift}         */
#define ARM64_OP_SUBS  0xB   /* SUBS Rd, Rn, Rm{, shift}  (CMP)  */
#define ARM64_OP_MOV   ARM64_OP_ORR   /* MOV alias: ORR Rd,XZR,Rm */
#define ARM64_OP_MVN   ARM64_OP_ORN   /* MVN alias: ORN Rd,XZR,Rm */
#define ARM64_OP_NEG   ARM64_OP_SUB   /* NEG alias: SUB Rd,XZR,Rm */
#define ARM64_OP_TST   ARM64_OP_ANDS  /* TST alias: ANDS XZR,Rn,Rm*/

/* ── Multiply opcodes ─────────────────────────────────────── */
#define ARM64_OP_MADD  0x0   /* MADD  Rd = Ra + Rn*Rm            */
#define ARM64_OP_MSUB  0x1   /* MSUB  Rd = Ra - Rn*Rm            */
#define ARM64_OP_SMULH 0x2   /* SMULH Xd = (Xn*Xm)[127:64] sig. */
#define ARM64_OP_UMULH 0x3   /* UMULH Xd = (Xn*Xm)[127:64] uns. */
#define ARM64_OP_SMADDL 0x4  /* SMADDL Xd = Xa + Wn*Wm signed   */
#define ARM64_OP_UMADDL 0x5  /* UMADDL Xd = Xa + Wn*Wm unsigned */

/* ── Shift types (bits [23:22] in shifted-register form) ─── */
#define ARM64_SHIFT_LSL  0x0  /* Logical shift left               */
#define ARM64_SHIFT_LSR  0x1  /* Logical shift right              */
#define ARM64_SHIFT_ASR  0x2  /* Arithmetic shift right           */
#define ARM64_SHIFT_ROR  0x3  /* Rotate right                     */

/* ── Load/store opcodes ───────────────────────────────────── */
#define ARM64_OP_LDR   0x0   /* LDR  Xt, [Xn, #imm]             */
#define ARM64_OP_STR   0x1   /* STR  Xt, [Xn, #imm]             */
#define ARM64_OP_LDRB  0x2   /* LDRB Wt, [Xn, #imm]             */
#define ARM64_OP_STRB  0x3   /* STRB Wt, [Xn, #imm]             */
#define ARM64_OP_LDRH  0x4   /* LDRH Wt, [Xn, #imm]             */
#define ARM64_OP_STRH  0x5   /* STRH Wt, [Xn, #imm]             */
#define ARM64_OP_LDRSB 0x6   /* LDRSB (sign-extend byte)         */
#define ARM64_OP_LDRSH 0x7   /* LDRSH (sign-extend half)         */
#define ARM64_OP_LDRSW 0x8   /* LDRSW (sign-extend word)         */
#define ARM64_OP_LDP   0x9   /* LDP  Xt1,Xt2, [Xn, #imm]        */
#define ARM64_OP_STP   0xA   /* STP  Xt1,Xt2, [Xn, #imm]        */

/* ── Branch opcodes ───────────────────────────────────────── */
#define ARM64_OP_B     0x0   /* B   label (unconditional)        */
#define ARM64_OP_BL    0x1   /* BL  label (branch with link)     */
#define ARM64_OP_BR    0x2   /* BR  Xn   (branch to register)    */
#define ARM64_OP_BLR   0x3   /* BLR Xn   (branch+link register)  */
#define ARM64_OP_RET   0x4   /* RET Xn   (return)                */
#define ARM64_OP_B_COND 0x5  /* B.cond label                     */
#define ARM64_OP_CBZ   0x6   /* CBZ  Xt, label                   */
#define ARM64_OP_CBNZ  0x7   /* CBNZ Xt, label                   */
#define ARM64_OP_TBZ   0x8   /* TBZ  Xt, #imm, label             */
#define ARM64_OP_TBNZ  0x9   /* TBNZ Xt, #imm, label             */

/* ── System / exception opcodes ──────────────────────────── */
#define ARM64_OP_SVC   0x0   /* SVC #imm16 (supervisor call)     */
#define ARM64_OP_HVC   0x1   /* HVC #imm16 (hypervisor call)     */
#define ARM64_OP_SMC   0x2   /* SMC #imm16 (secure monitor call) */
#define ARM64_OP_BRK   0x3   /* BRK #imm16 (breakpoint)          */
#define ARM64_OP_HLT   0x4   /* HLT #imm16 (halt, debug)         */
#define ARM64_OP_ERET  0x5   /* ERET (exception return)          */
#define ARM64_OP_MRS   0x6   /* MRS Xt, sysreg                   */
#define ARM64_OP_MSR   0x7   /* MSR sysreg, Xt                   */

#endif /* ARM64_OPCODES_H */
