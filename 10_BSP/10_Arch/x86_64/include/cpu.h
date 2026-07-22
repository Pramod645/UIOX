/*
 * 10_BSP/10_Arch/x86_64/include/cpu.h
 *
 * x86-64 CPU IRQ-control and port I/O helpers.
 *
 * arch_defs.h already defines arch_dsb_sy() (mfence), arch_isb()
 * (compiler fence), arch_wfi() (hlt), arch_outb/arch_inb macros.
 *
 * This file adds:
 *   cpu_irq_enable()   — sti  (set RFLAGS.IF)
 *   cpu_irq_disable()  — cli  (clear RFLAGS.IF)
 *
 * Port I/O helpers (arch_outb/arch_inb) are already in arch_defs.h
 * as macros — NOT redeclared here to avoid conflicts.
 * irq.c and mmio.h provide the inline function forms used by the
 * IRQ layer (arch_outb/arch_inb via the macro from arch_defs.h).
 *
 * @version 1.1.0  @date 2026-07-22
 */
#ifndef UIOX_CPU_X86_64_H
#define UIOX_CPU_X86_64_H

#ifdef __cplusplus
extern "C" {
#endif

/* Unmask IRQs — sets RFLAGS.IF */
static inline void cpu_irq_enable(void)
{ __asm__ volatile("sti" ::: "memory"); }

/* Mask IRQs — clears RFLAGS.IF */
static inline void cpu_irq_disable(void)
{ __asm__ volatile("cli" ::: "memory"); }

#ifdef __cplusplus
}
#endif
#endif /* UIOX_CPU_X86_64_H */
