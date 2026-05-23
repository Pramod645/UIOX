#ifndef ARM_INSTR_FORMAT_H
#define ARM_INSTR_FORMAT_H

/*
 * arm_instr_format.h — ARM instruction encoding structures
 * Reference: ARM Instruction Set (ARM DDI 0100), Chapter 3
 *
 * Every ARM instruction is 32 bits.
 * Bits [31:28] = condition code (all formats).
 * The remaining bits define the instruction class and fields.
 *
 * Each struct maps exactly onto a 32-bit instruction word.
 * Bit-field ordering: LSB first (little-endian bitfield layout).
 */

#include "arm_types.h"
#include "arm_opcodes.h"

/* ══════════════════════════════════════════════════════════
   FORMAT 1 — Data Processing (register and immediate)
   Bits: [31:28]=cond [27:26]=00 [25]=I [24:21]=opcode
         [20]=S [19:16]=Rn [15:12]=Rd [11:0]=Op2
   ══════════════════════════════════════════════════════════ */

/* Operand2 when I=0 (register shifted by immediate) */
typedef struct arm_dp_op2_imm_shift {
    arm_uint32_t Rm       : 4;   /* [3:0]   source register         */
    arm_uint32_t reserved : 1;   /* [4]     must be 0               */
    arm_uint32_t sh_type  : 2;   /* [6:5]   shift type              */
    arm_uint32_t sh_imm   : 5;   /* [11:7]  shift amount 0-31       */
} arm_dp_op2_imm_shift_t;

/* Operand2 when I=0 (register shifted by register) */
typedef struct arm_dp_op2_reg_shift {
    arm_uint32_t Rm       : 4;   /* [3:0]   source register         */
    arm_uint32_t reserved : 1;   /* [4]     must be 1               */
    arm_uint32_t sh_type  : 2;   /* [6:5]   shift type              */
    arm_uint32_t Rs       : 4;   /* [11:8]  shift register          */
    arm_uint32_t pad      : 1;   /* [11] padding                    */
} arm_dp_op2_reg_shift_t;

/* Operand2 when I=1 (immediate value rotated) */
typedef struct arm_dp_op2_imm {
    arm_uint32_t imm8     : 8;   /* [7:0]   8-bit immediate         */
    arm_uint32_t rotate   : 4;   /* [11:8]  rotate amount / 2       */
} arm_dp_op2_imm_t;

/* Full data processing instruction */
typedef struct arm_instr_dp {
    arm_uint32_t op2      : 12;  /* [11:0]  Operand 2               */
    arm_uint32_t Rd       : 4;   /* [15:12] Destination register    */
    arm_uint32_t Rn       : 4;   /* [19:16] First operand register  */
    arm_uint32_t S        : 1;   /* [20]    Set condition codes      */
    arm_uint32_t opcode   : 4;   /* [24:21] Operation code          */
    arm_uint32_t I        : 1;   /* [25]    Immediate operand flag   */
    arm_uint32_t cls      : 2;   /* [27:26] Must be 00              */
    arm_uint32_t cond     : 4;   /* [31:28] Condition code          */
} arm_instr_dp_t;

/* ══════════════════════════════════════════════════════════
   FORMAT 2 — Multiply and Multiply-Accumulate
   MUL/MLA: [27:22]=000000 [21]=A [20]=S
            [19:16]=Rd [15:12]=Rn [11:8]=Rs [7:4]=1001 [3:0]=Rm
   ══════════════════════════════════════════════════════════ */
typedef struct arm_instr_mul {
    arm_uint32_t Rm       : 4;   /* [3:0]   Multiplier register     */
    arm_uint32_t magic    : 4;   /* [7:4]   Must be 1001            */
    arm_uint32_t Rs       : 4;   /* [11:8]  Multiplicand register   */
    arm_uint32_t Rn       : 4;   /* [15:12] Accumulate register     */
    arm_uint32_t Rd       : 4;   /* [19:16] Destination register    */
    arm_uint32_t S        : 1;   /* [20]    Set condition codes      */
    arm_uint32_t A        : 1;   /* [21]    Accumulate flag          */
    arm_uint32_t cls      : 6;   /* [27:22] Must be 000000          */
    arm_uint32_t cond     : 4;   /* [31:28] Condition code          */
} arm_instr_mul_t;

/* ══════════════════════════════════════════════════════════
   FORMAT 3 — Multiply Long (UMULL/UMLAL/SMULL/SMLAL)
   [27:23]=00001 [22]=U [21]=A [20]=S
   [19:16]=RdHi [15:12]=RdLo [11:8]=Rs [7:4]=1001 [3:0]=Rm
   ══════════════════════════════════════════════════════════ */
typedef struct arm_instr_mull {
    arm_uint32_t Rm       : 4;   /* [3:0]   Multiplier              */
    arm_uint32_t magic    : 4;   /* [7:4]   Must be 1001            */
    arm_uint32_t Rs       : 4;   /* [11:8]  Multiplicand            */
    arm_uint32_t RdLo     : 4;   /* [15:12] Destination low word    */
    arm_uint32_t RdHi     : 4;   /* [19:16] Destination high word   */
    arm_uint32_t S        : 1;   /* [20]    Set condition codes      */
    arm_uint32_t A        : 1;   /* [21]    Accumulate               */
    arm_uint32_t U        : 1;   /* [22]    Unsigned flag            */
    arm_uint32_t cls      : 5;   /* [27:23] Must be 00001           */
    arm_uint32_t cond     : 4;   /* [31:28] Condition code          */
} arm_instr_mull_t;

/* ══════════════════════════════════════════════════════════
   FORMAT 4 — Single Data Swap (SWP/SWPB)
   [27:23]=00010 [22]=B [21:20]=00 [19:16]=Rn
   [15:12]=Rd [11:8]=0000 [7:4]=1001 [3:0]=Rm
   ══════════════════════════════════════════════════════════ */
typedef struct arm_instr_swp {
    arm_uint32_t Rm       : 4;   /* [3:0]   Source register         */
    arm_uint32_t magic    : 4;   /* [7:4]   Must be 1001            */
    arm_uint32_t pad      : 4;   /* [11:8]  Must be 0000            */
    arm_uint32_t Rd       : 4;   /* [15:12] Destination register    */
    arm_uint32_t Rn       : 4;   /* [19:16] Base register           */
    arm_uint32_t cls2     : 2;   /* [21:20] Must be 00              */
    arm_uint32_t B        : 1;   /* [22]    Byte swap flag          */
    arm_uint32_t cls      : 5;   /* [27:23] Must be 00010           */
    arm_uint32_t cond     : 4;   /* [31:28] Condition code          */
} arm_instr_swp_t;

/* ══════════════════════════════════════════════════════════
   FORMAT 5 — Branch and Branch with Link (B/BL)
   [27:25]=101 [24]=L [23:0]=offset (24-bit signed)
   Target = PC + 8 + (offset << 2)
   ══════════════════════════════════════════════════════════ */
typedef struct arm_instr_branch {
    arm_uint32_t offset   : 24;  /* [23:0]  Signed 24-bit offset    */
    arm_uint32_t L        : 1;   /* [24]    Link flag (BL if 1)     */
    arm_uint32_t cls      : 3;   /* [27:25] Must be 101             */
    arm_uint32_t cond     : 4;   /* [31:28] Condition code          */
} arm_instr_branch_t;

/* ══════════════════════════════════════════════════════════
   FORMAT 6 — Branch and Exchange (BX)
   [27:4]=000100101111111111110001 [3:0]=Rn
   ══════════════════════════════════════════════════════════ */
typedef struct arm_instr_bx {
    arm_uint32_t Rn       : 4;   /* [3:0]   Register holding target */
    arm_uint32_t magic    : 24;  /* [27:4]  Must be 000100101111... */
    arm_uint32_t cond     : 4;   /* [31:28] Condition code          */
} arm_instr_bx_t;

/* ══════════════════════════════════════════════════════════
   FORMAT 7 — Single Data Transfer (LDR/STR)
   [27:26]=01 [25]=I [24]=P [23]=U [22]=B [21]=W [20]=L
   [19:16]=Rn [15:12]=Rd [11:0]=Offset
   ══════════════════════════════════════════════════════════ */
typedef struct arm_instr_sdt {
    arm_uint32_t offset   : 12;  /* [11:0]  Offset (imm or reg)     */
    arm_uint32_t Rd       : 4;   /* [15:12] Dest/src register       */
    arm_uint32_t Rn       : 4;   /* [19:16] Base register           */
    arm_uint32_t L        : 1;   /* [20]    Load (1) or Store (0)   */
    arm_uint32_t W        : 1;   /* [21]    Writeback flag          */
    arm_uint32_t B        : 1;   /* [22]    Byte (1) or Word (0)    */
    arm_uint32_t U        : 1;   /* [23]    Up (1) or Down (0)      */
    arm_uint32_t P        : 1;   /* [24]    Pre (1) or Post (0)     */
    arm_uint32_t I        : 1;   /* [25]    Immediate offset flag   */
    arm_uint32_t cls      : 2;   /* [27:26] Must be 01              */
    arm_uint32_t cond     : 4;   /* [31:28] Condition code          */
} arm_instr_sdt_t;

/* ══════════════════════════════════════════════════════════
   FORMAT 8 — Halfword and Signed Data Transfer
   [27:25]=000 [24]=P [23]=U [22]=I [21]=W [20]=L
   [19:16]=Rn [15:12]=Rd [11:8]=offset_hi or 0
   [7]=1 [6:5]=sh [4]=1 [3:0]=Rm or offset_lo
   ══════════════════════════════════════════════════════════ */
typedef struct arm_instr_hwdt {
    arm_uint32_t Rm_lo    : 4;   /* [3:0]   Rm or offset lo nibble  */
    arm_uint32_t one_a    : 1;   /* [4]     Must be 1               */
    arm_uint32_t sh       : 2;   /* [6:5]   Signed/half type        */
    arm_uint32_t one_b    : 1;   /* [7]     Must be 1               */
    arm_uint32_t off_hi   : 4;   /* [11:8]  Offset high nibble      */
    arm_uint32_t Rd       : 4;   /* [15:12] Destination register    */
    arm_uint32_t Rn       : 4;   /* [19:16] Base register           */
    arm_uint32_t L        : 1;   /* [20]    Load/Store flag         */
    arm_uint32_t W        : 1;   /* [21]    Writeback               */
    arm_uint32_t I        : 1;   /* [22]    Immediate offset flag   */
    arm_uint32_t U        : 1;   /* [23]    Up/Down                 */
    arm_uint32_t P        : 1;   /* [24]    Pre/Post index          */
    arm_uint32_t cls      : 3;   /* [27:25] Must be 000             */
    arm_uint32_t cond     : 4;   /* [31:28] Condition code          */
} arm_instr_hwdt_t;

/* ══════════════════════════════════════════════════════════
   FORMAT 9 — Block Data Transfer (LDM/STM)
   [27:25]=100 [24]=P [23]=U [22]=S [21]=W [20]=L
   [19:16]=Rn [15:0]=register_list
   ══════════════════════════════════════════════════════════ */
typedef struct arm_instr_bdt {
    arm_uint32_t reg_list : 16;  /* [15:0]  Bitmask of registers    */
    arm_uint32_t Rn       : 4;   /* [19:16] Base register           */
    arm_uint32_t L        : 1;   /* [20]    Load (1) or Store (0)   */
    arm_uint32_t W        : 1;   /* [21]    Writeback               */
    arm_uint32_t S        : 1;   /* [22]    PSR/user bit            */
    arm_uint32_t U        : 1;   /* [23]    Up (1) or Down (0)      */
    arm_uint32_t P        : 1;   /* [24]    Pre (1) or Post (0)     */
    arm_uint32_t cls      : 3;   /* [27:25] Must be 100             */
    arm_uint32_t cond     : 4;   /* [31:28] Condition code          */
} arm_instr_bdt_t;

/* ══════════════════════════════════════════════════════════
   FORMAT 10 — PSR Transfer (MRS/MSR)
   MRS: [27:23]=00010 [22]=P [21:20]=00 [19:16]=1111
        [15:12]=Rd [11:0]=000000000000
   MSR: [27:23]=00010 [22]=P [21:20]=10 [19:16]=mask
        [15:12]=1111 [11:0]=Op (imm or reg)
   ══════════════════════════════════════════════════════════ */
typedef struct arm_instr_psr {
    arm_uint32_t op       : 12;  /* [11:0]  Operand (0 for MRS)     */
    arm_uint32_t Rd       : 4;   /* [15:12] Dest reg (MRS) / 1111   */
    arm_uint32_t field    : 4;   /* [19:16] Field mask (MSR) / 1111 */
    arm_uint32_t op2      : 2;   /* [21:20] 00=MRS, 10=MSR          */
    arm_uint32_t P        : 1;   /* [22]    0=CPSR, 1=SPSR          */
    arm_uint32_t cls      : 5;   /* [27:23] Must be 00010           */
    arm_uint32_t cond     : 4;   /* [31:28] Condition code          */
} arm_instr_psr_t;

/* ══════════════════════════════════════════════════════════
   FORMAT 11 — Software Interrupt (SWI)
   [27:24]=1111 [23:0]=comment field (OS-defined)
   ══════════════════════════════════════════════════════════ */
typedef struct arm_instr_swi {
    arm_uint32_t comment  : 24;  /* [23:0]  OS-defined SWI number   */
    arm_uint32_t cls      : 4;   /* [27:24] Must be 1111            */
    arm_uint32_t cond     : 4;   /* [31:28] Condition code          */
} arm_instr_swi_t;

/* ══════════════════════════════════════════════════════════
   FORMAT 12 — Coprocessor Data Processing (CDP)
   [27:24]=1110 [23:20]=CP_opc [19:16]=CRn [15:12]=CRd
   [11:8]=cp_num [7:5]=CP [4]=0 [3:0]=CRm
   ══════════════════════════════════════════════════════════ */
typedef struct arm_instr_cdp {
    arm_uint32_t CRm      : 4;   /* [3:0]   Coprocessor operand 2   */
    arm_uint32_t zero     : 1;   /* [4]     Must be 0               */
    arm_uint32_t CP       : 3;   /* [7:5]   Coprocessor info        */
    arm_uint32_t cp_num   : 4;   /* [11:8]  Coprocessor number      */
    arm_uint32_t CRd      : 4;   /* [15:12] Coprocessor dest reg    */
    arm_uint32_t CRn      : 4;   /* [19:16] Coprocessor operand 1   */
    arm_uint32_t CP_opc   : 4;   /* [23:20] Coprocessor opcode      */
    arm_uint32_t cls      : 4;   /* [27:24] Must be 1110            */
    arm_uint32_t cond     : 4;   /* [31:28] Condition code          */
} arm_instr_cdp_t;

/* ══════════════════════════════════════════════════════════
   FORMAT 13 — Coprocessor Data Transfer (LDC/STC)
   [27:25]=110 [24]=P [23]=U [22]=N [21]=W [20]=L
   [19:16]=Rn [15:12]=CRd [11:8]=cp_num [7:0]=offset
   ══════════════════════════════════════════════════════════ */
typedef struct arm_instr_cdt {
    arm_uint32_t offset   : 8;   /* [7:0]   Offset                  */
    arm_uint32_t cp_num   : 4;   /* [11:8]  Coprocessor number      */
    arm_uint32_t CRd      : 4;   /* [15:12] Coprocessor register    */
    arm_uint32_t Rn       : 4;   /* [19:16] Base register           */
    arm_uint32_t L        : 1;   /* [20]    Load/Store              */
    arm_uint32_t W        : 1;   /* [21]    Writeback               */
    arm_uint32_t N        : 1;   /* [22]    Long/short transfer     */
    arm_uint32_t U        : 1;   /* [23]    Up/Down                 */
    arm_uint32_t P        : 1;   /* [24]    Pre/Post                */
    arm_uint32_t cls      : 3;   /* [27:25] Must be 110             */
    arm_uint32_t cond     : 4;   /* [31:28] Condition code          */
} arm_instr_cdt_t;

/* ══════════════════════════════════════════════════════════
   FORMAT 14 — Coprocessor Register Transfer (MCR/MRC)
   [27:24]=1110 [23:21]=CP_opc [20]=L [19:16]=CRn
   [15:12]=Rd [11:8]=cp_num [7:5]=CP [4]=1 [3:0]=CRm
   ══════════════════════════════════════════════════════════ */
typedef struct arm_instr_crt {
    arm_uint32_t CRm      : 4;   /* [3:0]   Coprocessor operand     */
    arm_uint32_t one      : 1;   /* [4]     Must be 1               */
    arm_uint32_t CP       : 3;   /* [7:5]   Coprocessor info        */
    arm_uint32_t cp_num   : 4;   /* [11:8]  Coprocessor number      */
    arm_uint32_t Rd       : 4;   /* [15:12] ARM register            */
    arm_uint32_t CRn      : 4;   /* [19:16] Coprocessor register    */
    arm_uint32_t L        : 1;   /* [20]    MRC(1) / MCR(0)         */
    arm_uint32_t CP_opc   : 3;   /* [23:21] Coprocessor opcode      */
    arm_uint32_t cls      : 4;   /* [27:24] Must be 1110            */
    arm_uint32_t cond     : 4;   /* [31:28] Condition code          */
} arm_instr_crt_t;

/* ══════════════════════════════════════════════════════════
   FORMAT 15 — Undefined Instruction
   [27:25]=011 [24:20]=xxxxx [4]=1
   ══════════════════════════════════════════════════════════ */
typedef struct arm_instr_undef {
    arm_uint32_t pad      : 28;  /* [27:0]  Undefined pattern       */
    arm_uint32_t cond     : 4;   /* [31:28] Condition code          */
} arm_instr_undef_t;

/* ══════════════════════════════════════════════════════════
   UNION — Generic instruction word
   Allows access to any format from a single 32-bit word
   ══════════════════════════════════════════════════════════ */
typedef union arm_instr {
    arm_instr_t        raw;      /* Raw 32-bit instruction word      */
    arm_instr_dp_t     dp;       /* Data processing                  */
    arm_instr_mul_t    mul;      /* Multiply                         */
    arm_instr_mull_t   mull;     /* Multiply long                    */
    arm_instr_swp_t    swp;      /* Single data swap                 */
    arm_instr_branch_t branch;   /* Branch / branch-link             */
    arm_instr_bx_t     bx;       /* Branch and exchange              */
    arm_instr_sdt_t    sdt;      /* Single data transfer             */
    arm_instr_hwdt_t   hwdt;     /* Halfword data transfer           */
    arm_instr_bdt_t    bdt;      /* Block data transfer              */
    arm_instr_psr_t    psr;      /* PSR transfer                     */
    arm_instr_swi_t    swi;      /* Software interrupt               */
    arm_instr_cdp_t    cdp;      /* Coprocessor data processing      */
    arm_instr_cdt_t    cdt;      /* Coprocessor data transfer        */
    arm_instr_crt_t    crt;      /* Coprocessor register transfer    */
    arm_instr_undef_t  undef;    /* Undefined                        */
} arm_instr_u;

#endif /* ARM_INSTR_FORMAT_H */
