#ifndef CPU_IRQ_H
#define CPU_IRQ_H
/*
 * cpu_irq.h - Interrupt controller interface (GIC/APIC/PLIC)
 */
#include "cpu_types.h"

#define CPU_IRQ_MAX   1024

/* -- IRQ handler type --------------------------------------- */
typedef void (*cpu_irq_handler_t)(int irq, void *data);

/* -- IRQ trigger types -------------------------------------- */
typedef enum cpu_irq_trigger {
    CPU_IRQ_LEVEL_HIGH  = 0,
    CPU_IRQ_LEVEL_LOW   = 1,
    CPU_IRQ_EDGE_RISING = 2,
    CPU_IRQ_EDGE_FALLING= 3,
} cpu_irq_trigger_t;

/* -- IRQ descriptor ----------------------------------------- */
typedef struct cpu_irq_desc {
    int                irq;
    cpu_irq_handler_t  handler;
    void              *data;
    cpu_irq_trigger_t  trigger;
    cpu_u32_t          priority;
    cpu_bool_t         enabled;
} cpu_irq_desc_t;

/* -- Global IRQ table --------------------------------------- */
extern cpu_irq_desc_t g_irq_table[CPU_IRQ_MAX];

/* -- API ---------------------------------------------------- */
int  cpu_irq_init          (void);
int  cpu_irq_register      (int irq, cpu_irq_handler_t handler,
                             void *data, cpu_irq_trigger_t trigger);
int  cpu_irq_unregister    (int irq);
void cpu_irq_enable_line   (int irq);
void cpu_irq_disable_line  (int irq);
void cpu_irq_set_priority  (int irq, cpu_u32_t prio);
void cpu_irq_set_affinity  (int irq, cpu_u32_t core_mask);
void cpu_irq_dispatch      (int irq);
void cpu_irq_eoi           (int irq);
int  cpu_irq_pending       (int irq);
void cpu_irq_print_table   (void);

#endif
