#ifndef UIOX_CPU_H
#define UIOX_CPU_H

/*
 * cpu.h
 *
 * CPU-level operations: interrupt enable/disable, context
 * save/restore, and architecture-specific control register access.
 *
 * These are the hardware-level equivalents of the kernel's
 * setjmp/longjmp used in the device-open algorithm: when an
 * interrupt occurs the hardware saves the full register file so
 * the kernel can resume the interrupted process transparently.
 *
 * Three implementations are provided via #ifdef:
 *   UIOX_ARCH_ARM64   — DAIF, MRS/MSR, ISB, DSB
 *   UIOX_ARCH_ARM32   — CPSR, CPS, DMB
 *   UIOX_ARCH_X86_64  — CLI/STI, PUSHF/POPF, CPUID
 *
 * On a hosted (Linux/macOS) build all inline asm blocks are
 * guarded so the file compiles cleanly with a cross-compiler
 * or a native compiler when the matching -march is set.
 */

#include "hw_types.h"

/* =============================================================
 * Global interrupt enable / disable
 *
 * cpu_irq_disable — mask all IRQs; returns previous CPU flags
 *                   so they can be restored with cpu_irq_restore.
 * cpu_irq_enable  — unconditionally unmask IRQs.
 * cpu_irq_restore — restore flags returned by cpu_irq_disable.
 *
 * Typical usage (critical section):
 *
 *   uint64_t flags = cpu_irq_disable();
 *   // ... critical section ...
 *   cpu_irq_restore(flags);
 * ============================================================= */
uint64_t cpu_irq_disable(void);
void     cpu_irq_enable (void);
void     cpu_irq_restore(uint64_t flags);

/* Query whether IRQs are currently enabled (non-zero = enabled) */
int      cpu_irq_enabled(void);

/* =============================================================
 * CPU context save / restore
 *
 * These are the software equivalents of the hardware's automatic
 * context save on interrupt entry.  The device-open algorithm
 * calls cpu_context_save() (setjmp equivalent) before invoking
 * the driver open routine so that a driver longjmp on fatal error
 * can unwind back to the kernel safely.
 *
 * cpu_context_save   — save registers into *ctx; returns 0 on
 *                      the initial call, non-zero on return from
 *                      cpu_context_restore (longjmp return).
 * cpu_context_restore — restore registers from *ctx; does not
 *                       return to the caller.
 * ============================================================= */
int  cpu_context_save   (hw_context_t *ctx);
void cpu_context_restore(hw_context_t *ctx);

/* =============================================================
 * Architecture-specific control register access
 * ============================================================= */

/* ARM64 system register read/write (MRS / MSR) */
uint64_t arm64_read_daif    (void);
void     arm64_write_daif   (uint64_t val);
uint64_t arm64_read_elr_el1 (void);
uint64_t arm64_read_spsr_el1(void);
uint64_t arm64_read_mpidr   (void);   /* CPU affinity / core ID */
uint64_t arm64_read_cntpct  (void);   /* physical counter       */

/* ARM32 CPSR / SPSR read (MRS) */
uint32_t arm32_read_cpsr(void);
void     arm32_write_cpsr(uint32_t val);

/* x86_64 control registers and MSRs */
uint64_t x86_read_rflags(void);
void     x86_write_rflags(uint64_t val);
uint64_t x86_read_msr(uint32_t msr_id);
void     x86_write_msr(uint32_t msr_id, uint64_t val);
uint32_t x86_cpuid_family(void);      /* family/model/stepping  */

/* =============================================================
 * Memory barriers (architecture-specific)
 * ============================================================= */
void cpu_mb (void);   /* full  memory barrier                     */
void cpu_rmb(void);   /* read  memory barrier                     */
void cpu_wmb(void);   /* write memory barrier                     */
void cpu_isb(void);   /* instruction synchronisation barrier       */
void cpu_dsb(void);   /* data synchronisation barrier (ARM)        */

/* =============================================================
 * CPU identification
 * ============================================================= */
typedef struct {
    const char *ci_arch;      /* "arm64" / "arm32" / "x86_64"     */
    uint32_t    ci_family;    /* processor family code             */
    uint32_t    ci_model;     /* processor model code              */
    uint32_t    ci_stepping;  /* silicon revision                  */
    uint32_t    ci_ncores;    /* number of cores (MPIDR / APIC)    */
} cpu_info_t;

/* Populate *info with the current CPU's identity */
void cpu_identify(cpu_info_t *info);

/* Debug: print CPU info */
void cpu_print_info(const cpu_info_t *info);

/* =============================================================
 * Halt / idle helpers
 * ============================================================= */
void cpu_halt(void);      /* HLT / WFI — wait for next interrupt  */
void cpu_nop (void);      /* NOP — used in busy-wait loops        */
void cpu_wfe (void);      /* ARM WFE — wait-for-event             */
void cpu_sev (void);      /* ARM SEV — send-event (wake other CPU)*/

#endif /* UIOX_CPU_H */
