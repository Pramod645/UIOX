/*
 * cpu_irq.c - Generic IRQ subsystem
 */
#include "../include/cpu_irq.h"
#include "../include/cpu_regs.h"
#include <string.h>
#include <stdio.h>

cpu_irq_desc_t g_irq_table[CPU_IRQ_MAX];

int cpu_irq_init(void)
{
    memset(g_irq_table, 0, sizeof(g_irq_table));
    for (int i = 0; i < CPU_IRQ_MAX; i++)
        g_irq_table[i].irq = i;
    return CPU_OK;
}

int cpu_irq_register(int irq, cpu_irq_handler_t handler,
                      void *data, cpu_irq_trigger_t trigger)
{
    if (irq < 0 || irq >= CPU_IRQ_MAX) return CPU_ERR;
    g_irq_table[irq].handler = handler;
    g_irq_table[irq].data    = data;
    g_irq_table[irq].trigger = trigger;
    return CPU_OK;
}

int cpu_irq_unregister(int irq)
{
    if (irq < 0 || irq >= CPU_IRQ_MAX) return CPU_ERR;
    g_irq_table[irq].handler = NULL;
    g_irq_table[irq].data    = NULL;
    g_irq_table[irq].enabled = CPU_FALSE;
    return CPU_OK;
}

void cpu_irq_enable_line (int irq)
{ if (irq>=0 && irq<CPU_IRQ_MAX) g_irq_table[irq].enabled = CPU_TRUE; }

void cpu_irq_disable_line(int irq)
{ if (irq>=0 && irq<CPU_IRQ_MAX) g_irq_table[irq].enabled = CPU_FALSE; }

void cpu_irq_set_priority(int irq, cpu_u32_t prio)
{ if (irq>=0 && irq<CPU_IRQ_MAX) g_irq_table[irq].priority = prio; }

void cpu_irq_dispatch(int irq)
{
    if (irq < 0 || irq >= CPU_IRQ_MAX) return;
    cpu_irq_desc_t *d = &g_irq_table[irq];
    if (d->enabled && d->handler)
        d->handler(irq, d->data);
}

void cpu_irq_eoi(int irq) { (void)irq; /* arch driver handles EOI */ }

int cpu_irq_pending(int irq)
{
    if (irq < 0 || irq >= CPU_IRQ_MAX) return 0;
    return g_irq_table[irq].enabled ? 1 : 0;
}

void cpu_irq_print_table(void)
{
    printf("[irq] Registered IRQ handlers:\n");
    for (int i = 0; i < CPU_IRQ_MAX; i++) {
        if (g_irq_table[i].handler)
            printf("  IRQ %3d  prio=%u  enabled=%u\n",
                   i, g_irq_table[i].priority,
                   g_irq_table[i].enabled);
    }
}
