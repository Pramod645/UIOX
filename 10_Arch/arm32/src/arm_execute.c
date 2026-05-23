/*
 * arm_execute.c — ARM instruction executor
 * Evaluates condition, dispatches to format handler.
 */

 #include "../include/arm_arch.h"
 #include <stddef.h>
 
 /* ── Condition evaluator ─────────────────────────────────── */
 static arm_bool_t arm_eval_cond(arm_word_t cpsr, arm_uint32_t cond)
 {
     arm_bool_t N = ARM_PSR_GET_N(cpsr);
     arm_bool_t Z = ARM_PSR_GET_Z(cpsr);
     arm_bool_t C = ARM_PSR_GET_C(cpsr);
     arm_bool_t V = ARM_PSR_GET_V(cpsr);
 
     switch (cond) {
         case ARM_COND_EQ: return  Z;
         case ARM_COND_NE: return !Z;
         case ARM_COND_CS: return  C;
         case ARM_COND_CC: return !C;
         case ARM_COND_MI: return  N;
         case ARM_COND_PL: return !N;
         case ARM_COND_VS: return  V;
         case ARM_COND_VC: return !V;
         case ARM_COND_HI: return  C && !Z;
         case ARM_COND_LS: return !C ||  Z;
         case ARM_COND_GE: return  N == V;
         case ARM_COND_LT: return  N != V;
         case ARM_COND_GT: return !Z && (N == V);
         case ARM_COND_LE: return  Z || (N != V);
         case ARM_COND_AL: return ARM_TRUE;
         default:          return ARM_FALSE;
     }
 }
 
 /* ── Data processing execute ─────────────────────────────── */
 static void arm_exec_dp(arm_cpu_t *cpu, arm_instr_t instr)
 {
     arm_instr_dp_t *dp = ARM_INSTR_AT_AND(&instr);
     arm_word_t Rn  = cpu->regs.r[dp->Rn];
     arm_word_t Op2 = dp->op2;   /* simplified: no shift expand */
     arm_word_t result = 0;
 
     switch (dp->opcode) {
         case ARM_OP_AND: result = Rn &  Op2; break;
         case ARM_OP_EOR: result = Rn ^  Op2; break;
         case ARM_OP_SUB: result = Rn -  Op2; break;
         case ARM_OP_RSB: result = Op2 - Rn;  break;
         case ARM_OP_ADD: result = Rn +  Op2; break;
         case ARM_OP_ADC: result = Rn + Op2 + ARM_PSR_GET_C(cpu->psr.cpsr); break;
         case ARM_OP_SBC: result = Rn - Op2 - (1u - ARM_PSR_GET_C(cpu->psr.cpsr)); break;
         case ARM_OP_RSC: result = Op2 - Rn - (1u - ARM_PSR_GET_C(cpu->psr.cpsr)); break;
         case ARM_OP_ORR: result = Rn | Op2;  break;
         case ARM_OP_MOV: result = Op2;       break;
         case ARM_OP_BIC: result = Rn & ~Op2; break;
         case ARM_OP_MVN: result = ~Op2;      break;
         case ARM_OP_TST: result = Rn & Op2;  break;
         case ARM_OP_TEQ: result = Rn ^ Op2;  break;
         case ARM_OP_CMP: result = Rn - Op2;  break;
         case ARM_OP_CMN: result = Rn + Op2;  break;
         default: break;
     }
 
     /* write result for non-test opcodes */
     if (dp->opcode < ARM_OP_TST || dp->opcode > ARM_OP_CMN)
         cpu->regs.r[dp->Rd] = result;
 
     /* update flags if S bit set */
     if (dp->S) {
         if (result == 0)        ARM_PSR_SET_Z(cpu->psr.cpsr);
         else                    ARM_PSR_CLR_Z(cpu->psr.cpsr);
         if (result >> 31)       ARM_PSR_SET_N(cpu->psr.cpsr);
         else                    ARM_PSR_CLR_N(cpu->psr.cpsr);
     }
 }
 
 /* ── Branch execute ──────────────────────────────────────── */
 static void arm_exec_branch(arm_cpu_t *cpu, arm_instr_t instr)
 {
     arm_instr_branch_t *b = ARM_INSTR_AT_B(&instr);
     arm_addr_t target = ARM_BRANCH_TARGET(cpu->pc, b->offset);
     if (b->L)
         cpu->regs.r[ARM_REG_LR] = cpu->pc + 4u;
     cpu->pc = target;
 }
 
 /* ── BX execute ──────────────────────────────────────────── */
 static void arm_exec_bx(arm_cpu_t *cpu, arm_instr_t instr)
 {
     arm_instr_bx_t *bx = ARM_INSTR_AT_BX(&instr);
     arm_word_t target = cpu->regs.r[bx->Rn];
     cpu->thumb = (arm_bool_t)(target & 1u);
     cpu->pc    = target & 0xFFFFFFFEu;
 }
 
 /* ── SWI execute ─────────────────────────────────────────── */
 static void arm_exec_swi(arm_cpu_t *cpu, arm_instr_t instr)
 {
     (void)instr;
     /* Save PC and CPSR, switch to SVC mode */
     cpu->psr.spsr = cpu->psr.cpsr;
     cpu->regs.r[ARM_REG_LR] = cpu->pc + 4u;
     cpu->psr.cpsr = (cpu->psr.cpsr & ~(arm_word_t)ARM_PSR_MODE)
                   | ARM_MODE_SVC | ARM_PSR_I;
     cpu->pc = ARM_VEC_SWI;
 }
 
 /* ── Main step ───────────────────────────────────────────── */
 int arm_cpu_step(arm_cpu_t *cpu)
 {
     if (cpu->halted) return -1;
 
     /* fetch */
     arm_instr_t instr = arm_mem_read_word(cpu->pc);
 
     /* check condition */
     arm_uint32_t cond = ARM_INSTR_COND(instr);
     if (!arm_eval_cond(cpu->psr.cpsr, cond)) {
         cpu->pc += 4u;
         return 0;
     }
 
     /* decode and execute */
     arm_uint32_t b27_24 = (instr >> 24) & 0xFu;
     arm_uint32_t b27_25 = (instr >> 25) & 0x7u;
 
     if (b27_24 == 0xFu)                          arm_exec_swi(cpu, instr);
     else if ((instr & 0x0FFFFFF0u)==0x012FFF10u) arm_exec_bx(cpu, instr);
     else if (b27_25 == 0x5u)                     arm_exec_branch(cpu, instr);
     else                                          arm_exec_dp(cpu, instr);
 
     /* advance PC if not branched */
     if ((instr & 0x0E000000u) != 0x0A000000u &&
         (instr & 0x0FFFFFF0u) != 0x012FFF10u)
         cpu->pc += 4u;
 
     return 0;
 }
 
 void arm_cpu_reset(arm_cpu_t *cpu)
 {
     int i;
     for (i = 0; i < 16; i++) cpu->regs.r[i] = 0;
     cpu->psr.cpsr  = ARM_MODE_SVC | ARM_PSR_I | ARM_PSR_F;
     cpu->psr.spsr  = 0;
     cpu->pc        = ARM_MEM_RESET_VEC;
     cpu->thumb     = ARM_FALSE;
     cpu->halted    = ARM_FALSE;
 }
 
 void arm_cpu_run(arm_cpu_t *cpu)
 {
     while (!cpu->halted)
         arm_cpu_step(cpu);
 }
 