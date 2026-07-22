/*
 * 10_BSP/10_Arch/riscv64/include/irq.h
 *
 * RISC-V 64 IRQ handler registration API.
 * Interrupt controller: PLIC (Platform-Level Interrupt Controller).
 * Context: S-mode context 1 (hart 0 S-mode = context 2*hart+1 = 1).
 * IRQ numbers are PLIC source numbers (1-based).
 *
 *   UART0_IRQ  10  — PLIC source 10 — QEMU virt NS16550A UART0
 *   TIMER0_IRQ  5  — PLIC source  5 — CLINT timer (routed via PLIC on QEMU)
 *
 * ARCH_TIMER_IRQ_PHYS is defined in arch_defs.h (same value as TIMER0_IRQ).
 *
 * Implementation: 10_Arch/riscv64/src/irq.c → libbsp.a
 *
 * @version 1.0.0  @date 2026-07-22
 */
#ifndef UIOX_IRQ_RISCV64_H
#define UIOX_IRQ_RISCV64_H

#ifndef UIOX_BASETYPES_COMPAT
#  define UIOX_BASETYPES_COMPAT
#endif
#include "uiox_base_types.h"
#include "hw_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define UART0_IRQ    10u   /* PLIC source 10 — QEMU virt NS16550A UART0  */
#define TIMER0_IRQ    5u   /* PLIC source  5 — CLINT timer               */
#ifndef ARCH_TIMER_IRQ_PHYS
#  define ARCH_TIMER_IRQ_PHYS   TIMER0_IRQ   /* PLIC source 5 — CLINT timer */
#endif
#define IRQ_MAX     256u

/* PLIC S-mode context 1 (hart 0) register offsets */
#ifndef PLIC_BASE
#  define PLIC_BASE         0x0C000000UL
#endif
#define PLIC_CTX1_ENABLE    (PLIC_BASE + 0x002080UL)  /* IE[0] for ctx 1 */
#define PLIC_CTX1_THRESHOLD (PLIC_BASE + 0x201000UL)
#define PLIC_CTX1_CLAIM     (PLIC_BASE + 0x201004UL)
//#define PLIC_PRIORITY(src)  (PLIC_BASE + 4UL * (uint64_t)(src))

void irq_init(void);
int  irq_request(int irq, hw_irq_handler_t fn, void *dev_id, const char *name);
void irq_enable(int irq);
void irq_free(int irq);
void irq_dispatch(hw_context_t *ctx);

#ifdef __cplusplus
}
#endif
#endif /* UIOX_IRQ_RISCV64_H */
