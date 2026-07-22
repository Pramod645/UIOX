/*
 * 10_BSP/10_Arch/x86_64/src/irq.c
 *
 * x86-64 IRQ handler table — LAPIC + 8259A PIC implementation.
 *
 * Hardware flow:
 *   1. Exception/IRQ fires → CPU pushes iretq frame onto stack
 *   2. IDT stub (vectors.S) pushes GPRs + vector number → calls irq_dispatch()
 *   3. irq_dispatch() reads ctx->vector, calls registered handler
 *   4. irq_dispatch() sends LAPIC EOI (for LAPIC-routed IRQs)
 *      OR sends PIC EOI (for legacy 8259A IRQs < vector base + 16)
 *
 * 8259A PIC EOI ports:
 *   Master PIC command: 0x20  — EOI = 0x20
 *   Slave  PIC command: 0xA0  — EOI = 0x20 (plus master ACK for IRQ 8-15)
 *
 * LAPIC EOI: write 0 to LAPIC_EOI (offset 0x0B0 from LAPIC_BASE).
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
#include "../include/cpu.h"

#include "arch_defs.h"

/* 8259A PIC I/O ports */
#define PIC1_CMD   0x20u   /* Master PIC command port */
#define PIC2_CMD   0xA0u   /* Slave  PIC command port */
#define PIC_EOI    0x20u   /* End-of-Interrupt command */

/* LAPIC EOI register */
#ifndef LAPIC_BASE
#  define LAPIC_BASE  0xFEE00000UL
#endif
#define LAPIC_EOI_REG  0x0B0U

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

/* irq_enable / irq_free: for legacy 8259A PIC unmask IMR bit.
 * For LAPIC/IOAPIC the full driver in 03_SoC handles mask bits.
 * Vectors below IRQ_VECTOR_BASE are CPU exceptions — never masked here. */
void irq_enable(int irq) {
    if (irq < (int)IRQ_VECTOR_BASE || (uint32_t)irq >= IRQ_MAX) return;
    int pic_irq = irq - (int)IRQ_VECTOR_BASE;
    if (pic_irq < 8) {
        uint8_t mask = arch_inb(0x21u);          /* Master PIC IMR */
        arch_outb(0x21u, mask & ~(uint8_t)(1u << pic_irq));
    } else if (pic_irq < 16) {
        uint8_t mask = arch_inb(0xA1u);          /* Slave  PIC IMR */
        arch_outb(0xA1u, mask & ~(uint8_t)(1u << (pic_irq - 8)));
    }
}

void irq_free(int irq) {
    if (irq < 0 || (uint32_t)irq >= IRQ_MAX) return;
    int pic_irq = irq - (int)IRQ_VECTOR_BASE;
    if (pic_irq >= 0 && pic_irq < 8) {
        uint8_t mask = arch_inb(0x21u);
        arch_outb(0x21u, mask | (uint8_t)(1u << pic_irq));
    } else if (pic_irq >= 8 && pic_irq < 16) {
        uint8_t mask = arch_inb(0xA1u);
        arch_outb(0xA1u, mask | (uint8_t)(1u << (pic_irq - 8)));
    }
    g_slots[irq].fn = (hw_irq_handler_t)0; g_slots[irq].active = 0u;
}

void irq_dispatch(hw_context_t *ctx) {
    if (!ctx) return;
    int irq = (int)ctx->vector;
    ctx->irq_num = irq;

    if ((uint32_t)irq < IRQ_MAX && g_slots[irq].active && g_slots[irq].fn)
        g_slots[irq].fn(irq, ctx, g_slots[irq].dev_id);

    /* Send EOI */
    int pic_irq = irq - (int)IRQ_VECTOR_BASE;
    if (pic_irq >= 0 && pic_irq < 16) {
        /* Legacy 8259A PIC EOI */
        if (pic_irq >= 8) arch_outb(PIC2_CMD, PIC_EOI);  /* Slave first */
        arch_outb(PIC1_CMD, PIC_EOI);                      /* Then master */
    } else {
        /* LAPIC EOI — write 0 to EOI register */
        mmio_write32((uintptr_t)(LAPIC_BASE + LAPIC_EOI_REG), 0u);
    }
}
