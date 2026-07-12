/*
 * 10_Arch/riscv64/src/uiox_soc_riscv64_init.c
 * UIOX RISC-V 64-bit SoC — extended init: SiFive L2 cache,
 * CLINT multi-hart setup, PLIC priority programming, and
 * S-mode delegation configuration.
 *
 * Called by arch_init() after uiox_soc_init_riscv64() completes.
 */
#include "../include/uiox_soc_riscv64.h"
#include "../../../02_FwHal/include/uiox_soc.h"
#include "../../../20_DriverInterfaces/include/mmio.h"
#include <stdio.h>

/* ── Delegation: forward all standard exceptions to S-mode ──────────── */
static void rv_delegation_init(void)
{
    /*
     * medeleg: delegate these exceptions to S-mode:
     *   bit  0 — instruction address misaligned
     *   bit  1 — instruction access fault
     *   bit  2 — illegal instruction
     *   bit  3 — breakpoint
     *   bit  4 — load address misaligned
     *   bit  5 — load access fault
     *   bit  6 — store/AMO address misaligned
     *   bit  7 — store/AMO access fault
     *   bit  8 — environment call from U-mode
     *   bit 12 — instruction page fault
     *   bit 13 — load page fault
     *   bit 15 — store/AMO page fault
     */
    uint64_t medeleg = (1u <<  0) | (1u <<  1) | (1u <<  2) | (1u <<  3)
                     | (1u <<  4) | (1u <<  5) | (1u <<  6) | (1u <<  7)
                     | (1u <<  8) | (1u << 12) | (1u << 13) | (1u << 15);
    CSR_WRITE("medeleg", medeleg);

    /*
     * mideleg: delegate to S-mode:
     *   bit  1 — supervisor software interrupt
     *   bit  5 — supervisor timer interrupt
     *   bit  9 — supervisor external interrupt
     */
    uint64_t mideleg = (1u << 1) | (1u << 5) | (1u << 9);
    CSR_WRITE("mideleg", mideleg);

    printf("[soc/riscv] medeleg=0x%016llx  mideleg=0x%016llx\n",
           (unsigned long long)medeleg, (unsigned long long)mideleg);
}

/* ── SiFive L2 cache controller enable ───────────────────────────────── */
static void rv_l2_cache_init(void)
{
    uint32_t cfg = mmio_read32(SIFIVE_L2CC_CONFIG);
    if (cfg == 0u || cfg == 0xFFFFFFFFu) {
        printf("[soc/riscv] SiFive L2 cache controller not present\n");
        return;
    }

    uint32_t banks     = (cfg >>  0u) & 0xFFu;
    uint32_t ways      = (cfg >>  8u) & 0xFFu;
    uint32_t sets_log2 = (cfg >> 16u) & 0xFFu;
    uint32_t size_kb   = banks * ways * (1u << sets_log2) * 64u / 1024u;

    /* Enable all ways */
    mmio_write32(SIFIVE_L2CC_WAYS, (1u << ways) - 1u);
    arch_mb();

    printf("[soc/riscv] SiFive L2 cache: %u KB "
           "(%u banks × %u ways × %u-set)\n",
           size_kb, banks, ways, 1u << sets_log2);
}

/* ── CLINT multi-hart: clear MSIPs and initialise MTIMECMP ─────────── */
static void rv_clint_multi_hart_init(uint32_t num_harts)
{
    for (uint32_t h = 0u; h < num_harts && h < 8u; h++) {
        /* Clear software interrupt */
        mmio_write32(CLINT_MSIP(h), 0u);
        /* Set MTIMECMP far in future (no spurious timer on secondary harts) */
        mmio_write32(CLINT_MTIMECMP(h),       0xFFFFFFFFu);
        mmio_write32(CLINT_MTIMECMP(h) + 4u,  0xFFFFFFFFu);
    }
    printf("[soc/riscv] CLINT: %u hart(s) initialised\n", num_harts);
}

/* ── Print current CSR state ─────────────────────────────────────────── */
static void rv_print_csr_state(void)
{
    uint64_t misa     = CSR_READ("misa");
    uint64_t mstatus  = CSR_READ("mstatus");
    uint64_t mhartid  = CSR_READ("mhartid");

    /* Decode misa extension letters */
    char exts[27] = {0};
    uint32_t ei = 0u;
    for (int b = 0; b < 26; b++)
        if (misa & (1ull << b))
            exts[ei++] = (char)('A' + b);

    printf("[soc/riscv] mhartid=%llu  misa=0x%016llx (RV64%s)\n",
           (unsigned long long)mhartid,
           (unsigned long long)misa, exts);
    printf("[soc/riscv] mstatus=0x%016llx\n",
           (unsigned long long)mstatus);
}

/* =========================================================================
 * uiox_soc_riscv64_init — extended RISC-V 64 SoC init
 * ====================================================================== */
int uiox_soc_riscv64_init(void)
{
    printf("[soc/riscv] Extended RISC-V 64 SoC init "
           "(delegation, L2, CLINT multi-hart)\n");

    rv_print_csr_state();

    const uiox_soc_desc_t *desc = uiox_soc_get_desc();
    uint32_t num_harts = desc ? desc->num_cpus : 1u;

    rv_delegation_init();
    rv_clint_multi_hart_init(num_harts);
    rv_l2_cache_init();

    printf("[soc/riscv] Extended RISC-V 64 SoC init complete.\n");
    return UIOX_SOC_OK;
}

void uiox_soc_riscv64_fini(void)
{
    arch_irq_disable();
    printf("[soc/riscv] RISC-V 64 SoC torn down.\n");
}
