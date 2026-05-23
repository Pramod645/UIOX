#ifndef ARM_OPCODES_H
#define ARM_OPCODES_H

/*
 * arm_opcodes.h — ARM instruction opcode constants
 * Reference: ARM Instruction Set, Sections 3-5
 */

#include "arm_types.h"

/* ── Instruction class bits [27:26] ─────────────────────── */
#define ARM_CLASS_DATA   0x0   /* Data processing / PSR       */
#define ARM_CLASS_MUL    0x0   /* Multiply (bits[7:4]=1001)   */
#define ARM_CLASS_LDST   0x1   /* Load/store                  */
#define ARM_CLASS_LDSTM  0x2   /* Load/store multiple         */
#define ARM_CLASS_BRANCH 0x2   /* Branch                      */
#define ARM_CLASS_COPROC 0x3   /* Coprocessor                 */
#define ARM_CLASS_SWI    0x3   /* Software interrupt          */

/* ── Data processing opcodes bits[24:21] ────────────────── */
#define ARM_OP_AND  0x0   /* AND   Rd = Rn AND Op2           */
#define ARM_OP_EOR  0x1   /* EOR   Rd = Rn EOR Op2           */
#define ARM_OP_SUB  0x2   /* SUB   Rd = Rn - Op2             */
#define ARM_OP_RSB  0x3   /* RSB   Rd = Op2 - Rn             */
#define ARM_OP_ADD  0x4   /* ADD   Rd = Rn + Op2             */
#define ARM_OP_ADC  0x5   /* ADC   Rd = Rn + Op2 + C         */
#define ARM_OP_SBC  0x6   /* SBC   Rd = Rn - Op2 - (1-C)    */
#define ARM_OP_RSC  0x7   /* RSC   Rd = Op2 - Rn - (1-C)    */
#define ARM_OP_TST  0x8   /* TST   set cc on Rn AND Op2      */
#define ARM_OP_TEQ  0x9   /* TEQ   set cc on Rn EOR Op2      */
#define ARM_OP_CMP  0xA   /* CMP   set cc on Rn - Op2        */
#define ARM_OP_CMN  0xB   /* CMN   set cc on Rn + Op2        */
#define ARM_OP_ORR  0xC   /* ORR   Rd = Rn OR Op2            */
#define ARM_OP_MOV  0xD   /* MOV   Rd = Op2                  */
#define ARM_OP_BIC  0xE   /* BIC   Rd = Rn AND NOT Op2       */
#define ARM_OP_MVN  0xF   /* MVN   Rd = NOT Op2              */

/* ── Shift types bits[6:5] ──────────────────────────────── */
#define ARM_SHIFT_LSL  0x0   /* Logical shift left           */
#define ARM_SHIFT_LSR  0x1   /* Logical shift right          */
#define ARM_SHIFT_ASR  0x2   /* Arithmetic shift right       */
#define ARM_SHIFT_ROR  0x3   /* Rotate right                 */

/* ── Load/Store opcodes ──────────────────────────────────── */
#define ARM_LS_STR   0x0   /* Store register                 */
#define ARM_LS_LDR   0x1   /* Load register                  */
#define ARM_LS_STRB  0x2   /* Store byte                     */
#define ARM_LS_LDRB  0x3   /* Load byte                      */
#define ARM_LS_STRH  0x4   /* Store halfword                 */
#define ARM_LS_LDRH  0x5   /* Load halfword                  */
#define ARM_LS_LDRSB 0x6   /* Load signed byte               */
#define ARM_LS_LDRSH 0x7   /* Load signed halfword           */

/* ── Multiply opcodes ────────────────────────────────────── */
#define ARM_MUL_MUL   0x0   /* Rd = Rm * Rs                  */
#define ARM_MUL_MLA   0x1   /* Rd = Rm * Rs + Rn             */
#define ARM_MUL_UMULL 0x4   /* RdHi:RdLo = Rm * Rs unsigned  */
#define ARM_MUL_UMLAL 0x5   /* RdHi:RdLo += Rm * Rs unsigned */
#define ARM_MUL_SMULL 0x6   /* RdHi:RdLo = Rm * Rs signed    */
#define ARM_MUL_SMLAL 0x7   /* RdHi:RdLo += Rm * Rs signed   */

/* ── Coprocessor opcodes ─────────────────────────────────── */
#define ARM_CP_CDP   0x0   /* Coprocessor data processing    */
#define ARM_CP_LDC   0x1   /* Load coprocessor               */
#define ARM_CP_STC   0x2   /* Store coprocessor              */
#define ARM_CP_MCR   0x4   /* Move ARM reg to CP reg         */
#define ARM_CP_MRC   0x5   /* Move CP reg to ARM reg         */

/* ── PSR transfer ────────────────────────────────────────── */
#define ARM_PSR_MRS  0x0   /* Move PSR to register           */
#define ARM_PSR_MSR  0x1   /* Move register to PSR           */

#endif /* ARM_OPCODES_H */
