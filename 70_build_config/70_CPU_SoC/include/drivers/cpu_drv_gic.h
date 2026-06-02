#ifndef CPU_DRV_GIC_H
#define CPU_DRV_GIC_H
/*
 * cpu_drv_gic.h - ARM GIC-600 driver interface
 * Reference: ARM GIC Architecture Specification IHI0069
 */
#include "../cpu_types.h"
#include "../cpu_irq.h"

/* -- GIC distributor register offsets ----------------------- */
#define GICD_CTLR           0x0000u
#define GICD_TYPER          0x0004u
#define GICD_IIDR           0x0008u
#define GICD_IGROUPR0       0x0080u
#define GICD_ISENABLER0     0x0100u
#define GICD_ICENABLER0     0x0180u
#define GICD_ISPENDR0       0x0200u
#define GICD_ICPENDR0       0x0280u
#define GICD_ISACTIVER0     0x0300u
#define GICD_ICACTIVER0     0x0380u
#define GICD_IPRIORITYR0    0x0400u
#define GICD_ITARGETSR0     0x0800u
#define GICD_ICFGR0         0x0C00u
#define GICD_SGIR           0x0F00u
#define GICD_IROUTER0       0x6000u

/* -- GIC CPU interface register offsets --------------------- */
#define GICC_CTLR           0x0000u
#define GICC_PMR            0x0004u
#define GICC_BPR            0x0008u
#define GICC_IAR            0x000Cu
#define GICC_EOIR           0x0010u
#define GICC_RPR            0x0014u
#define GICC_HPPIR          0x0018u
#define GICC_AIAR           0x0020u
#define GICC_AEOIR          0x0024u

/* -- GIC redistributor register offsets (GICv3+) ------------ */
#define GICR_CTLR           0x0000u
#define GICR_IIDR           0x0004u
#define GICR_TYPER          0x0008u
#define GICR_WAKER          0x0014u
#define GICR_IGROUPR0       0x0080u
#define GICR_ISENABLER0     0x0100u
#define GICR_ICENABLER0     0x0180u
#define GICR_IPRIORITYR0    0x0400u

/* -- GIC context -------------------------------------------- */
typedef struct gic_ctx {
    cpu_addr_t dist_base;
    cpu_addr_t cpu_base;
    cpu_addr_t rdist_base;   /* GICv3 redistributor              */
    cpu_u32_t  num_irqs;
    cpu_u32_t  version;      /* 2 or 3                           */
} gic_ctx_t;

extern gic_ctx_t g_gic;

/* -- API ---------------------------------------------------- */
int  gic_init           (cpu_addr_t dist_base, cpu_addr_t cpu_base,
                          cpu_u32_t version);
void gic_enable_irq     (cpu_u32_t irq);
void gic_disable_irq    (cpu_u32_t irq);
void gic_set_priority   (cpu_u32_t irq, cpu_u32_t prio);
void gic_set_target     (cpu_u32_t irq, cpu_u32_t cpu_mask);
void gic_set_config     (cpu_u32_t irq, cpu_irq_trigger_t trig);
void gic_eoi            (cpu_u32_t irq);
cpu_u32_t gic_ack       (void);
void gic_send_sgi       (cpu_u32_t target_list, cpu_u32_t sgi_id);
void gic_cpu_init       (void);
void gic_print_info     (void);

#endif /* CPU_DRV_GIC_H */
