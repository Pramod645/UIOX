/*
 * 10_BSP/10_Arch/riscv64/include/cpu.h
 *
 * RISC-V 64 CPU IRQ-control helpers.
 *
 * arch_defs.h already owns barrier macros and CSR helpers
 * (arch_csrs_sie, arch_csrc_sie, arch_csrs_sstatus etc.).
 *
 * This file ONLY adds:
 *   cpu_irq_enable()  — sets sstatus.SIE (bit 1) — S-mode IRQ unmask
 *   cpu_irq_disable() — clears sstatus.SIE       — S-mode IRQ mask
 *
 * @version 1.1.0  @date 2026-07-22
 */
#ifndef UIOX_CPU_RISCV64_H
#define UIOX_CPU_RISCV64_H

#ifdef __cplusplus
extern "C" {
#endif


/* Unmask S-mode interrupts — sets sstatus.SIE (bit 1) */
static inline void cpu_irq_enable(void)
{ __asm__ volatile("csrsi sstatus, 0x2" ::: "memory"); }

/* Mask S-mode interrupts — clears sstatus.SIE (bit 1) */
static inline void cpu_irq_disable(void)
{ __asm__ volatile("csrci sstatus, 0x2" ::: "memory"); }

#ifdef __cplusplus
}
#endif
#endif /* UIOX_CPU_RISCV64_H */
