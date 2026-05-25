/*
 * arm64_decode.c — AArch64 instruction decoder
 * Classifies a 32-bit instruction word into its format type.
 */
#include "../include/arm64_arch.h"

/* ── Instruction class from bits [28:25] ─────────────────── */
typedef enum arm64_instr_class {
    CLS_DP_IMM      = 0x8,   /* 100x — Data processing immediate   */
    CLS_BRANCH      = 0xA,   /* 101x — Branch / exception / system */
    CLS_LDST        = 0xC,   /* x1x0 — Load / store                */
    CLS_DP_REG      = 0xD,   /* x101 — Data processing register    */
    CLS_FP_SIMD     = 0xE,   /* x111 — FP and SIMD                 */
    CLS_UNKNOWN     = 0xFF,
} arm64_instr_class_t;

static arm64_instr_class_t arm64_decode_class(arm64_instr_t instr)
{
    arm64_uint32_t op = (instr >> 25) & 0xF;

    switch (op) {
        case 0x8: case 0x9:              return CLS_DP_IMM;
        case 0xA: case 0xB:              return CLS_BRANCH;
        case 0x4: case 0x6:
        case 0xC: case 0xE:              return CLS_LDST;
        case 0x5: case 0xD:              return CLS_DP_REG;
        case 0x7: case 0xF:              return CLS_FP_SIMD;
        default:                         return CLS_UNKNOWN;
    }
}

/* ── Data-processing immediate sub-class ─────────────────── */
static void arm64_decode_dp_imm(arm64_instr_t instr)
{
    arm64_uint32_t op0 = (instr >> 23) & 0x7;
    (void)op0;
    /*
     * op0:
     *  000/001 — PC-rel addressing (ADR / ADRP)
     *  010     — Add/subtract immediate
     *  011     — Add/subtract immediate with tags
     *  100     — Logical immediate
     *  101     — Move wide immediate
     *  110     — Bitfield
     *  111     — Extract
     */
}

/* ── Branch / exception / system sub-class ───────────────── */
static void arm64_decode_branch(arm64_instr_t instr)
{
    arm64_uint32_t op0 = (instr >> 29) & 0x7;
    arm64_uint32_t op1 = (instr >> 22) & 0x7F;
    (void)op1;

    switch (op0) {
        case 0x0: case 0x4:   /* Unconditional branch (imm) B / BL  */ break;
        case 0x1: case 0x5:   /* Compare and branch CBZ / CBNZ       */ break;
        case 0x2: case 0x6:   /* Test and branch TBZ / TBNZ          */ break;
        case 0x3: case 0x7:   /* Conditional branch B.cond           */ break;
        default:                                                         break;
    }
}

/* ── Load/store sub-class ────────────────────────────────── */
static void arm64_decode_ldst(arm64_instr_t instr)
{
    arm64_uint32_t op0 = (instr >> 28) & 0xF;
    arm64_uint32_t op1 = (instr >> 26) & 0x1;
    arm64_uint32_t op2 = (instr >> 23) & 0x3;
    arm64_uint32_t op3 = (instr >> 16) & 0x3F;
    arm64_uint32_t op4 = (instr >> 10) & 0x3;
    (void)op0; (void)op1; (void)op2; (void)op3; (void)op4;
    /*
     * Covers: LDR/STR (imm, reg, literal),
     *         LDRB/STRB, LDRH/STRH, LDRSB/LDRSH/LDRSW,
     *         LDP/STP, LDNP/STNP, Load/store exclusive
     */
}

/* ── Data-processing register sub-class ─────────────────── */
static void arm64_decode_dp_reg(arm64_instr_t instr)
{
    arm64_uint32_t op0 = (instr >> 30) & 0x1;
    arm64_uint32_t op1 = (instr >> 28) & 0x1;
    arm64_uint32_t op2 = (instr >> 24) & 0xF;
    arm64_uint32_t op3 = (instr >> 10) & 0x3F;
    (void)op0; (void)op1; (void)op2; (void)op3;
    /*
     * Covers: Logical shifted reg, Add/sub shifted/extended,
     *         Add/sub with carry, Rotate right into flags,
     *         Evaluate into flags, Conditional compare,
     *         Conditional select, Data processing 1/2/3 source
     *         (MUL, MADD, MSUB, SDIV, UDIV, SMULH, UMULH …)
     */
}

/*
 * arm64_decode() — top-level decode entry point
 * Returns the instruction class of `instr`.
 */
arm64_instr_class_t arm64_decode(arm64_instr_t instr)
{
    arm64_instr_class_t cls = arm64_decode_class(instr);

    switch (cls) {
        case CLS_DP_IMM:  arm64_decode_dp_imm(instr);  break;
        case CLS_BRANCH:  arm64_decode_branch(instr);  break;
        case CLS_LDST:    arm64_decode_ldst(instr);    break;
        case CLS_DP_REG:  arm64_decode_dp_reg(instr);  break;
        case CLS_FP_SIMD: /* TODO: FP/SIMD decode */   break;
        default:                                        break;
    }

    return cls;
}
