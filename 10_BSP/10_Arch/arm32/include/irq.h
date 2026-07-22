/*
 * 10_BSP/10_Arch/arm32/include/irq.h
 *
 * ARM32 IRQ handler registration API.
 * Interrupt controller: GIC-400 (same register layout as ARM64).
 * IRQ numbers from arch_defs.h (QEMU versatilepb defaults).
 *
 *   UART0_IRQ  69  — GIC SPI 37 — PL011 UART0 (versatilepb)
 *   TIMER0_IRQ 29  — PPI        — EL1 physical timer (ARCH_TIMER_IRQ_PHYS)
 *
 * Implementation: 10_Arch/arm32/src/irq.c → libbsp.a
 *
 * @version 1.0.0  @date 2026-07-22
 */
#ifndef UIOX_IRQ_ARM32_H
#define UIOX_IRQ_ARM32_H

#ifndef UIOX_BASETYPES_COMPAT
#  define UIOX_BASETYPES_COMPAT
#endif
#include "uiox_base_types.h"
#include "hw_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define UART0_IRQ    69u   /* GIC SPI 37 — QEMU versatilepb PL011 UART0  */
#define TIMER0_IRQ   29u   /* GIC PPI 29 — EL1 physical timer             */
#define IRQ_MAX     256u

void irq_init(void);
int  irq_request(int irq, hw_irq_handler_t fn, void *dev_id, const char *name);
void irq_enable(int irq);
void irq_free(int irq);
void irq_dispatch(hw_context_t *ctx);

#ifdef __cplusplus
}
#endif
#endif /* UIOX_IRQ_ARM32_H */
