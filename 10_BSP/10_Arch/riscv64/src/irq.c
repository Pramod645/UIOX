/*
 * 10_BSP/10_Arch/riscv64/src/irq.c
 *
 * RISC-V 64 IRQ handler table — PLIC S-mode context 1 implementation.
 *
 * PLIC S-mode context 1 (hart 0):
 *   Priority   : PLIC_PRIORITY(src) — set to 1 to enable
 *   Enable     : PLIC_CTX1_ENABLE + (src/32)*4  bit (src%32)
 *   Threshold  : PLIC_CTX1_THRESHOLD = 0 (allow all priorities)
 *   Claim/EOI  : PLIC_CTX1_CLAIM — read=claim, write=complete
 *
 * @version 1.0.0  @date 2026-07-22
 */
#ifndef UIOX_BASETYPES_COMPAT
#  define UIOX_BASETYPES_COMPAT
#endif
#include "uiox_base_types.h"
#include "../include/hw_types.h"
#include "../include/irq.h"
#include "../include/mmio.h"
#include "arch_defs.h"

typedef struct { hw_irq_handler_t fn; void *dev_id; uint32_t active; } irq_slot_t;
static irq_slot_t g_slots[IRQ_MAX];

void irq_init(void) {
    /* Set PLIC threshold to 0 — allow all priorities */
    mmio_write32(PLIC_CTX1_THRESHOLD, 0u);
    for (uint32_t i = 0; i < IRQ_MAX; i++)
        g_slots[i].fn = (hw_irq_handler_t)0, g_slots[i].active = 0u;
}

int irq_request(int irq, hw_irq_handler_t fn, void *dev_id, const char *name) {
    (void)name;
    if (irq < 0 || (uint32_t)irq >= IRQ_MAX || g_slots[irq].active) return -1;
    g_slots[irq].fn = fn; g_slots[irq].dev_id = dev_id; g_slots[irq].active = 1u;
    /* Set PLIC source priority = 1 so it can fire */
    mmio_write32(PLIC_PRIORITY((uint32_t)irq), 1u);
    return 0;
}

void irq_enable(int irq) {
    if (irq < 0 || (uint32_t)irq >= IRQ_MAX) return;
    uint32_t reg = (uint32_t)irq / 32u, bit = (uint32_t)irq % 32u;
    uintptr_t a = PLIC_CTX1_ENABLE + reg * 4u;
    mmio_write32(a, mmio_read32(a) | (1u << bit));
}

void irq_free(int irq) {
    if (irq < 0 || (uint32_t)irq >= IRQ_MAX) return;
    uint32_t reg = (uint32_t)irq / 32u, bit = (uint32_t)irq % 32u;
    uintptr_t a = PLIC_CTX1_ENABLE + reg * 4u;
    mmio_write32(a, mmio_read32(a) & ~(1u << bit));
    mmio_write32(PLIC_PRIORITY((uint32_t)irq), 0u);
    g_slots[irq].fn = (hw_irq_handler_t)0; g_slots[irq].active = 0u;
}

void irq_dispatch(hw_context_t *ctx) {
    uint32_t src = mmio_read32(PLIC_CTX1_CLAIM);
    if (src == 0u) return;   /* no pending interrupt */
    int irq = (int)src;
    if (ctx) ctx->irq_num = irq;
    if ((uint32_t)irq < IRQ_MAX && g_slots[irq].active && g_slots[irq].fn)
        g_slots[irq].fn(irq, ctx, g_slots[irq].dev_id);
    mmio_write32(PLIC_CTX1_CLAIM, src);   /* complete */
}
