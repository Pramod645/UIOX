#ifndef CPU_SOC_H
#define CPU_SOC_H
/*
 * cpu_soc.h - SoC-level master include + init API
 */
#include "cpu_types.h"
#include "cpu_features.h"
#include "cpu_regs.h"
#include "cpu_cache.h"
#include "cpu_mmu.h"
#include "cpu_irq.h"
#include "cpu_timer.h"
#include "cpu_power.h"
#include "cpu_smp.h"
#include "cpu_context.h"
#include "cpu_debug.h"
#include "cpu_io.h"

/* -- SoC descriptor ----------------------------------------- */
typedef struct cpu_soc_desc {
    const char   *name;          /* "Cortex-A76 SoC", "Ice Lake"  */
    cpu_arch_t    arch;
    cpu_u32_t     num_cores;
    cpu_u32_t     num_clusters;
    cpu_u64_t     dram_base;
    cpu_u64_t     dram_size;
    cpu_u64_t     mmio_base;
    cpu_u64_t     mmio_size;
    cpu_u32_t     gic_dist_base;  /* ARM GIC / 0 for x86/RISC-V  */
    cpu_u32_t     gic_cpu_base;
    cpu_u32_t     plic_base;      /* RISC-V PLIC / 0 for others   */
    cpu_u32_t     lapic_base;     /* x86 LAPIC / 0 for others     */
    cpu_u32_t     uart_base;
    cpu_u32_t     uart_irq;
    cpu_u32_t     timer_irq;
} cpu_soc_desc_t;

extern const cpu_soc_desc_t *g_soc;

/* -- SoC init pipeline -------------------------------------- */
int cpu_soc_init        (const cpu_soc_desc_t *soc);
int cpu_soc_early_init  (void);   /* called before MMU/cache on  */
int cpu_soc_late_init   (void);   /* called after MMU/cache on   */
void cpu_soc_print_info (void);

/* -- Well-known SoC descriptors ----------------------------- */
extern const cpu_soc_desc_t soc_cortex_a76;
extern const cpu_soc_desc_t soc_x86_64_generic;
extern const cpu_soc_desc_t soc_riscv64_generic;

#endif /* CPU_SOC_H */
