/*
 * cpu_drv_apic.c - x86-64 LAPIC / IOAPIC driver
 */
#include "../../include/drivers/cpu_drv_apic.h"
#include "../../include/cpu_regs.h"
#include <stdio.h>

apic_ctx_t g_apic;

void lapic_write(cpu_u32_t reg, cpu_u32_t val)
{ cpu_mmio_write32(g_apic.lapic_base + reg, val); }

cpu_u32_t lapic_read(cpu_u32_t reg)
{ return cpu_mmio_read32(g_apic.lapic_base + reg); }

int lapic_init(cpu_addr_t base)
{
    g_apic.lapic_base = base;
    g_apic.lapic_id   = lapic_read(LAPIC_ID_REG) >> 24;

    /* enable LAPIC via SVR: set bit 8 + spurious vector 0xFF */
    lapic_write(LAPIC_SVR, (1u << 8) | LAPIC_SPURIOUS_VEC);

    /* mask all LVT entries */
    lapic_write(LAPIC_TIMER_LVT, (1u << 16));

    /* set task priority to 0 — accept all interrupts */
    lapic_write(LAPIC_TPR, 0u);

    /* set flat logical destination */
    lapic_write(0x0D0u, 0xFF000000u); /* LDR */
    lapic_write(0x0E0u, 0xFFFFFFFFu); /* DFR flat model */

    printf("[lapic] ID=%u  base=0x%llx\n",
           g_apic.lapic_id,
           (unsigned long long)base);
    return CPU_OK;
}

void lapic_enable(void)
{ lapic_write(LAPIC_SVR, lapic_read(LAPIC_SVR) | (1u << 8)); }

void lapic_eoi(void)
{ lapic_write(LAPIC_EOI, 0u); }

void lapic_send_ipi(cpu_u32_t dest, cpu_u32_t vec, cpu_u32_t delivery)
{
    lapic_write(LAPIC_ICR_HI, dest << 24);
    lapic_write(LAPIC_ICR_LO, vec | delivery | (1u << 14));
}

void lapic_send_init_ipi(cpu_u32_t apic_id)
{
    lapic_write(LAPIC_ICR_HI, apic_id << 24);
    lapic_write(LAPIC_ICR_LO, LAPIC_DM_INIT | (1u << 14));
}

void lapic_send_sipi(cpu_u32_t apic_id, cpu_u8_t vector)
{
    lapic_write(LAPIC_ICR_HI, apic_id << 24);
    lapic_write(LAPIC_ICR_LO, LAPIC_DM_STARTUP | (1u << 14) | vector);
}

void lapic_timer_init(cpu_u32_t vector, cpu_u32_t count, cpu_u32_t mode)
{
    lapic_write(LAPIC_TIMER_DCR, 0x3u); /* divide by 16 */
    lapic_write(LAPIC_TIMER_LVT, mode | vector);
    lapic_write(LAPIC_TIMER_ICR, count);
}

void lapic_timer_stop(void)
{ lapic_write(LAPIC_TIMER_LVT, (1u << 16)); }

cpu_u32_t lapic_get_id(void)
{ return lapic_read(LAPIC_ID_REG) >> 24; }

/* -- IOAPIC ------------------------------------------------- */
void ioapic_write(cpu_u32_t reg, cpu_u32_t val)
{
    cpu_mmio_write32(g_apic.ioapic_base + IOAPIC_REGSEL, reg);
    cpu_mmio_write32(g_apic.ioapic_base + IOAPIC_IOWIN,  val);
}

cpu_u32_t ioapic_read(cpu_u32_t reg)
{
    cpu_mmio_write32(g_apic.ioapic_base + IOAPIC_REGSEL, reg);
    return cpu_mmio_read32(g_apic.ioapic_base + IOAPIC_IOWIN);
}

int ioapic_init(cpu_addr_t base)
{
    g_apic.ioapic_base = base;
    g_apic.ioapic_id   = (ioapic_read(0x00) >> 24) & 0xF;
    g_apic.ioapic_max_redir =
        ((ioapic_read(0x01) >> 16) & 0xFF) + 1u;
    printf("[ioapic] ID=%u  base=0x%llx  max_redir=%u\n",
           g_apic.ioapic_id,
           (unsigned long long)base,
           g_apic.ioapic_max_redir);
    return CPU_OK;
}

void ioapic_set_redir(cpu_u32_t irq, cpu_u8_t vector,
                       cpu_u32_t flags, cpu_u8_t dest)
{
    cpu_u32_t lo = vector | flags;
    cpu_u32_t hi = (cpu_u32_t)dest << 24;
    ioapic_write(IOAPIC_REDTBL_BASE + irq * 2u,     lo);
    ioapic_write(IOAPIC_REDTBL_BASE + irq * 2u + 1u, hi);
}

void ioapic_mask_irq(cpu_u32_t irq)
{
    cpu_u32_t lo = ioapic_read(IOAPIC_REDTBL_BASE + irq * 2u);
    ioapic_write(IOAPIC_REDTBL_BASE + irq * 2u, lo | (1u << 16));
}

void ioapic_unmask_irq(cpu_u32_t irq)
{
    cpu_u32_t lo = ioapic_read(IOAPIC_REDTBL_BASE + irq * 2u);
    ioapic_write(IOAPIC_REDTBL_BASE + irq * 2u, lo & ~(1u << 16));
}

void pic8259_disable(void)
{
    /* mask all IRQs on both PICs */
    cpu_outb(0x21, 0xFF);
    cpu_outb(0xA1, 0xFF);
}

void pic8259_init(cpu_u8_t master_vec, cpu_u8_t slave_vec)
{
    /* ICW1: cascade, edge-triggered */
    cpu_outb(0x20, 0x11); cpu_outb(0xA0, 0x11);
    /* ICW2: vector offsets */
    cpu_outb(0x21, master_vec);
    cpu_outb(0xA1, slave_vec);
    /* ICW3: master IRQ2 = slave; slave ID = 2 */
    cpu_outb(0x21, 0x04); cpu_outb(0xA1, 0x02);
    /* ICW4: 8086 mode */
    cpu_outb(0x21, 0x01); cpu_outb(0xA1, 0x01);
    /* mask all */
    cpu_outb(0x21, 0xFF); cpu_outb(0xA1, 0xFF);
}

void pic8259_eoi(cpu_u32_t irq)
{
    if (irq >= 8) cpu_outb(0xA0, 0x20);
    cpu_outb(0x20, 0x20);
}

void apic_print_info(void)
{
    printf("[apic] LAPIC id=%u base=0x%llx\n",
           g_apic.lapic_id,
           (unsigned long long)g_apic.lapic_base);
    printf("[apic] IOAPIC id=%u base=0x%llx max_redir=%u\n",
           g_apic.ioapic_id,
           (unsigned long long)g_apic.ioapic_base,
           g_apic.ioapic_max_redir);
}

