/*
 * cortex_a76.c - ARM Cortex-A76 specific initialisation
 */
#include "../../include/arch/cortex_a76.h"
#include "../../include/cpu_regs.h"
#include "../../include/cpu_cache.h"
#include <stdio.h>

int cortex_a76_init(void)
{
    cortex_a76_errata_apply();
    cortex_a76_enable_caches();
    cortex_a76_pmu_init();
    printf("[a76] Cortex-A76 initialised\n");
    return CPU_OK;
}

void cortex_a76_enable_caches(void)
{
    /* flush caches before enabling */
    cpu_cache_flush_all();

    /* SCTLR_EL1: enable M + C + I */
    cpu_u64_t sctlr;
    CPU_MRS(SCTLR_EL1, sctlr);
    sctlr |= SCTLR_EL1_M | SCTLR_EL1_C | SCTLR_EL1_I;
    sctlr &= ~SCTLR_EL1_WXN;
    cpu_dsb();
    CPU_MSR(SCTLR_EL1, sctlr);
    cpu_isb();
}

void cortex_a76_setup_mmu(cpu_addr_t ttbr0, cpu_addr_t ttbr1)
{
    /* MAIR_EL1: normal cached | normal NC | device nGnRE */
    cpu_u64_t mair =
        ((cpu_u64_t)MAIR_ATTR_NORMAL_CACHED << (MAIR_IDX_NORMAL   * 8)) |
        ((cpu_u64_t)MAIR_ATTR_NORMAL_NC     << (MAIR_IDX_NORMAL_NC* 8)) |
        ((cpu_u64_t)MAIR_ATTR_DEVICE_nGnRE  << (MAIR_IDX_DEVICE   * 8));
    CPU_MSR(MAIR_EL1, mair);

    /* TCR_EL1: 48-bit VA, 4KB pages, inner/outer WB WA */
    cpu_u64_t tcr = TCR_EL1_T0SZ48     |
                    TCR_EL1_T1SZ48     |
                    TCR_EL1_TG0_4K     |
                    TCR_EL1_TG1_4K     |
                    TCR_EL1_IRGN0_WB_WA|
                    TCR_EL1_ORGN0_WB_WA|
                    TCR_EL1_SH0_IS     |
                    TCR_EL1_IPS_48;
    CPU_MSR(TCR_EL1, tcr);
    cpu_isb();

    CPU_MSR(TTBR0_EL1, ttbr0);
    CPU_MSR(TTBR1_EL1, ttbr1);
    cpu_isb();
}

void cortex_a76_errata_apply(void)
{
    /* Cortex-A76 erratum 1463225: DSB + ISB before ERET */
    /* Applied automatically by compiler barrier sequences */
    cpu_dsb();
    cpu_isb();
}

void cortex_a76_pmu_init(void)
{
    /* PMCR_EL0: enable all counters, reset */
    cpu_u64_t pmcr = (1u << 0) | (1u << 1) | (1u << 2);
    CPU_MSR(PMCR_EL0, pmcr);
    /* PMCNTENSET_EL0: enable cycle counter */
    CPU_MSR(PMCNTENSET_EL0, (1u << 31));
    cpu_isb();
}

void cortex_a76_print_info(void)
{
    cpu_u64_t midr;
    CPU_MRS(MIDR_EL1, midr);
    printf("[a76] MIDR=0x%016llx  PartNum=0x%03X  Rev=%u  Var=%u\n",
           (unsigned long long)midr,
           (cpu_u32_t)((midr >> 4)  & 0xFFF),
           (cpu_u32_t)(midr & 0xF),
           (cpu_u32_t)((midr >> 20) & 0xF));
}
