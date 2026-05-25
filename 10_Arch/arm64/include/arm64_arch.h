#ifndef ARM64_ARCH_H
#define ARM64_ARCH_H
/*
 * arm64_arch.h — Master include for UIOX AArch64 architecture layer
 * Include this single header to get all AArch64 architecture
 * definitions, instruction formats, macros, and interfaces.
 */
#include "arm64_types.h"
#include "arm64_registers.h"
#include "arm64_psr.h"
#include "arm64_opcodes.h"
#include "arm64_instr_format.h"
#include "arm64_macros.h"
#include "arm64_memory.h"
#include "arm64_exceptions.h"
#include "arm64_sysregs.h"

/* ── CPU state ───────────────────────────────────────────── */
typedef struct arm64_cpu {
    arm64_regfile_t  regs;       /* General-purpose registers X0-X30 */
    arm64_psr_t      psr;        /* PSTATE / SPSR / ELR              */
    arm64_sysregs_t  sysregs;    /* System registers                  */
    arm64_addr_t     pc;         /* Current program counter           */
    arm64_uint32_t   el;         /* Current exception level (0-3)     */
    arm64_bool_t     halted;     /* 1 = CPU halted                    */
} arm64_cpu_t;

/* ── CPU lifecycle ───────────────────────────────────────── */
void arm64_cpu_reset (arm64_cpu_t *cpu);
int  arm64_cpu_step  (arm64_cpu_t *cpu);  /* execute one instruction   */
void arm64_cpu_run   (arm64_cpu_t *cpu);  /* run until halted          */

#endif /* ARM64_ARCH_H */
