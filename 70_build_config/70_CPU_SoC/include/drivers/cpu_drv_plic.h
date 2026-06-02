#ifndef CPU_DRV_PLIC_H
#define CPU_DRV_PLIC_H
/*
 * cpu_drv_plic.h - RISC-V PLIC driver interface
 * Reference: RISC-V Platform-Level Interrupt Controller Spec
 */
#include "../cpu_types.h"
#include "../cpu_irq.h"

/* -- PLIC register offsets ---------------------------------- */
#define PLIC_PRIORITY_BASE   0x000000u  /* priority[irq] = base+irq*4 */
#define PLIC_PENDING_BASE    0x001000u
#define PLIC_ENABLE_BASE     0x002000u  /* enable[ctx][irq/32]         */
#define PLIC_THRESHOLD_BASE  0x200000u  /* threshold per context        */
#define PLIC_CLAIM_BASE      0x200004u  /* claim/complete per context   */

#define PLIC_CONTEXT_STRIDE  0x1000u
#define PLIC_MAX_IRQS        1024u
#define PLIC_MAX_CONTEXTS    15872u

/* -- PLIC context ID formula -------------------------------- */
/* For a simple 1-hart system: M-mode context=0, S-mode context=1 */
#define PLIC_SMODE_CONTEXT(hart)  ((hart) * 2 + 1)
#define PLIC_MMODE_CONTEXT(hart)  ((hart) * 2)

/* -- PLIC driver context ------------------------------------ */
typedef struct plic_ctx {
    cpu_addr_t base;
    cpu_u32_t  num_irqs;
    cpu_u32_t  num_contexts;
    cpu_u32_t  my_context;    /* this hart's S-mode context id   */
} plic_ctx_t;

extern plic_ctx_t g_plic;

/* -- API ---------------------------------------------------- */
int  plic_init           (cpu_addr_t base, cpu_u32_t num_irqs,
                           cpu_u32_t hart_id);
void plic_set_priority   (cpu_u32_t irq, cpu_u32_t prio);
void plic_enable_irq     (cpu_u32_t irq, cpu_u32_t context);
void plic_disable_irq    (cpu_u32_t irq, cpu_u32_t context);
void plic_set_threshold  (cpu_u32_t context, cpu_u32_t threshold);
cpu_u32_t plic_claim     (cpu_u32_t context);
void plic_complete       (cpu_u32_t context, cpu_u32_t irq);
void plic_print_info     (void);

#endif /* CPU_DRV_PLIC_H */
