/*
 * cpu_drv_gic.c - ARM GIC-600 driver
 */
#include "../../include/drivers/cpu_drv_gic.h"
#include "../../include/cpu_regs.h"
#include <stdio.h>

gic_ctx_t g_gic;

int gic_init(cpu_addr_t dist_base, cpu_addr_t cpu_base,
              cpu_u32_t version)
{
    g_gic.dist_base = dist_base;
    g_gic.cpu_base  = cpu_base;
    g_gic.version   = version;

    /* read number of IRQs from TYPER */
    cpu_u32_t typer  = cpu_mmio_read32(dist_base + GICD_TYPER);
    g_gic.num_irqs   = ((typer & 0x1Fu) + 1u) * 32u;

    /* disable distributor */
    cpu_mmio_write32(dist_base + GICD_CTLR, 0u);

    /* configure all SPIs as non-secure group 1 */
    for (cpu_u32_t i = 1; i < g_gic.num_irqs / 32; i++)
        cpu_mmio_write32(dist_base + GICD_IGROUPR0 + i * 4u, 0xFFFFFFFFu);

    /* set all priorities to mid-level */
    for (cpu_u32_t i = 0; i < g_gic.num_irqs / 4u; i++)
        cpu_mmio_write32(dist_base + GICD_IPRIORITYR0 + i * 4u,
                          0xA0A0A0A0u);

    /* set all targets to CPU0 */
    for (cpu_u32_t i = 8u; i < g_gic.num_irqs / 4u; i++)
        cpu_mmio_write32(dist_base + GICD_ITARGETSR0 + i * 4u,
                          0x01010101u);

    /* set all to level-triggered */
    for (cpu_u32_t i = 1u; i < g_gic.num_irqs / 16u; i++)
        cpu_mmio_write32(dist_base + GICD_ICFGR0 + i * 4u, 0u);

    /* enable distributor */
    cpu_mmio_write32(dist_base + GICD_CTLR, (version >= 3) ? 0x3u : 0x1u);

    gic_cpu_init();

    printf("[gic] GIC-v%u  dist=0x%llx  irqs=%u\n",
           version,
           (unsigned long long)dist_base,
           g_gic.num_irqs);
    return CPU_OK;
}

void gic_cpu_init(void)
{
    /* enable CPU interface */
    cpu_mmio_write32(g_gic.cpu_base + GICC_PMR,  0xF0u); /* all prios */
    cpu_mmio_write32(g_gic.cpu_base + GICC_BPR,  0x3u);
    cpu_mmio_write32(g_gic.cpu_base + GICC_CTLR, 0x1u);
}

void gic_enable_irq(cpu_u32_t irq)
{
    cpu_u32_t reg = irq / 32u;
    cpu_u32_t bit = irq % 32u;
    cpu_mmio_write32(g_gic.dist_base + GICD_ISENABLER0 + reg * 4u,
                      1u << bit);
}

void gic_disable_irq(cpu_u32_t irq)
{
    cpu_u32_t reg = irq / 32u;
    cpu_u32_t bit = irq % 32u;
    cpu_mmio_write32(g_gic.dist_base + GICD_ICENABLER0 + reg * 4u,
                      1u << bit);
}

void gic_set_priority(cpu_u32_t irq, cpu_u32_t prio)
{
    cpu_u32_t reg  = irq / 4u;
    cpu_u32_t byte = irq % 4u;
    cpu_u32_t val  = cpu_mmio_read32(
                         g_gic.dist_base + GICD_IPRIORITYR0 + reg * 4u);
    val &= ~(0xFFu << (byte * 8u));
    val |=  ((prio & 0xFFu) << (byte * 8u));
    cpu_mmio_write32(g_gic.dist_base + GICD_IPRIORITYR0 + reg * 4u, val);
}

void gic_set_target(cpu_u32_t irq, cpu_u32_t cpu_mask)
{
    cpu_u32_t reg  = irq / 4u;
    cpu_u32_t byte = irq % 4u;
    cpu_u32_t val  = cpu_mmio_read32(
                         g_gic.dist_base + GICD_ITARGETSR0 + reg * 4u);
    val &= ~(0xFFu << (byte * 8u));
    val |=  ((cpu_mask & 0xFFu) << (byte * 8u));
    cpu_mmio_write32(g_gic.dist_base + GICD_ITARGETSR0 + reg * 4u, val);
}

void gic_set_config(cpu_u32_t irq, cpu_irq_trigger_t trig)
{
    cpu_u32_t reg  = irq / 16u;
    cpu_u32_t bit  = (irq % 16u) * 2u;
    cpu_u32_t val  = cpu_mmio_read32(
                         g_gic.dist_base + GICD_ICFGR0 + reg * 4u);
    if (trig == CPU_IRQ_EDGE_RISING || trig == CPU_IRQ_EDGE_FALLING)
        val |=  (2u << bit);
    else
        val &= ~(2u << bit);
    cpu_mmio_write32(g_gic.dist_base + GICD_ICFGR0 + reg * 4u, val);
}

cpu_u32_t gic_ack(void)
{
    return cpu_mmio_read32(g_gic.cpu_base + GICC_IAR) & 0x3FFu;
}

void gic_eoi(cpu_u32_t irq)
{
    cpu_mmio_write32(g_gic.cpu_base + GICC_EOIR, irq & 0x3FFu);
}

void gic_send_sgi(cpu_u32_t target_list, cpu_u32_t sgi_id)
{
    cpu_u32_t val = ((target_list & 0xFF) << 16) | (sgi_id & 0xF);
    cpu_mmio_write32(g_gic.dist_base + GICD_SGIR, val);
}

void gic_print_info(void)
{
    printf("[gic] version=%u  dist=0x%llx  cpu=0x%llx  irqs=%u\n",
           g_gic.version,
           (unsigned long long)g_gic.dist_base,
           (unsigned long long)g_gic.cpu_base,
           g_gic.num_irqs);
}
