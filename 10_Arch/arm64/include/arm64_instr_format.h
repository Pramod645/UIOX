#ifndef ARM64_INSTR_FORMAT_H
#define ARM64_INSTR_FORMAT_H
/*
 * arm64_instr_format.h — AArch64 instruction encoding structs
 * Reference: ARM DDI 0487, Chapter C4
 *
 * All A64 instructions are fixed 32-bit little-endian words.
 * Bit-fields are laid out LSB-first as seen in the ISA.
 */
#include "arm64_types.h"

/* ══════════════════════════════════════════════════════════
   FORMAT 1 — Data Processing (Register, shifted)
   [31]=sf [30:29]=opc [28:24]=01011 [23:22]=shift
   [21]=N  [20:16]=Rm  [15:10]=imm6  [9:5]=Rn  [4:0]=Rd
   ══════════════════════════════════════════════════════════ */
typedef struct arm64_instr_dp_reg {
    arm64_uint32_t Rd       : 5;   /* [4:0]   Destination register      */
    arm64_uint32_t Rn       : 5;   /* [9:5]   First source register      */
    arm64_uint32_t imm6     : 6;   /* [15:10] Shift amount               */
    arm64_uint32_t Rm       : 5;   /* [20:16] Second source register     */
    arm64_uint32_t N        : 1;   /* [21]    Invert bit (BIC/ORN/EON)   */
    arm64_uint32_t shift    : 2;   /* [23:22] LSL/LSR/ASR/ROR            */
    arm64_uint32_t fixed    : 5;   /* [28:24] Must be 01011              */
    arm64_uint32_t opc      : 2;   /* [30:29] Operation code             */
    arm64_uint32_t sf       : 1;   /* [31]    0=32-bit, 1=64-bit         */
} arm64_instr_dp_reg_t;

/* ══════════════════════════════════════════════════════════
   FORMAT 2 — Data Processing Immediate (ADD/SUB imm12)
   [31]=sf [30:29]=opc [28:23]=100010 [22]=sh
   [21:10]=imm12  [9:5]=Rn  [4:0]=Rd
   ══════════════════════════════════════════════════════════ */
typedef struct arm64_instr_addsub_imm {
    arm64_uint32_t Rd       : 5;   /* [4:0]   Destination register      */
    arm64_uint32_t Rn       : 5;   /* [9:5]   Source register            */
    arm64_uint32_t imm12    : 12;  /* [21:10] Unsigned immediate         */
    arm64_uint32_t shift    : 1;   /* [22]    0=no shift, 1=LSL #12      */
    arm64_uint32_t fixed    : 6;   /* [28:23] Must be 100010             */
    arm64_uint32_t S        : 1;   /* [29]    Set flags                  */
    arm64_uint32_t op       : 1;   /* [30]    0=ADD, 1=SUB               */
    arm64_uint32_t sf       : 1;   /* [31]    0=32-bit, 1=64-bit         */
} arm64_instr_addsub_imm_t;

/* ══════════════════════════════════════════════════════════
   FORMAT 3 — Logical Immediate (AND/ORR/EOR/ANDS)
   [31]=sf [30:29]=opc [28:23]=100100
   [22]=N  [21:16]=immr  [15:10]=imms  [9:5]=Rn  [4:0]=Rd
   ══════════════════════════════════════════════════════════ */
typedef struct arm64_instr_logical_imm {
    arm64_uint32_t Rd       : 5;   /* [4:0]   Destination register      */
    arm64_uint32_t Rn       : 5;   /* [9:5]   Source register            */
    arm64_uint32_t imms     : 6;   /* [15:10] Element size / ones count  */
    arm64_uint32_t immr     : 6;   /* [21:16] Rotation amount            */
    arm64_uint32_t N        : 1;   /* [22]    Width qualifier            */
    arm64_uint32_t fixed    : 6;   /* [28:23] Must be 100100             */
    arm64_uint32_t opc      : 2;   /* [30:29] 00=AND, 01=ORR, 10=EOR,   */
                                   /*         11=ANDS                     */
    arm64_uint32_t sf       : 1;   /* [31]    0=32-bit, 1=64-bit         */
} arm64_instr_logical_imm_t;

/* ══════════════════════════════════════════════════════════
   FORMAT 4 — Move Wide Immediate (MOVZ/MOVN/MOVK)
   [31]=sf [30:29]=opc [28:23]=100101
   [22:21]=hw  [20:5]=imm16  [4:0]=Rd
   ══════════════════════════════════════════════════════════ */
typedef struct arm64_instr_movwide {
    arm64_uint32_t Rd       : 5;   /* [4:0]   Destination register      */
    arm64_uint32_t imm16    : 16;  /* [20:5]  16-bit immediate           */
    arm64_uint32_t hw       : 2;   /* [22:21] Shift: 0/16/32/48 bits     */
    arm64_uint32_t fixed    : 6;   /* [28:23] Must be 100101             */
    arm64_uint32_t opc      : 2;   /* [30:29] 00=MOVN, 10=MOVZ, 11=MOVK */
    arm64_uint32_t sf       : 1;   /* [31]    0=32-bit, 1=64-bit         */
} arm64_instr_movwide_t;

/* ══════════════════════════════════════════════════════════
   FORMAT 5 — Bitfield (SBFM / BFM / UBFM)
   [31]=sf [30:29]=opc [28:23]=100110
   [22]=N  [21:16]=immr  [15:10]=imms  [9:5]=Rn  [4:0]=Rd
   ══════════════════════════════════════════════════════════ */
typedef struct arm64_instr_bitfield {
    arm64_uint32_t Rd       : 5;   /* [4:0]   Destination register      */
    arm64_uint32_t Rn       : 5;   /* [9:5]   Source register            */
    arm64_uint32_t imms     : 6;   /* [15:10] Bit position (lsb)         */
    arm64_uint32_t immr     : 6;   /* [21:16] Rotate amount              */
    arm64_uint32_t N        : 1;   /* [22]    Must equal sf              */
    arm64_uint32_t fixed    : 6;   /* [28:23] Must be 100110             */
    arm64_uint32_t opc      : 2;   /* [30:29] 00=SBFM,01=BFM,10=UBFM    */
    arm64_uint32_t sf       : 1;   /* [31]    0=32-bit, 1=64-bit         */
} arm64_instr_bitfield_t;

/* ══════════════════════════════════════════════════════════
   FORMAT 6 — Load/Store (Unsigned offset)
   [31:30]=size [29:27]=111 [26]=V [25:24]=01
   [23:22]=opc  [21:10]=imm12  [9:5]=Rn  [4:0]=Rt
   ══════════════════════════════════════════════════════════ */
typedef struct arm64_instr_ldst_uimm {
    arm64_uint32_t Rt       : 5;   /* [4:0]   Target / source register  */
    arm64_uint32_t Rn       : 5;   /* [9:5]   Base address register      */
    arm64_uint32_t imm12    : 12;  /* [21:10] Unsigned offset (scaled)   */
    arm64_uint32_t opc      : 2;   /* [23:22] 00=STR, 01=LDR, 10=LDRS   */
    arm64_uint32_t fixed    : 2;   /* [25:24] Must be 01                 */
    arm64_uint32_t V        : 1;   /* [26]    0=GP reg, 1=SIMD/FP reg   */
    arm64_uint32_t fixed2   : 3;   /* [29:27] Must be 111                */
    arm64_uint32_t size     : 2;   /* [31:30] 00=B, 01=H, 10=W, 11=X    */
} arm64_instr_ldst_uimm_t;

/* ══════════════════════════════════════════════════════════
   FORMAT 7 — Load/Store (Register offset)
   [31:30]=size [29:27]=111 [26]=V [25:24]=00
   [23:22]=opc  [21]=1  [20:16]=Rm  [15:13]=option
   [12]=S  [11:10]=10  [9:5]=Rn  [4:0]=Rt
   ══════════════════════════════════════════════════════════ */
typedef struct arm64_instr_ldst_reg {
    arm64_uint32_t Rt       : 5;   /* [4:0]   Target / source register  */
    arm64_uint32_t Rn       : 5;   /* [9:5]   Base address register      */
    arm64_uint32_t fixed3   : 2;   /* [11:10] Must be 10                 */
    arm64_uint32_t S        : 1;   /* [12]    Scale shift enable         */
    arm64_uint32_t option   : 3;   /* [15:13] Extend type (LSL/UXTX…)   */
    arm64_uint32_t Rm       : 5;   /* [20:16] Offset register            */
    arm64_uint32_t fixed2   : 1;   /* [21]    Must be 1                  */
    arm64_uint32_t opc      : 2;   /* [23:22] 00=STR, 01=LDR             */
    arm64_uint32_t fixed    : 2;   /* [25:24] Must be 00                 */
    arm64_uint32_t V        : 1;   /* [26]    0=GP reg, 1=SIMD/FP        */
    arm64_uint32_t fixed4   : 3;   /* [29:27] Must be 111                */
    arm64_uint32_t size     : 2;   /* [31:30] 00=B, 01=H, 10=W, 11=X    */
} arm64_instr_ldst_reg_t;

/* ══════════════════════════════════════════════════════════
   FORMAT 8 — Load/Store Pair (STP / LDP)
   [31:30]=opc [29:27]=101 [26]=V [25:24]=opc2
   [23:22]=L/W [21:15]=imm7 [14:10]=Rt2 [9:5]=Rn [4:0]=Rt
   ══════════════════════════════════════════════════════════ */
typedef struct arm64_instr_ldst_pair {
    arm64_uint32_t Rt       : 5;   /* [4:0]   First target register     */
    arm64_uint32_t Rn       : 5;   /* [9:5]   Base address register      */
    arm64_uint32_t Rt2      : 5;   /* [14:10] Second target register     */
    arm64_uint32_t imm7     : 7;   /* [21:15] Signed scaled offset       */
    arm64_uint32_t L        : 1;   /* [22]    0=STP, 1=LDP               */
    arm64_uint32_t opc2     : 2;   /* [24:23] Pre/post/signed offset     */
    arm64_uint32_t V        : 1;   /* [25]    0=GP reg, 1=SIMD/FP        */
    arm64_uint32_t fixed    : 3;   /* [28:26] Must be 101                */
    arm64_uint32_t opc      : 2;   /* [30:29] Operand size               */
    arm64_uint32_t sf       : 1;   /* [31]    0=32-bit pairs, 1=64-bit   */
} arm64_instr_ldst_pair_t;

/* ══════════════════════════════════════════════════════════
   FORMAT 9 — Unconditional Branch (immediate)
   [31]=op [30:26]=00101  [25:0]=imm26
   ══════════════════════════════════════════════════════════ */
typedef struct arm64_instr_branch_imm {
    arm64_int32_t  imm26    : 26;  /* [25:0]  Signed PC-relative offset  */
    arm64_uint32_t fixed    : 5;   /* [30:26] Must be 00101              */
    arm64_uint32_t op       : 1;   /* [31]    0=B, 1=BL                  */
} arm64_instr_branch_imm_t;

/* ══════════════════════════════════════════════════════════
   FORMAT 10 — Conditional Branch (B.cond)
   [31:25]=0101010 [24]=0 [23:5]=imm19 [4]=0 [3:0]=cond
   ══════════════════════════════════════════════════════════ */
typedef struct arm64_instr_branch_cond {
    arm64_uint32_t cond     : 4;   /* [3:0]   Condition code             */
    arm64_uint32_t o0       : 1;   /* [4]     Must be 0                  */
    arm64_int32_t  imm19    : 19;  /* [23:5]  Signed PC-relative offset  */
    arm64_uint32_t o1       : 1;   /* [24]    Must be 0                  */
    arm64_uint32_t fixed    : 7;   /* [31:25] Must be 0101010            */
} arm64_instr_branch_cond_t;

/* ══════════════════════════════════════════════════════════
   FORMAT 11 — Compare and Branch (CBZ / CBNZ)
   [31]=sf [30:25]=011010 [24]=op [23:5]=imm19 [4:0]=Rt
   ══════════════════════════════════════════════════════════ */
typedef struct arm64_instr_cbz {
    arm64_uint32_t Rt       : 5;   /* [4:0]   Register to test          */
    arm64_int32_t  imm19    : 19;  /* [23:5]  Signed PC-relative offset  */
    arm64_uint32_t op       : 1;   /* [24]    0=CBZ, 1=CBNZ              */
    arm64_uint32_t fixed    : 6;   /* [30:25] Must be 011010             */
    arm64_uint32_t sf       : 1;   /* [31]    0=32-bit, 1=64-bit         */
} arm64_instr_cbz_t;

/* ══════════════════════════════════════════════════════════
   FORMAT 12 — Test and Branch (TBZ / TBNZ)
   [31:24]=01101101/01101111 [23:19]=b40 [18:5]=imm14 [4:0]=Rt
   ══════════════════════════════════════════════════════════ */
typedef struct arm64_instr_tbz {
    arm64_uint32_t Rt       : 5;   /* [4:0]   Register to test          */
    arm64_int32_t  imm14    : 14;  /* [18:5]  Signed PC-relative offset  */
    arm64_uint32_t b40      : 5;   /* [23:19] Bit position [4:0]         */
    arm64_uint32_t op       : 1;   /* [24]    0=TBZ, 1=TBNZ              */
    arm64_uint32_t fixed    : 6;   /* [30:25] Must be 011011             */
    arm64_uint32_t b5       : 1;   /* [31]    Bit position [5]           */
} arm64_instr_tbz_t;

/* ══════════════════════════════════════════════════════════
   FORMAT 13 — Branch to Register (BR / BLR / RET)
   [31:25]=1101011 [24:23]=opc [22:21]=op2 [20:16]=op3
   [15:10]=111111  [9:5]=Rn    [4:0]=Rm
   ══════════════════════════════════════════════════════════ */
typedef struct arm64_instr_branch_reg {
    arm64_uint32_t Rm       : 5;   /* [4:0]   Must be 0b00000            */
    arm64_uint32_t Rn       : 5;   /* [9:5]   Target register            */
    arm64_uint32_t fixed2   : 6;   /* [15:10] Must be 111111             */
    arm64_uint32_t op3      : 5;   /* [20:16] Must be 00000              */
    arm64_uint32_t op2      : 2;   /* [22:21] Must be 11                 */
    arm64_uint32_t opc      : 2;   /* [24:23] 00=BR,01=BLR,10=RET        */
    arm64_uint32_t fixed    : 7;   /* [31:25] Must be 1101011            */
} arm64_instr_branch_reg_t;

/* ══════════════════════════════════════════════════════════
   FORMAT 14 — Exception Generation (SVC / HVC / SMC / BRK)
   [31:24]=11010100 [23:21]=opc [20:5]=imm16 [4:2]=op2 [1:0]=LL
   ══════════════════════════════════════════════════════════ */
typedef struct arm64_instr_exception {
    arm64_uint32_t LL       : 2;   /* [1:0]   Level (01=EL1, 10=EL2)    */
    arm64_uint32_t op2      : 3;   /* [4:2]   Must be 000                */
    arm64_uint32_t imm16    : 16;  /* [20:5]  Immediate (comment field)  */
    arm64_uint32_t opc      : 3;   /* [23:21] 000=SVC,001=HVC,010=SMC    */
                                   /*         001=BRK,010=HLT            */
    arm64_uint32_t fixed    : 8;   /* [31:24] Must be 11010100           */
} arm64_instr_exception_t;

/* ══════════════════════════════════════════════════════════
   FORMAT 15 — System Register (MRS / MSR)
   [31:20]=1101010100 [19]=L [18:5]=sysreg [4:0]=Rt
   ══════════════════════════════════════════════════════════ */
typedef struct arm64_instr_sysreg {
    arm64_uint32_t Rt       : 5;   /* [4:0]   GP target register        */
    arm64_uint32_t op2      : 3;   /* [7:5]   System register op2        */
    arm64_uint32_t CRm      : 4;   /* [11:8]  CRm field                  */
    arm64_uint32_t CRn      : 4;   /* [15:12] CRn field                  */
    arm64_uint32_t op1      : 3;   /* [18:16] op1 field                  */
    arm64_uint32_t op0      : 2;   /* [20:19] op0 field                  */
    arm64_uint32_t L        : 1;   /* [21]    0=MSR (write), 1=MRS (read)*/
    arm64_uint32_t fixed    : 10;  /* [31:22] Must be 1101010100         */
} arm64_instr_sysreg_t;

/* ── Master union of all formats ─────────────────────────── */
typedef union arm64_instr_u {
    arm64_instr_dp_reg_t      dp_reg;
    arm64_instr_addsub_imm_t  addsub_imm;
    arm64_instr_logical_imm_t logical_imm;
    arm64_instr_movwide_t     movwide;
    arm64_instr_bitfield_t    bitfield;
    arm64_instr_ldst_uimm_t   ldst_uimm;
    arm64_instr_ldst_reg_t    ldst_reg;
    arm64_instr_ldst_pair_t   ldst_pair;
    arm64_instr_branch_imm_t  branch_imm;
    arm64_instr_branch_cond_t branch_cond;
    arm64_instr_cbz_t         cbz;
    arm64_instr_tbz_t         tbz;
    arm64_instr_branch_reg_t  branch_reg;
    arm64_instr_exception_t   exception;
    arm64_instr_sysreg_t      sysreg;
    arm64_uint32_t            raw;          /* raw 32-bit word            */
} arm64_instr_u;

#endif /* ARM64_INSTR_FORMAT_H */
