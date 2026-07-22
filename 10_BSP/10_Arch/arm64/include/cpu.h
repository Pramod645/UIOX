/*
 * 10_BSP/10_Arch/arm64/include/cpu.h
 *
 * ARM64 CPU IRQ-control helpers.
 *
 * arch_defs.h already owns all barrier/hint macros:
 *   arch_isb()  arch_wfi()  arch_nop()  arch_dsb_sy()  arch_dmb_sy() etc.
 *
 * This file ONLY adds what arch_defs.h does NOT provide:
 *   cpu_irq_enable()  — clear DAIF.I bit (unmask IRQs)
 *   cpu_irq_disable() — set   DAIF.I bit (mask   IRQs)
 *
 * Do NOT redeclare arch_isb/arch_wfi/arch_nop here — they are already
 * defined as zero-arg macros in arch_defs.h and redefining them as inline
 * functions causes a preprocessor error ("macro passed N args, takes 0").
 *
 * @version 1.1.0  @date 2026-07-22
 */
#ifndef UIOX_CPU_ARM64_H
#define UIOX_CPU_ARM64_H

#ifdef __cplusplus
extern "C" {
#endif

/* Unmask IRQs at CPU level — clears DAIF.I (bit 1) */
static inline void cpu_irq_enable(void)
{ __asm__ volatile("msr daifclr, #2" ::: "memory"); }

/* Mask IRQs at CPU level — sets DAIF.I (bit 1) */
static inline void cpu_irq_disable(void)
{ __asm__ volatile("msr daifset, #2" ::: "memory"); }

#ifdef __cplusplus
}
#endif
#endif /* UIOX_CPU_ARM64_H */
