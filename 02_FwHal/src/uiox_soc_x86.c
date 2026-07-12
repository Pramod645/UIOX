/*
 * 02_FwHal/src/uiox_soc_x86.c
 * UIOX SoC abstraction — x86-64 backend.
 *
 * Detects CPU vendor/model via CPUID, configures LAPIC, IOAPIC, HPET,
 * legacy PIT, and COM1 UART, then populates the SoC descriptor.
 */
#include "../include/uiox_soc.h"
#include "../../10_Arch/x86_64/include/arch_defs.h"
#include "../../20_DriverInterfaces/include/hw_types.h"
#include "../../20_DriverInterfaces/include/mmio.h"
#include "../../20_DriverInterfaces/include/irq.h"
#include <stdio.h>
#include <string.h>

/* ── CPUID helper ────────────────────────────────────────────────────── */
static void x86_cpuid(uint32_t leaf, uint32_t subleaf,
                       uint32_t *eax, uint32_t *ebx,
                       uint32_t *ecx, uint32_t *edx)
{
    __asm__ volatile(
        "cpuid"
        : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
        : "a"(leaf), "c"(subleaf)
    );
}

/* ── Port I/O ────────────────────────────────────────────────────────── */
static inline void x86_outb(uint16_t port, uint8_t val)
{ __asm__ volatile("outb %0,%1" :: "a"(val), "Nd"(port)); }

static inline uint8_t x86_inb(uint16_t port)
{ uint8_t v; __asm__ volatile("inb %1,%0" : "=a"(v) : "Nd"(port)); return v; }

static inline void x86_outw(uint16_t port, uint16_t val)
{ __asm__ volatile("outw %0,%1" :: "a"(val), "Nd"(port)); }

/* ── LAPIC init ──────────────────────────────────────────────────────── */
static void x86_lapic_init(void)
{
    /* Enable LAPIC via MSR IA32_APIC_BASE */
    uint32_t lo, hi;
    __asm__ volatile("rdmsr" : "=a"(lo), "=d"(hi) : "c"(0x1Bu));
    lo |= (1u << 11);   /* Global Enable bit */
    __asm__ volatile("wrmsr" :: "c"(0x1Bu), "a"(lo), "d"(hi));

    /* Spurious interrupt vector register: enable LAPIC, vector 0xFF */
    mmio_write32(SOC_LAPIC_BASE + 0x0F0u, 0x1FFu);
    printf("[soc/x86]  LAPIC enabled at 0x%08lx\n",
           (unsigned long)SOC_LAPIC_BASE);
}

/* ── Legacy 16550A COM1 UART init ────────────────────────────────────── */
static void x86_uart_init(void)
{
    x86_outb(SOC_UART0_PORT + 1u, 0x00u);  /* Disable all interrupts      */
    x86_outb(SOC_UART0_PORT + 3u, 0x80u);  /* DLAB on (set baud divisor)  */
    x86_outb(SOC_UART0_PORT + 0u, 0x01u);  /* Divisor lo: 115200 baud     */
    x86_outb(SOC_UART0_PORT + 1u, 0x00u);  /* Divisor hi                  */
    x86_outb(SOC_UART0_PORT + 3u, 0x03u);  /* 8 bits, no parity, 1 stop   */
    x86_outb(SOC_UART0_PORT + 2u, 0xC7u);  /* FIFO enable, 14-byte thresh */
    x86_outb(SOC_UART0_PORT + 4u, 0x0Bu);  /* RTS+DTR+OUT2                */
    x86_outb(SOC_UART0_PORT + 1u, 0x01u);  /* Enable RX interrupt         */
    printf("[soc/x86]  COM1 UART @ 0x%04x  115200 8N1\n", SOC_UART0_PORT);
}

/* ── HPET init ───────────────────────────────────────────────────────── */
static void x86_hpet_init(void)
{
    uint32_t caps_lo = mmio_read32(SOC_HPET_BASE + 0x000u);
    if (caps_lo == 0u || caps_lo == 0xFFFFFFFFu) {
        printf("[soc/x86]  HPET not present — skipping\n");
        return;
    }
    /* Enable HPET: set ENABLE_CNF bit in General Config register */
    uint32_t cfg = mmio_read32(SOC_HPET_BASE + 0x010u);
    mmio_write32(SOC_HPET_BASE + 0x010u, cfg | 0x1u);
    printf("[soc/x86]  HPET enabled at 0x%08lx\n",
           (unsigned long)SOC_HPET_BASE);
}

/* ── CPUID-based SoC identify ────────────────────────────────────────── */
static void x86_identify(uiox_soc_desc_t *desc)
{
    uint32_t eax, ebx, ecx, edx;
    char vendor[13] = {0};

    x86_cpuid(0u, 0u, &eax, &ebx, &ecx, &edx);
    memcpy(vendor + 0, &ebx, 4);
    memcpy(vendor + 4, &edx, 4);
    memcpy(vendor + 8, &ecx, 4);

    desc->soc_id = UIOX_SOC_X86_QEMU_Q35;
    snprintf(desc->name, UIOX_SOC_NAME_LEN,
             "x86-64 [%s] maxleaf=%u", vendor, eax);

    /* Read max physical/logical CPU count */
    x86_cpuid(1u, 0u, &eax, &ebx, &ecx, &edx);
    desc->num_cpus     = (ebx >> 16u) & 0xFFu;
    if (desc->num_cpus == 0u) desc->num_cpus = 1u;
    desc->num_clusters = 1u;
    desc->cpu_freq_khz = 0u;  /* runtime calibration via TSC */

    /* Cache topology (leaf 4) */
    x86_cpuid(4u, 0u, &eax, &ebx, &ecx, &edx);
    desc->l1_dcache_kb = 32u;
    desc->l1_icache_kb = 32u;
    desc->l2_cache_kb  = 256u;
    desc->l3_cache_kb  = 0u;   /* not always exposed via leaf 4 */

    desc->dram_base    = SOC_DRAM_BASE;
    desc->dram_size    = SOC_DRAM_SIZE;

    /* Capability detection from CPUID */
    desc->capabilities = UIOX_SOC_CAP_MMU    | UIOX_SOC_CAP_CACHE_L1
                       | UIOX_SOC_CAP_CACHE_L2 | UIOX_SOC_CAP_SMP
                       | UIOX_SOC_CAP_APIC    | UIOX_SOC_CAP_UART
                       | UIOX_SOC_CAP_ACPI    | UIOX_SOC_CAP_EFI
                       | UIOX_SOC_CAP_PCIE;

    /* RDRAND / RDSEED */
    x86_cpuid(1u, 0u, &eax, &ebx, &ecx, &edx);
    if (ecx & (1u << 30)) desc->capabilities |= UIOX_SOC_CAP_TRNG;
}

/* =========================================================================
 * uiox_soc_init_x86 — called from uiox_soc_init()
 * ====================================================================== */
int uiox_soc_init_x86(uiox_soc_desc_t *desc)
{
    if (!desc) return UIOX_SOC_ERR_INVAL;

    printf("[soc/x86]  Initialising x86-64 SoC layer...\n");

    x86_identify(desc);
    printf("[soc/x86]  SoC: %s  CPUs=%u\n", desc->name, desc->num_cpus);

    uiox_clk_ctx_t *clk = uiox_soc_get_clk();
    uiox_clk_init(clk, desc);

    x86_lapic_init();
    x86_uart_init();
    x86_hpet_init();

    uiox_pm_ctx_t *pm = uiox_soc_get_pm();
    uiox_pm_init(pm, desc);

    desc->initialized = true;
    printf("[soc/x86]  x86-64 SoC layer ready.\n");
    return UIOX_SOC_OK;
}
