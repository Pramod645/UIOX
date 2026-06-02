#ifndef CORTEX_A76_H
#define CORTEX_A76_H
/*
 * cortex_a76.h - ARM Cortex-A76 specific definitions
 * Reference: ARM Cortex-A76 TRM (DDI0582)
 */
#include "../cpu_types.h"

/* -- MIDR values -------------------------------------------- */
#define CORTEX_A76_MIDR_PARTNUM   0xD0Bu
#define CORTEX_A76_IMPLEMENTER    0x41u   /* ARM Ltd              */

/* -- L1 / L2 sizes ------------------------------------------ */
#define CORTEX_A76_L1I_SIZE_KB    64u
#define CORTEX_A76_L1D_SIZE_KB    64u
#define CORTEX_A76_L2_SIZE_KB     512u
#define CORTEX_A76_L3_SIZE_KB     (4u * 1024u)
#define CORTEX_A76_CACHE_LINE     64u

/* -- GIC-600 base addresses (QEMU virt) --------------------- */
#define A76_GIC_DIST_BASE         0x08000000u
#define A76_GIC_RDIST_BASE        0x080A0000u
#define A76_GIC_CPU_BASE          0x08010000u

/* -- PL011 UART --------------------------------------------- */
#define A76_UART0_BASE            0x09000000u
#define A76_UART0_IRQ             33u

/* -- Generic timer IRQs ------------------------------------- */
#define A76_TIMER_PHYS_IRQ        30u
#define A76_TIMER_VIRT_IRQ        27u
#define A76_TIMER_HYP_IRQ         26u

/* -- SCTLR_EL1 bits ----------------------------------------- */
#define SCTLR_EL1_M    (1u <<  0)   /* MMU enable                */
#define SCTLR_EL1_A    (1u <<  1)   /* Alignment check           */
#define SCTLR_EL1_C    (1u <<  2)   /* D-cache enable            */
#define SCTLR_EL1_SA   (1u <<  3)   /* SP alignment EL1          */
#define SCTLR_EL1_I    (1u << 12)   /* I-cache enable            */
#define SCTLR_EL1_WXN  (1u << 19)   /* Write-XOR-execute         */

/* -- TCR_EL1 ------------------------------------------------- */
#define TCR_EL1_TG0_4K  (0u << 14)
#define TCR_EL1_TG1_4K  (2u << 30)
#define TCR_EL1_T0SZ48  (64u - 48u)
#define TCR_EL1_T1SZ48  ((cpu_u64_t)(64u - 48u) << 16)
#define TCR_EL1_IPS_48  ((cpu_u64_t)5u << 32)
#define TCR_EL1_IRGN0_WB_WA (1u << 8)
#define TCR_EL1_ORGN0_WB_WA (1u << 10)
#define TCR_EL1_SH0_IS  (3u << 12)

/* -- MAIR_EL1 attribute indices ----------------------------- */
#define MAIR_ATTR_NORMAL_CACHED  0xFF   /* Normal, WB WA cacheable */
#define MAIR_ATTR_NORMAL_NC      0x44   /* Normal, non-cacheable  */
#define MAIR_ATTR_DEVICE_nGnRE   0x04   /* Device nGnRE           */
#define MAIR_IDX_NORMAL          0u
#define MAIR_IDX_NORMAL_NC       1u
#define MAIR_IDX_DEVICE          2u

/* -- Cortex-A76 specific functions -------------------------- */
int  cortex_a76_init         (void);
void cortex_a76_enable_caches(void);
void cortex_a76_setup_mmu    (cpu_addr_t ttbr0, cpu_addr_t ttbr1);
void cortex_a76_errata_apply (void);
void cortex_a76_pmu_init     (void);
void cortex_a76_print_info   (void);

/* -- PMU event numbers -------------------------------------- */
#define A76_PMU_SW_INCR          0x00u
#define A76_PMU_L1I_CACHE_REFILL 0x01u
#define A76_PMU_L1I_TLB_REFILL   0x02u
#define A76_PMU_L1D_CACHE_REFILL 0x03u
#define A76_PMU_L1D_CACHE        0x04u
#define A76_PMU_L1D_TLB_REFILL   0x05u
#define A76_PMU_INST_RETIRED     0x08u
#define A76_PMU_EXC_TAKEN        0x09u
#define A76_PMU_CPU_CYCLES       0x11u
#define A76_PMU_BR_MIS_PRED      0x10u

#endif /* CORTEX_A76_H */
