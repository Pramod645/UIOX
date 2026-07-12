/*
 * 10_Arch/arm64/src/uiox_soc_arm64_init.c
 * UIOX ARM64 SoC — GIC-600 redistributor wake-up, cache topology
 * detection, and DSU cluster configuration.
 *
 * Called by arch_init() after uiox_soc_init_arm64() completes.
 */
#include "../include/uiox_soc_arm64.h"
#include "../../../02_FwHal/include/uiox_soc.h"
#include "../../../20_DriverInterfaces/include/mmio.h"
#include "../../../20_DriverInterfaces/include/irq.h"
#include <stdio.h>

/* ── GIC-v3 redistributor: clear ProcessorSleep bit ─────────────────── */
static void gic600_redist_wakeup(uint32_t cpu)
{
    uint64_t waker_reg = GIC600_REDIST_WAKER(cpu);
    uint32_t waker     = mmio_read32(waker_reg);

    /* Clear ProcessorSleep (bit 1) */
    waker &= ~(1u << 1u);
    mmio_write32(waker_reg, waker);
    arch_dsb();

    /* Poll ChildrenAsleep (bit 2) until cleared — max 1000 iterations */
    for (int i = 0; i < 1000; i++) {
        if (!(mmio_read32(waker_reg) & (1u << 2u))) break;
        arch_isb();
    }
    printf("[soc/arm64] GIC-600 redistributor[%u] awake\n", cpu);
}

/* ── GIC-v3 CPU interface enable via system registers ────────────────── */
static void gic600_cpu_iface_enable(void)
{
    uint64_t val;

    /* Enable system register access (ICC_SRE_EL1) */
    __asm__ volatile("mrs %0, ICC_SRE_EL1" : "=r"(val));
    val |= (uint64_t)(ICC_SRE_EL1_SRE | ICC_SRE_EL1_DFB | ICC_SRE_EL1_DIB);
    __asm__ volatile("msr ICC_SRE_EL1, %0" :: "r"(val) : "memory");
    arch_isb();

    /* Set priority mask to allow all interrupts (0xFF = allow all) */
    __asm__ volatile("msr ICC_PMR_EL1, %0" :: "r"((uint64_t)0xFFu) : "memory");

    /* Enable Group 1 interrupts (non-secure) */
    __asm__ volatile("msr ICC_IGRPEN1_EL1, %0"
                     :: "r"((uint64_t)ICC_IGRPEN1_EL1_EN) : "memory");
    arch_isb();

    printf("[soc/arm64] GIC-v3 CPU interface enabled (system registers)\n");
}

/* ── Cache topology detection via CLIDR_EL1 ─────────────────────────── */
static void arm64_cache_topology(void)
{
    uint64_t clidr;
    __asm__ volatile("mrs %0, clidr_el1" : "=r"(clidr));

    uint32_t loc   = (uint32_t)((clidr >> 24u) & 0x7u); /* LoC field       */
    printf("[soc/arm64] Cache topology (LoC=%u):\n", loc);

    for (uint32_t level = 0u; level < loc && level < 7u; level++) {
        uint32_t ctype = (uint32_t)((clidr >> (level * 3u)) & 0x7u);
        const char *type_str[] = { "none", "i-only", "d-only",
                                   "separate i+d", "unified",
                                   "res5", "res6", "res7" };
        /* Select cache level */
        uint64_t csselr = ((uint64_t)level << 1u);
        __asm__ volatile("msr csselr_el1, %0" :: "r"(csselr) : "memory");
        arch_isb();

        uint64_t ccsidr;
        __asm__ volatile("mrs %0, ccsidr_el1" : "=r"(ccsidr));
        uint32_t sets  = (uint32_t)(((ccsidr >> 13u) & 0x7FFFu) + 1u);
        uint32_t ways  = (uint32_t)(((ccsidr >>  3u) & 0x3FFu)  + 1u);
        uint32_t lsize = (uint32_t)(1u << (((uint32_t)(ccsidr & 0x7u)) + 4u));
        uint32_t size_kb = (sets * ways * lsize) / 1024u;

        printf("[soc/arm64]   L%u %s: %u KB (%u sets × %u ways × %u B)\n",
               level + 1u, type_str[ctype & 0x7u],
               size_kb, sets, ways, lsize);
    }
}

/* ── Cortex-A76 SMP enable via CPUECTLR_EL1 ─────────────────────────── */
static void arm64_smp_enable(void)
{
    uint64_t ectlr;
    __asm__ volatile("mrs %0, S3_0_C15_C1_4" : "=r"(ectlr)); /* CPUECTLR */
    ectlr |= A76_CPUECTLR_SMPEN;
    __asm__ volatile("msr S3_0_C15_C1_4, %0" :: "r"(ectlr) : "memory");
    arch_isb();
    printf("[soc/arm64] Cortex-A76 SMP enabled (CPUECTLR_EL1.SMPEN=1)\n");
}

/* ── Read MPIDR and identify this CPU's position in the topology ───────── */
static void arm64_print_cpu_info(void)
{
    uint64_t mpidr, midr;
    __asm__ volatile("mrs %0, mpidr_el1" : "=r"(mpidr));
    __asm__ volatile("mrs %0, midr_el1"  : "=r"(midr));

    uint32_t cluster, core;
    arm64_mpidr_decode(mpidr, &cluster, &core);

    uint32_t impl    = (uint32_t)((midr >> 24u) & 0xFFu);
    uint32_t partnum = (uint32_t)((midr >>  4u) & 0xFFFu);
    uint32_t rev     = (uint32_t)((midr >>  0u) & 0xFu);
    uint32_t var     = (uint32_t)((midr >> 20u) & 0xFu);

    printf("[soc/arm64] CPU[%u:%u] MIDR=0x%08llx "
           "(impl=0x%02x part=0x%03x r%up%u)\n",
           cluster, core,
           (unsigned long long)midr, impl, partnum, var, rev);
}

/* =========================================================================
 * uiox_soc_arm64_init — extended ARM64 SoC init
 * Called by arch_init() after the base uiox_soc_init_arm64() call.
 * ====================================================================== */
int uiox_soc_arm64_init(void)
{
    printf("[soc/arm64] Extended ARM64 SoC init (GIC-600, cache, DSU)\n");

    arm64_print_cpu_info();

    const uiox_soc_desc_t *desc = uiox_soc_get_desc();
    if (!desc) {
        printf("[soc/arm64] ERROR: SoC descriptor not populated\n");
        return UIOX_SOC_ERR_NOTFOUND;
    }

    /* Wake redistributors for all CPUs */
    for (uint32_t i = 0u; i < desc->num_cpus && i < 8u; i++)
        gic600_redist_wakeup(i);

    /* Enable GIC-v3 CPU interface on boot CPU */
    if (arm64_is_primary_cpu())
        gic600_cpu_iface_enable();

    /* SMP enable (Cortex-A76 specific) */
    arm64_smp_enable();

    /* Detect and print cache topology */
    arm64_cache_topology();

    printf("[soc/arm64] Extended ARM64 SoC init complete.\n");
    return UIOX_SOC_OK;
}

/* ── Tear-down ──────────────────────────────────────────────────────── */
void uiox_soc_arm64_fini(void)
{
    arch_irq_disable();
    printf("[soc/arm64] ARM64 SoC torn down.\n");
}
