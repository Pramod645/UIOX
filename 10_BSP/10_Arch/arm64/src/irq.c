/*
 * 10_BSP/10_Arch/arm64/src/irq.c
 *
 * ARM64 IRQ handler table — GIC-400 implementation.
 *
 * Interrupt controller: GIC-400
 *   Distributor base : GIC_DIST_BASE (from arch_defs.h)
 *   CPU interface    : GIC_CPU_BASE  (from arch_defs.h)
 *   IAR read  → GIC_CPU_IAR   (bits[9:0] = interrupt ID)
 *   EOI write → GIC_CPU_EOIR
 *   Enable    → GIC_DIST_ISENABLER0 + reg*4 (1 bit per IRQ)
 *   Disable   → GIC_DIST_ICENABLER0 + reg*4
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

#ifndef GIC_DIST_ISENABLER0
#  define GIC_DIST_ISENABLER0  (GIC_DIST_BASE + 0x100U)
#endif
#ifndef GIC_DIST_ICENABLER0
#  define GIC_DIST_ICENABLER0  (GIC_DIST_BASE + 0x180U)
#endif

typedef struct { hw_irq_handler_t fn; void *dev_id; uint32_t active; } irq_slot_t;
static irq_slot_t g_slots[IRQ_MAX];

void irq_init(void) {
    for (uint32_t i = 0; i < IRQ_MAX; i++)
        g_slots[i].fn = (hw_irq_handler_t)0, g_slots[i].active = 0u;
}

int irq_request(int irq, hw_irq_handler_t fn, void *dev_id, const char *name) {
    (void)name;
    if (irq < 0 || (uint32_t)irq >= IRQ_MAX || g_slots[irq].active) return -1;
    g_slots[irq].fn = fn; g_slots[irq].dev_id = dev_id; g_slots[irq].active = 1u;
    return 0;
}

void irq_enable(int irq) {
    if (irq < 0 || (uint32_t)irq >= IRQ_MAX) return;
    uint32_t reg = (uint32_t)irq / 32u, bit = (uint32_t)irq % 32u;
    mmio_write32(GIC_DIST_ISENABLER0 + reg * 4u, 1u << bit);
}

void irq_free(int irq) {
    if (irq < 0 || (uint32_t)irq >= IRQ_MAX) return;
    uint32_t reg = (uint32_t)irq / 32u, bit = (uint32_t)irq % 32u;
    mmio_write32(GIC_DIST_ICENABLER0 + reg * 4u, 1u << bit);
    g_slots[irq].fn = (hw_irq_handler_t)0; g_slots[irq].active = 0u;
}

void irq_dispatch(hw_context_t *ctx) {
    uint32_t iar = mmio_read32(GIC_CPU_IAR);
    int irq = (int)(iar & 0x3FFu);
    if (irq >= 1020) return;   /* spurious */
    if (ctx) ctx->irq_num = irq;
    if ((uint32_t)irq < IRQ_MAX && g_slots[irq].active && g_slots[irq].fn)
        g_slots[irq].fn(irq, ctx, g_slots[irq].dev_id);
    mmio_write32(GIC_CPU_EOIR, iar);
}
