/*
 * 02_FwHal/src/uiox_soc_riscv64.c
 * UIOX SoC abstraction — RISC-V 64 backend.
 *
 * Initialises CLINT, PLIC, NS16550A UART, and SBI time extension.
 * Targets QEMU virt and SiFive U74 (HiFive Unmatched).
 */
#include "../include/uiox_soc.h"
#include "../../10_Arch/riscv64/include/arch_defs.h"
#include "../../20_DriverInterfaces/include/hw_types.h"
#include "../../20_DriverInterfaces/include/mmio.h"
#include "../../20_DriverInterfaces/include/irq.h"
#include <stdio.h>
#include <string.h>

/* ── CSR helpers ─────────────────────────────────────────────────────── */
#define rv_csrr(reg)   ({ uint64_t _v; \
    __asm__ volatile("csrr %0, " #reg : "=r"(_v)); _v; })
#define rv_csrw(reg,v) \
    __asm__ volatile("csrw " #reg ", %0" :: "r"((uint64_t)(v)) : "memory")

/* ── CLINT init — clear pending MSIs, program initial MTIMECMP ────────── */
static void rv_clint_init(uint32_t hart_id)
{
    /* Clear software interrupt for this hart */
    mmio_write32(SOC_CLINT_MSIP(hart_id), 0u);

    /* Set MTIMECMP far in the future so no immediate timer fires */
    mmio_write32(SOC_CLINT_MTIMECMP(hart_id),     0xFFFFFFFFu);
    mmio_write32(SOC_CLINT_MTIMECMP(hart_id) + 4u, 0xFFFFFFFFu);

    printf("[soc/riscv] CLINT @ 0x%08lx  hart=%u  MTIMECMP cleared\n",
           (unsigned long)SOC_CLINT_BASE, hart_id);
}

/* ── PLIC init — priority and enable for UART IRQ ─────────────────────── */
static void rv_plic_init(void)
{
    /* Set UART priority = 1 (above threshold 0) */
    mmio_write32(SOC_PLIC_PRIORITY(SOC_UART_IRQ), 1u);

    /* Enable UART IRQ for hart 0 in supervisor (context 1) */
    uint32_t ctx = 1u;   /* hart0 S-mode context */
    uint32_t word   = SOC_UART_IRQ / 32u;
    uint32_t bit    = SOC_UART_IRQ % 32u;
    uint32_t en_reg = mmio_read32(SOC_PLIC_ENABLE(ctx) + word * 4u);
    mmio_write32(SOC_PLIC_ENABLE(ctx) + word * 4u, en_reg | (1u << bit));

    /* Set threshold = 0 (allow all priorities) */
    mmio_write32(SOC_PLIC_THRESHOLD(ctx), 0u);

    printf("[soc/riscv] PLIC @ 0x%08lx  UART_IRQ=%u enabled (ctx=%u)\n",
           (unsigned long)SOC_PLIC_BASE, SOC_UART_IRQ, ctx);
}

/* ── NS16550A UART init ──────────────────────────────────────────────── */
static void rv_uart_init(void)
{
    /* NS16550A register offsets (8-bit stride) */
    mmio_write32(SOC_UART0_BASE + 0x04u, 0x00u);  /* IER: all disabled    */
    mmio_write32(SOC_UART0_BASE + 0x03u, 0x83u);  /* LCR: 8N1 + DLAB     */
    mmio_write32(SOC_UART0_BASE + 0x00u, 0x01u);  /* DLL: divisor lo      */
    mmio_write32(SOC_UART0_BASE + 0x01u, 0x00u);  /* DLM: divisor hi      */
    mmio_write32(SOC_UART0_BASE + 0x03u, 0x03u);  /* LCR: 8N1, DLAB off  */
    mmio_write32(SOC_UART0_BASE + 0x02u, 0xC7u);  /* FCR: FIFO enable     */
    mmio_write32(SOC_UART0_BASE + 0x04u, 0x01u);  /* IER: RX interrupt    */
    printf("[soc/riscv] NS16550A UART @ 0x%08lx  115200 8N1\n",
           (unsigned long)SOC_UART0_BASE);
}

/* ── SBI probe ───────────────────────────────────────────────────────── */
static bool rv_sbi_probe_ext(uint64_t eid)
{
    register uint64_t a0 __asm__("a0") = eid;
    register uint64_t a6 __asm__("a6") = 0u;
    register uint64_t a7 __asm__("a7") = 0x10u; /* Base extension */
    __asm__ volatile("ecall" : "+r"(a0) : "r"(a6), "r"(a7) : "memory");
    return (a0 == 0u);
}

/* ── Identify from misa / mvendorid ─────────────────────────────────── */
static void rv_identify(uiox_soc_desc_t *desc)
{
    uint64_t misa      = rv_csrr(misa);
    uint64_t mvendorid = rv_csrr(mvendorid);
    uint64_t marchid   = rv_csrr(marchid);
    (void)marchid;

    /* SiFive vendor ID = 0x489 */
    if ((mvendorid & 0xFFFu) == 0x489u) {
        desc->soc_id = UIOX_SOC_SIFIVE_U74;
        strncpy(desc->name, "SiFive U74 (HiFive Unmatched)",
                UIOX_SOC_NAME_LEN - 1);
        desc->num_cpus     = 4u;
        desc->cpu_freq_khz = 1200000u;
        desc->l1_icache_kb = 32u;
        desc->l1_dcache_kb = 32u;
        desc->l2_cache_kb  = 2048u;
    } else {
        desc->soc_id = UIOX_SOC_QEMU_VIRT_RV64;
        strncpy(desc->name, "QEMU virt (rv64)", UIOX_SOC_NAME_LEN - 1);
        desc->num_cpus     = 1u;
        desc->cpu_freq_khz = 1000000u;
        desc->l1_icache_kb = 32u;
        desc->l1_dcache_kb = 32u;
        desc->l2_cache_kb  = 0u;
    }

    desc->dram_base    = SOC_DRAM_BASE;
    desc->dram_size    = SOC_DRAM_SIZE;
    desc->num_clusters = 1u;

    desc->capabilities = UIOX_SOC_CAP_MMU    | UIOX_SOC_CAP_CACHE_L1
                       | UIOX_SOC_CAP_PLIC   | UIOX_SOC_CAP_UART
                       | UIOX_SOC_CAP_DTB;

    if (rv_sbi_probe_ext(0x54494D45u))      /* TIME */
        desc->capabilities |= UIOX_SOC_CAP_SBI;
    if (desc->l2_cache_kb > 0u)
        desc->capabilities |= UIOX_SOC_CAP_CACHE_L2;
    if (desc->num_cpus > 1u)
        desc->capabilities |= UIOX_SOC_CAP_SMP;

    /* Check M/A/F/D extensions from misa */
    if (misa & (1u << ('M' - 'A'))) { /* Multiply */ }
    if (misa & (1u << ('A' - 'A'))) { /* Atomics  */ }
}

/* =========================================================================
 * uiox_soc_init_riscv64 — called from uiox_soc_init()
 * ====================================================================== */
int uiox_soc_init_riscv64(uiox_soc_desc_t *desc)
{
    if (!desc) return UIOX_SOC_ERR_INVAL;

    printf("[soc/riscv] Initialising RISC-V 64 SoC layer...\n");

    rv_identify(desc);
    printf("[soc/riscv] SoC: %s  CPUs=%u  DRAM=0x%llx+%llu MB\n",
           desc->name,
           desc->num_cpus,
           (unsigned long long)desc->dram_base,
           (unsigned long long)(desc->dram_size >> 20));

    uiox_clk_ctx_t *clk = uiox_soc_get_clk();
    uiox_clk_init(clk, desc);

    rv_clint_init(0u);
    rv_plic_init();
    rv_uart_init();

    uiox_pm_ctx_t *pm = uiox_soc_get_pm();
    uiox_pm_init(pm, desc);

    /* Enable S-mode external interrupts (PLIC) */
    uint64_t sie = rv_csrr(sie);
    rv_csrw(sie, sie | (1u << 9));  /* SEIE bit */

    desc->initialized = true;
    printf("[soc/riscv] RISC-V 64 SoC layer ready.\n");
    return UIOX_SOC_OK;
}
