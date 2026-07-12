/*
 * 10_Arch/arm32/src/uiox_soc_arm32_init.c
 * UIOX ARM32 SoC — SCU enable, L2C-310 cache controller init,
 * Cortex-A9 private timer, and SMP ACTLR.SMP setup.
 */
#include "../include/uiox_soc_arm32.h"
#include "../../../02_FwHal/include/uiox_soc.h"
#include "../../../20_DriverInterfaces/include/mmio.h"
#include "../../../20_DriverInterfaces/include/irq.h"
#include <stdio.h>

/* ── SCU enable ──────────────────────────────────────────────────────── */
static void arm32_scu_enable(void)
{
    /* Invalidate all SCU tag RAMs before enabling */
    mmio_write32(SCU_INVALIDATE, 0xFFFFu);
    arch_dsb();

    uint32_t ctrl = mmio_read32(SCU_CTRL);
    ctrl |= SCU_CTRL_ENABLE;
    mmio_write32(SCU_CTRL, ctrl);
    arch_dsb();

    uint32_t cfg = mmio_read32(SCU_CFG);
    uint32_t ncpus = (cfg & 0x3u) + 1u;
    printf("[soc/arm32] SCU enabled: %u CPU(s) detected\n", ncpus);
}

/* ── L2C-310 PL310 — ARM L2 cache controller init ───────────────────── */
static void arm32_l2c310_init(void)
{
    /* Check presence: if base reads all-FF, no PL310 */
    uint32_t id = mmio_read32(L2C310_BASE + 0x000u);
    if (id == 0u || id == 0xFFFFFFFFu) {
        printf("[soc/arm32] L2C-310 not present — skipping\n");
        return;
    }

    /* Ensure L2 is disabled before configuring */
    if (mmio_read32(L2C310_CTRL) & L2C310_CTRL_ENABLE) {
        printf("[soc/arm32] L2C-310 already enabled\n");
        return;
    }

    /* Auxiliary control: 16-way associativity, 64KB way size,
     * early BRESP, instruction + data prefetch enabled */
    mmio_write32(L2C310_AUX_CTRL, 0x02060000u);

    /* Invalidate all ways before enable */
    mmio_write32(L2C310_INV_WAY, 0xFFFFu);
    /* Poll until operation complete */
    for (int i = 0; i < 100000 &&
         (mmio_read32(L2C310_INV_WAY) & 0xFFFFu); i++) {}

    /* Cache sync */
    mmio_write32(L2C310_CACHE_SYNC, 0u);
    arch_dsb();

    /* Enable */
    mmio_write32(L2C310_CTRL, L2C310_CTRL_ENABLE);
    arch_dsb();

    printf("[soc/arm32] L2C-310 enabled (id=0x%08x)\n", id);
}

/* ── Cortex-A9 private timer init at 100 Hz ──────────────────────────── */
static void arm32_ptimer_init(uint32_t hz)
{
    /* Private timer prescaler = 0 (no prescale), use periph clock / 1 */
    uint32_t periph_clk = 400000000u;  /* 400 MHz typical on Cortex-A9    */
    uint32_t load_val   = (periph_clk / hz) - 1u;

    mmio_write32(PTIMER_CTRL,    0u);           /* disable first           */
    mmio_write32(PTIMER_LOAD,    load_val);
    mmio_write32(PTIMER_INT_STATUS, 1u);        /* clear any pending       */
    mmio_write32(PTIMER_CTRL,
                 PTIMER_CTRL_ENABLE |
                 PTIMER_CTRL_AUTO_RELOAD |
                 PTIMER_CTRL_IRQ_EN);

    printf("[soc/arm32] Cortex-A9 private timer: %u Hz "
           "(load=0x%08x IRQ=%d)\n", hz, load_val, PTIMER_IRQ);
}

/* ── SMP: set ACTLR.SMP so coherent cache maintenance works ─────────── */
static void arm32_smp_enable(void)
{
    uint32_t actlr = ARM32_READ_ACTLR();
    if (!(actlr & ACTLR_SMP_BIT)) {
        actlr |= ACTLR_SMP_BIT;
        ARM32_WRITE_ACTLR(actlr);
        arch_isb();
        arch_dsb();
    }
    printf("[soc/arm32] ACTLR.SMP enabled (0x%08x)\n", ARM32_READ_ACTLR());
}

/* ── MIDR / MPIDR identify ───────────────────────────────────────────── */
static void arm32_print_cpu_info(void)
{
    uint32_t midr  = ARM32_READ_MIDR();
    uint32_t mpidr = ARM32_READ_MPIDR();
    uint32_t part  = (midr >>  4u) & 0xFFFu;
    uint32_t rev   = (midr >>  0u) & 0xFu;
    uint32_t var   = (midr >> 20u) & 0xFu;
    uint32_t cpu   =  mpidr        & 0x3u;
    uint32_t clust = (mpidr >> 8u) & 0xFu;

    printf("[soc/arm32] CPU[%u:%u] MIDR=0x%08x (part=0x%03x r%up%u)\n",
           clust, cpu, midr, part, var, rev);
}

/* =========================================================================
 * uiox_soc_arm32_init — extended ARM32 SoC init
 * ====================================================================== */
int uiox_soc_arm32_init(void)
{
    printf("[soc/arm32] Extended ARM32 SoC init (SCU, L2C, private timer)\n");

    arm32_print_cpu_info();
    arm32_smp_enable();
    arm32_scu_enable();
    arm32_l2c310_init();
    arm32_ptimer_init(100u);

    printf("[soc/arm32] Extended ARM32 SoC init complete.\n");
    return UIOX_SOC_OK;
}

void uiox_soc_arm32_fini(void)
{
    /* Disable private timer */
    mmio_write32(PTIMER_CTRL, 0u);
    cpu_irq_disable();
    printf("[soc/arm32] ARM32 SoC torn down.\n");
}
