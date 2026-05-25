/*
 * arm64_execute.c — AArch64 instruction executor
 * Evaluates condition, dispatches to format handler.
 */
#include "../include/arm64_arch.h"
#include <stddef.h>

/* ── Condition evaluator ─────────────────────────────────── */
static arm64_bool_t arm64_eval_cond(arm64_word_t pstate,
                                    arm64_uint32_t cond)
{
    arm64_bool_t N = ARM64_PSR_GET_N(pstate);
    arm64_bool_t Z = ARM64_PSR_GET_Z(pstate);
    arm64_bool_t C = ARM64_PSR_GET_C(pstate);
    arm64_bool_t V = ARM64_PSR_GET_V(pstate);

    switch (cond & 0xE) {            /* ignore LSB for negation */
        case 0x0: return  Z;                        /* EQ / NE */
        case 0x2: return  C;                        /* CS / CC */
        case 0x4: return  N;                        /* MI / PL */
        case 0x6: return  V;                        /* VS / VC */
        case 0x8: return  (C && !Z);                /* HI / LS */
        case 0xA: return  (N == V);                 /* GE / LT */
        case 0xC: return  (!Z && (N == V));         /* GT / LE */
        case 0xE: return  ARM64_TRUE;               /* AL / NV */
        default:  return  ARM64_FALSE;
    }
    /* If LSB of cond is 1 and cond != AL, invert result */
    /* (handled by caller wrapping this return value)     */
}

/* ── Flag update helpers ─────────────────────────────────── */
static void arm64_update_flags_add(arm64_cpu_t *cpu,
                                   arm64_word_t a,
                                   arm64_word_t b,
                                   arm64_word_t result)
{
    //arm64_word_t *pstate = &cpu->psr.pstate;
    arm64_uint32_t *pstate = &cpu->psr.pstate;

    /* N — bit 63 of result */
    if (result >> 63)   *pstate |=  ARM64_PSR_N;
    else                *pstate &= ~ARM64_PSR_N;

    /* Z — result is zero */
    if (result == 0)    *pstate |=  ARM64_PSR_Z;
    else                *pstate &= ~ARM64_PSR_Z;

    /* C — unsigned overflow: result < a */
    if (result < a)     *pstate |=  ARM64_PSR_C;
    else                *pstate &= ~ARM64_PSR_C;

    /* V — signed overflow */
    if (~(a ^ b) & (a ^ result) & 0x8000000000000000ULL)
                        *pstate |=  ARM64_PSR_V;
    else                *pstate &= ~ARM64_PSR_V;
}

static void arm64_update_flags_sub(arm64_cpu_t *cpu,
                                   arm64_word_t a,
                                   arm64_word_t b,
                                   arm64_word_t result)
{
    /* SUB flags: C is set when there is NO borrow (a >= b) */
    arm64_update_flags_add(cpu, a, ~b, result);
}

/* ── Data processing — register executor ────────────────── */
static void arm64_exec_dp_reg(arm64_cpu_t *cpu, arm64_instr_t instr)
{
    arm64_instr_dp_reg_t *dp = (arm64_instr_dp_reg_t *)&instr;

    arm64_word_t Rn_val = (dp->Rn == ARM64_REG_XZR) ? 0
                          : cpu->regs.x[dp->Rn];
    arm64_word_t Rm_val = (dp->Rm == ARM64_REG_XZR) ? 0
                          : cpu->regs.x[dp->Rm];

    /* Apply shift to Rm */
    arm64_uint32_t sh = dp->imm6 & 0x3F;
    switch (dp->shift) {
        case ARM64_SHIFT_LSL: Rm_val <<= sh; break;
        case ARM64_SHIFT_LSR: Rm_val >>= sh; break;
        case ARM64_SHIFT_ASR:
            Rm_val = (arm64_word_t)((arm64_int64_t)Rm_val >> sh); break;
        case ARM64_SHIFT_ROR:
            if (sh) Rm_val = (Rm_val >> sh) | (Rm_val << (64 - sh)); break;
    }

    arm64_word_t result = 0;
    /* opc encodes the ALU operation (see arm64_opcodes.h) */
    switch (dp->opc) {
        case 0x0: result = Rn_val  &  Rm_val; break;  /* AND / BIC  */
        case 0x1: result = Rn_val  |  Rm_val; break;  /* ORR / ORN  */
        case 0x2: result = Rn_val  ^  Rm_val; break;  /* EOR / EON  */
        case 0x3: result = Rn_val  &  Rm_val; break;  /* ANDS/BICS  */
        default:  result = 0;                  break;
    }

    /* Mask to 32-bit if sf=0 */
    if (!dp->sf) result &= 0xFFFFFFFFULL;

    if (dp->Rd != ARM64_REG_XZR)
        cpu->regs.x[dp->Rd] = result;
}

/* ── Add/subtract immediate executor ─────────────────────── */
static void arm64_exec_addsub_imm(arm64_cpu_t *cpu, arm64_instr_t instr)
{
    arm64_instr_addsub_imm_t *as = (arm64_instr_addsub_imm_t *)&instr;

    arm64_word_t Rn_val = (as->Rn == ARM64_REG_SP)
                          ? cpu->regs.sp
                          : ((as->Rn == ARM64_REG_XZR) ? 0
                             : cpu->regs.x[as->Rn]);

    arm64_word_t imm = as->imm12;
    if (as->shift) imm <<= 12;

    arm64_word_t result = as->op ? (Rn_val - imm) : (Rn_val + imm);
    if (!as->sf) result &= 0xFFFFFFFFULL;

    if (as->S) {
        if (as->op) arm64_update_flags_sub(cpu, Rn_val, imm, result);
        else        arm64_update_flags_add(cpu, Rn_val, imm, result);
    }

    /* Write result — Rd=31 means SP when S=0, XZR when S=1 */
    if (as->S) {
        if (as->Rd != ARM64_REG_XZR)
            cpu->regs.x[as->Rd] = result;
    } else {
        if (as->Rd == ARM64_REG_SP)
            cpu->regs.sp = result;
        else
            cpu->regs.x[as->Rd] = result;
    }
}

/* ── Load/store unsigned offset executor ─────────────────── */
static void arm64_exec_ldst_uimm(arm64_cpu_t *cpu, arm64_instr_t instr)
{
    arm64_instr_ldst_uimm_t *ls = (arm64_instr_ldst_uimm_t *)&instr;

    /* Scale: imm12 is scaled by the access size */
    arm64_uint32_t scale = ls->size;
    arm64_word_t   offset = (arm64_word_t)ls->imm12 << scale;

    arm64_word_t Xn = (ls->Rn == ARM64_REG_SP)
                      ? cpu->regs.sp : cpu->regs.x[ls->Rn];
    arm64_addr_t addr = Xn + offset;

    if (ls->opc == 0) {
        /* STR */
        arm64_word_t val = (ls->Rt == ARM64_REG_XZR)
                           ? 0 : cpu->regs.x[ls->Rt];
        switch (ls->size) {
            case 0: arm64_mem_write_byte (addr, val); break;
            case 1: arm64_mem_write_word (addr, val); break;
            case 2: arm64_mem_write_dword(addr, val); break;
            case 3: arm64_mem_write_qword(addr, val); break;
        }
    } else {
        /* LDR */
        arm64_word_t val = 0;
        switch (ls->size) {
            case 0: val = arm64_mem_read_byte (addr); break;
            case 1: val = arm64_mem_read_word (addr); break;
            case 2: val = arm64_mem_read_dword(addr); break;
            case 3: val = arm64_mem_read_qword(addr); break;
        }
        if (ls->Rt != ARM64_REG_XZR)
            cpu->regs.x[ls->Rt] = val;
    }
}

/* ── Branch immediate executor ───────────────────────────── */
static void arm64_exec_branch_imm(arm64_cpu_t *cpu, arm64_instr_t instr)
{
    arm64_instr_branch_imm_t *br = (arm64_instr_branch_imm_t *)&instr;
    arm64_int64_t offset = (arm64_int64_t)((arm64_int32_t)(br->imm26 << 6) >> 4);

    if (br->op) /* BL */
        cpu->regs.x[ARM64_REG_X30] = cpu->pc + 4;

    cpu->pc = (arm64_addr_t)((arm64_int64_t)cpu->pc + offset);
}

/* ── Conditional branch executor ─────────────────────────── */
static void arm64_exec_branch_cond(arm64_cpu_t *cpu, arm64_instr_t instr)
{
    arm64_instr_branch_cond_t *bc = (arm64_instr_branch_cond_t *)&instr;
    arm64_bool_t taken = arm64_eval_cond(cpu->psr.pstate, bc->cond);

    /* Invert for odd condition codes (NE, CC, PL …) */
    if ((bc->cond & 0x1) && bc->cond != ARM64_COND_AL)
        taken = !taken;

    if (taken) {
        arm64_int64_t offset =
            (arm64_int64_t)((arm64_int32_t)(bc->imm19 << 13) >> 11);
        cpu->pc = (arm64_addr_t)((arm64_int64_t)cpu->pc + offset);
    } else {
        cpu->pc += 4;
    }
}

/* ── Branch to register executor ─────────────────────────── */
static void arm64_exec_branch_reg(arm64_cpu_t *cpu, arm64_instr_t instr)
{
    arm64_instr_branch_reg_t *br = (arm64_instr_branch_reg_t *)&instr;
    arm64_word_t target = cpu->regs.x[br->Rn];

    switch (br->opc) {
        case 0x0: cpu->pc = target;                              break; /* BR  */
        case 0x1: cpu->regs.x[ARM64_REG_X30] = cpu->pc + 4;
                  cpu->pc = target;                              break; /* BLR */
        case 0x2: cpu->pc = cpu->regs.x[ARM64_REG_X30];         break; /* RET */
        default:                                                  break;
    }
}

/*
 * arm64_cpu_step() — fetch, decode and execute one instruction
 */
int arm64_cpu_step(arm64_cpu_t *cpu)
{
    if (cpu->halted) return -1;

    arm64_instr_t instr = (arm64_instr_t)arm64_mem_read_dword(cpu->pc);
    arm64_uint32_t op   = (instr >> 25) & 0xF;

    switch (op) {
        case 0x8: case 0x9:            /* DP immediate          */
        {
            arm64_uint32_t op23 = (instr >> 23) & 0x7;
            if (op23 == 0x2 || op23 == 0x3)
                arm64_exec_addsub_imm(cpu, instr);
            cpu->pc += 4;
            break;
        }
        case 0xA: case 0xB:            /* Branch                */
        {
            arm64_uint32_t op29 = (instr >> 29) & 0x7;
            if      (op29 == 0x0 || op29 == 0x4)
                arm64_exec_branch_imm(cpu, instr);
            else if (op29 == 0x2 || op29 == 0x6)
                arm64_exec_branch_cond(cpu, instr);
            else
                cpu->pc += 4;
            break;
        }
        case 0x4: case 0x6:
        case 0xC: case 0xE:            /* Load/store            */
            arm64_exec_ldst_uimm(cpu, instr);
            cpu->pc += 4;
            break;
        case 0x5: case 0xD:            /* DP register           */
            arm64_exec_dp_reg(cpu, instr);
            cpu->pc += 4;
            break;
        default:
            cpu->pc += 4;
            break;
    }

    return 0;
}

/*
 * arm64_cpu_reset() — initialise CPU state to power-on defaults
 */
void arm64_cpu_reset(arm64_cpu_t *cpu)
{
    arm64_uint32_t i;
    if (!cpu) return;

    for (i = 0; i < 31; i++) cpu->regs.x[i] = 0;
    cpu->regs.sp   = ARM64_MEM_STACK_TOP;
    cpu->regs.pc   = ARM64_MEM_ROM_BASE;
    cpu->pc        = ARM64_MEM_ROM_BASE;
    cpu->psr.pstate= ARM64_PSR_EL1h | ARM64_PSR_D |
                     ARM64_PSR_A    | ARM64_PSR_I  | ARM64_PSR_F;
    cpu->el        = ARM64_EL1;
    cpu->halted    = ARM64_FALSE;
}

/*
 * arm64_cpu_run() — run until halted
 */
void arm64_cpu_run(arm64_cpu_t *cpu)
{
    while (!cpu->halted)
        arm64_cpu_step(cpu);
}
