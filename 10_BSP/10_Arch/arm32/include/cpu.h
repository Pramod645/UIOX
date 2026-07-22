/*
 * 10_BSP/10_Arch/arm32/include/cpu.h
 *
 * ARM32 CPU IRQ-control helpers.
 *
 * arch_defs.h already owns all barrier/hint macros:
 *   arch_isb()  arch_wfi()  arch_nop()  arch_dsb_sy()  arch_dmb_sy() etc.
 *
 * This file ONLY adds what arch_defs.h does NOT provide:
 *   cpu_irq_enable()  — clear CPSR.I (unmask IRQs via cpsie i)
 *   cpu_irq_disable() — set   CPSR.I (mask   IRQs via cpsid i)
 *
 * @version 1.1.0  @date 2026-07-22
 */
#ifndef UIOX_CPU_ARM32_H
#define UIOX_CPU_ARM32_H

#ifdef __cplusplus
extern "C" {
#endif

/* arch_dsb() — alias for arch_dsb_sy() used by SoC files */
#ifdef arch_dsb_sy
#  define arch_dsb()  arch_dsb_sy()
#endif


/* Unmask IRQs at CPU level — clears CPSR.I */
static inline void cpu_irq_enable(void)
{ __asm__ volatile("cpsie i" ::: "memory"); }

/* Mask IRQs at CPU level — sets CPSR.I */
static inline void cpu_irq_disable(void)
{ __asm__ volatile("cpsid i" ::: "memory"); }

#ifdef __cplusplus
}
#endif
#endif /* UIOX_CPU_ARM32_H */
