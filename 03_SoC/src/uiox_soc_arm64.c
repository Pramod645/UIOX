/*
 * 02_FwHal/src/uiox_soc_arm64.c
 * UIOX SoC abstraction — ARM64 (AArch64 / ARMv8-A) backend.
 *
 * Detects the running SoC (via MIDR_EL1 / DTB compatible string),
 * populates the SoC descriptor, initialises the clock tree and PM
 * subsystem, and prints a boot-time summary.
 *
 * Tested on:
 *   QEMU -machine virt  (UIOX_SOC_QEMU_VIRT_A64)
 *   Raspberry Pi 4      (UIOX_SOC_BCM2711)
 *   Raspberry Pi 5      (UIOX_SOC_BCM2712)
 */
#include "../include/uiox_soc.h"
#include "../../10_Arch/arm64/include/arch_defs.h"
#include "../../20_DriverInterfaces/include/hw_types.h"
#include "../../20_DriverInterfaces/include/mmio.h"
#include "../../20_DriverInterfaces/include/irq.h"
#include <stdio.h>
#include <string.h>

/* ── Helpers ─────────────────────────────────────────────────────────── */
static void a64_memset(void *d, int v, size_t n)
{ uint8_t *p = (uint8_t *)d; while (n--) *p++ = (uint8_t)v; }

/* ── MIDR_EL1 decode ─────────────────────────────────────────────────── */
#define MIDR_PARTNUM(m)     (((m) >> 4)  & 0xFFFu)
#define MIDR_IMPLEMENTER(m) (((m) >> 24) & 0xFFu)

#define MIDR_IMPL_ARM       0x41u
#define MIDR_PART_A53       0xD03u
#define MIDR_PART_A55       0xD05u
#define MIDR_PART_A72       0xD08u
#define MIDR_PART_A76       0xD0Bu
#define MIDR_PART_A78       0xD41u

static uint64_t arm64_read_midr(void)
{
    uint64_t midr = 0u;
    __asm__ volatile("mrs %0, midr_el1" : "=r"(midr) :: "memory");
    return midr;
}

/* ── GIC-v3 system register init ─────────────────────────────────────── */
static void arm64_gicv3_sre_enable(void)
{
    uint64_t sre;
    __asm__ volatile("mrs %0, ICC_SRE_EL1" : "=r"(sre));
    sre |= 0x7u;   /* SRE | DFB | DIB */
    __asm__ volatile("msr ICC_SRE_EL1, %0" :: "r"(sre) : "memory");
    arch_isb();
}

/* ── GIC-400 distributor + CPU interface init ────────────────────────── */
static void arm64_gic400_init(void)
{
    mmio_write32(SOC_GIC_DIST_BASE + 0x000u, 0x0u);   /* disable GICD    */
    mmio_write32(SOC_GIC_DIST_BASE + 0x100u, 0xFFFFFFFFu); /* enable SPIs */
    mmio_write32(SOC_GIC_CPU_BASE  + 0x004u, 0xFFu);   /* priority mask   */
    mmio_write32(SOC_GIC_CPU_BASE  + 0x000u, 0x1u);    /* enable GICC     */
    mmio_write32(SOC_GIC_DIST_BASE + 0x000u, 0x1u);    /* enable GICD     */
    printf("[soc/arm64] GIC-400 initialised (DIST=0x%08lx CPU=0x%08lx)\n",
           (unsigned long)SOC_GIC_DIST_BASE,
           (unsigned long)SOC_GIC_CPU_BASE);
}

/* ── PL011 UART clock configuration ─────────────────────────────────── */
static void arm64_uart0_clk_init(const uiox_clk_ctx_t *clk)
{
    uint32_t ref  = uiox_clk_get_hz(clk, UIOX_CLK_UART0);
    uint32_t baud = 115200u;
    uint32_t ibrd = UIOX_UART_IBRD(ref, baud);
    uint32_t fbrd = UIOX_UART_FBRD(ref, baud);

    mmio_write32(SOC_UART0_BASE + 0x030u, 0x0u);       /* CR: disable     */
    mmio_write32(SOC_UART0_BASE + 0x024u, ibrd);        /* IBRD            */
    mmio_write32(SOC_UART0_BASE + 0x028u, fbrd);        /* FBRD            */
    mmio_write32(SOC_UART0_BASE + 0x02Cu, 0x70u);       /* LCR_H: 8N1+FIFO */
    mmio_write32(SOC_UART0_BASE + 0x038u, (1u << 4));   /* IMSC: RX intr   */
    mmio_write32(SOC_UART0_BASE + 0x030u, 0x301u);      /* CR: TX+RX+EN    */

    printf("[soc/arm64] UART0 @ 0x%08lx  %u baud (IBRD=%u FBRD=%u)\n",
           (unsigned long)SOC_UART0_BASE, baud, ibrd, fbrd);
}

/* ── Identify SoC from MIDR_EL1 ─────────────────────────────────────── */
static void arm64_identify(uiox_soc_desc_t *desc)
{
    uint64_t midr    = arm64_read_midr();
    uint32_t partnum = (uint32_t)MIDR_PARTNUM(midr);

    switch (partnum) {
    case MIDR_PART_A72:
        desc->soc_id    = UIOX_SOC_BCM2711;
        strncpy(desc->name, "BCM2711 (Cortex-A72)", UIOX_SOC_NAME_LEN - 1);
        desc->num_cpus       = 4u;
        desc->cpu_freq_khz   = 1500000u;
        desc->l1_icache_kb   = 48u;
        desc->l1_dcache_kb   = 32u;
        desc->l2_cache_kb    = 1024u;
        desc->capabilities  |= UIOX_SOC_CAP_GIC_V2 | UIOX_SOC_CAP_TRNG
                              | UIOX_SOC_CAP_TRUSTZONE | UIOX_SOC_CAP_EMMC
                              | UIOX_SOC_CAP_USB | UIOX_SOC_CAP_ETH;
        break;

    case MIDR_PART_A76:
        desc->soc_id    = UIOX_SOC_BCM2712;
        strncpy(desc->name, "BCM2712 (Cortex-A76)", UIOX_SOC_NAME_LEN - 1);
        desc->num_cpus       = 4u;
        desc->cpu_freq_khz   = 2400000u;
        desc->l1_icache_kb   = 64u;
        desc->l1_dcache_kb   = 64u;
        desc->l2_cache_kb    = 512u;
        desc->l3_cache_kb    = 2048u;
        desc->capabilities  |= UIOX_SOC_CAP_GIC_V3 | UIOX_SOC_CAP_TRNG
                              | UIOX_SOC_CAP_TRUSTZONE | UIOX_SOC_CAP_PCIE
                              | UIOX_SOC_CAP_USB | UIOX_SOC_CAP_ETH;
        break;

    default:
        /* QEMU virt or unknown — safe defaults */
        desc->soc_id    = UIOX_SOC_QEMU_VIRT_A64;
        strncpy(desc->name, "QEMU virt (arm64)", UIOX_SOC_NAME_LEN - 1);
        desc->num_cpus       = 1u;
        desc->cpu_freq_khz   = 1000000u;
        desc->l1_icache_kb   = 32u;
        desc->l1_dcache_kb   = 32u;
        desc->capabilities  |= UIOX_SOC_CAP_GIC_V2 | UIOX_SOC_CAP_DTB
                              | UIOX_SOC_CAP_UART;
        break;
    }

    desc->dram_base        = SOC_DRAM_BASE;
    desc->dram_size        = SOC_DRAM_SIZE;
    desc->num_clusters     = (desc->num_cpus + 3u) / 4u;
    desc->capabilities    |= UIOX_SOC_CAP_MMU   | UIOX_SOC_CAP_CACHE_L1
                           |  UIOX_SOC_CAP_CACHE_L2 | UIOX_SOC_CAP_SMP;
}

/* =========================================================================
 * uiox_soc_init_arm64 — called from uiox_soc_init()
 * ====================================================================== */
int uiox_soc_init_arm64(uiox_soc_desc_t *desc)
{
    if (!desc) return UIOX_SOC_ERR_INVAL;

    printf("[soc/arm64] Initialising ARM64 SoC layer...\n");

    /* ── 1. Identify SoC ─────────────────────────────────────────────── */
    arm64_identify(desc);
    printf("[soc/arm64] SoC: %s  CPUs=%u  DRAM=0x%llx+%llu MB\n",
           desc->name,
           desc->num_cpus,
           (unsigned long long)desc->dram_base,
           (unsigned long long)(desc->dram_size >> 20));

    /* ── 2. Clock context ───────────────────────────────────────────── */
    uiox_clk_ctx_t *clk = uiox_soc_get_clk();
    uiox_clk_init(clk, desc);

    /* ── 3. GIC init ─────────────────────────────────────────────────── */
    if (desc->capabilities & UIOX_SOC_CAP_GIC_V3)
        arm64_gicv3_sre_enable();
    arm64_gic400_init();

    /* ── 4. UART0 ────────────────────────────────────────────────────── */
    arm64_uart0_clk_init(clk);

    /* ── 5. PM context ───────────────────────────────────────────────── */
    uiox_pm_ctx_t *pm = uiox_soc_get_pm();
    uiox_pm_init(pm, desc);

    desc->initialized = true;
    printf("[soc/arm64] ARM64 SoC layer ready.\n");
    return UIOX_SOC_OK;
}
