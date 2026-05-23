#ifndef ARM_ARCH_H
#define ARM_ARCH_H

/*
 * arm_arch.h — Master include for UIX ARM architecture layer
 *
 * Include this single header to get all ARM architecture
 * definitions, instruction formats, macros, and interfaces.
 */

#include "arm_types.h"
#include "arm_registers.h"
#include "arm_psr.h"
#include "arm_opcodes.h"
#include "arm_instr_format.h"
#include "arm_macros.h"
#include "arm_memory.h"
#include "arm_exceptions.h"
#include "arm_coprocessor.h"

/* ── CPU state ───────────────────────────────────────────── */
typedef struct arm_cpu {
    arm_regfile_t  regs;       /* General-purpose registers    */
    arm_psr_t      psr;        /* Program status registers     */
    arm_coproc_t   cp[16];     /* Coprocessors CP0-CP15        */
    arm_addr_t     pc;         /* Current PC                   */
    arm_bool_t     thumb;      /* 1 = Thumb state              */
    arm_bool_t     halted;     /* 1 = CPU halted               */
} arm_cpu_t;

/* ── CPU lifecycle ───────────────────────────────────────── */
void arm_cpu_reset  (arm_cpu_t *cpu);
int  arm_cpu_step   (arm_cpu_t *cpu);   /* execute one instruction */
void arm_cpu_run    (arm_cpu_t *cpu);   /* run until halted        */

#endif /* ARM_ARCH_H */
