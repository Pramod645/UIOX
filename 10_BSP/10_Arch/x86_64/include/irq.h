/*
 * 10_BSP/10_Arch/x86_64/include/irq.h
 *
 * x86-64 IRQ handler registration API.
 * Interrupt controller: LAPIC + I/O APIC (or legacy 8259A PIC remapped).
 * IRQ vectors are APIC vector numbers (32–255 for hardware IRQs).
 *
 * 8259A PIC remapping (done in arch_init.c):
 *   Master PIC: IRQ 0–7  → vectors 32–39
 *   Slave  PIC: IRQ 8–15 → vectors 40–47
 *
 *   UART0_IRQ  36  — vector 36 — COM1 NS16550A (8259 IRQ4 + offset 32)
 *   TIMER0_IRQ 32  — vector 32 — PIT timer     (8259 IRQ0 + offset 32)
 *
 * Implementation: 10_Arch/x86_64/src/irq.c → libbsp.a
 *
 * @version 1.0.0  @date 2026-07-22
 */
#ifndef UIOX_IRQ_X86_64_H
#define UIOX_IRQ_X86_64_H

#ifndef UIOX_BASETYPES_COMPAT
#  define UIOX_BASETYPES_COMPAT
#endif
#include "uiox_base_types.h"
#include "hw_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define UART0_IRQ    36u   /* APIC vector 36 — COM1 (8259 IRQ4 + base 32) */
#define TIMER0_IRQ   32u   /* APIC vector 32 — PIT  (8259 IRQ0 + base 32) */
#define IRQ_VECTOR_BASE 32u  /* PIC remap base — vectors 0-31 are exceptions */
#define IRQ_MAX     256u

void irq_init(void);
int  irq_request(int irq, hw_irq_handler_t fn, void *dev_id, const char *name);
void irq_enable(int irq);
void irq_free(int irq);
void irq_dispatch(hw_context_t *ctx);

#ifdef __cplusplus
}
#endif
#endif /* UIOX_IRQ_X86_64_H */
