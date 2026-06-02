/*
 * cpu_drv_plic.c - RISC-V PLIC driver
 */
#include "../../include/drivers/cpu_drv_plic.h"
#include "../../include/cpu_regs.h"
#include <stdio.h>

plic_ctx_t g_plic;

int plic_init(cpu_addr_t base, cpu_u32_t num_irqs, cpu_u32_t hart_id)
{
    g_plic.base       = base;
    g_plic.num_irqs   = num_irqs;
    g_plic.my_context = PLIC_SMODE_CONTEXT(hart_id);

    /* disable all interrupts by default */
    for (cpu_u32_t i = 0; i < num_irqs; i++) {
        plic_set_priority(i, 0u);
        plic_disable_irq(i, g_plic.my_context);
    }

    /* set threshold to 0 = accept all above priority 0 */
    plic_set_threshold(g_plic.my_context, 0u);

    printf("[plic] base=0x%llx  irqs=%u  context=%u\n",
           (unsigned long long)base, num_irqs,
           g_plic.my_context);
    return CPU_OK;
}

void plic_set_priority(cpu_u32_t irq, cpu_u32_t prio)
{
    cpu_u32_t off = PLIC_PRIORITY_BASE + irq * 4u;
    cpu_mmio_write32(g_plic.base + off, prio);
}

void plic_enable_irq(cpu_u32_t irq, cpu_u32_t context)
{
    cpu_u32_t reg = PLIC_ENABLE_BASE +
                    context * PLIC_CONTEXT_STRIDE +
                    (irq / 32u) * 4u;
    cpu_u32_t val = cpu_mmio_read32(g_plic.base + reg);
    cpu_mmio_write32(g_plic.base + reg, val | (1u << (irq % 32u)));
}

void plic_disable_irq(cpu_u32_t irq, cpu_u32_t context)
{
    cpu_u32_t reg = PLIC_ENABLE_BASE +
                    context * PLIC_CONTEXT_STRIDE +
                    (irq / 32u) * 4u;
    cpu_u32_t val = cpu_mmio_read32(g_plic.base + reg);
    cpu_mmio_write32(g_plic.base + reg, val & ~(1u << (irq % 32u)));
}

void plic_set_threshold(cpu_u32_t context, cpu_u32_t threshold)
{
    cpu_u32_t off = PLIC_THRESHOLD_BASE + context * PLIC_CONTEXT_STRIDE;
    cpu_mmio_write32(g_plic.base + off, threshold);
}

cpu_u32_t plic_claim(cpu_u32_t context)
{
    cpu_u32_t off = PLIC_CLAIM_BASE + context * PLIC_CONTEXT_STRIDE;
    return cpu_mmio_read32(g_plic.base + off);
}

void plic_complete(cpu_u32_t context, cpu_u32_t irq)
{
    cpu_u32_t off = PLIC_CLAIM_BASE + context * PLIC_CONTEXT_STRIDE;
    cpu_mmio_write32(g_plic.base + off, irq);
}

void plic_print_info(void)
{
    printf("[plic] base=0x%llx  irqs=%u  contexts=%u  my_ctx=%u\n",
           (unsigned long long)g_plic.base,
           g_plic.num_irqs,
           g_plic.num_contexts,
           g_plic.my_context);
}
