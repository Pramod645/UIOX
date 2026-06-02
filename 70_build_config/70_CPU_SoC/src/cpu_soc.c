/*
 * cpu_soc.c - SoC-level initialisation pipeline
 */
#include "../include/cpu_soc.h"
#include "../include/arch/cortex_a76.h"
#include "../include/arch/x86_64_cpu.h"
#include "../include/arch/riscv64.h"
#include "../include/drivers/cpu_drv_gic.h"
#include "../include/drivers/cpu_drv_apic.h"
#include "../include/drivers/cpu_drv_plic.h"
#include "../include/drivers/cpu_drv_timer.h"
#include "../include/drivers/cpu_drv_uart.h"
#include <stdio.h>

const cpu_soc_desc_t *g_soc = NULL;

/* -- Well-known SoC descriptors ----------------------------- */
const cpu_soc_desc_t soc_cortex_a76 = {
    .name          = "ARM Cortex-A76 (QEMU virt)",
    .arch          = CPU_ARCH_ARM_CORTEX_A76,
    .num_cores     = 4,
    .num_clusters  = 1,
    .dram_base     = 0x0000000040000000ULL,
    .dram_size     = 0x0000000004000000ULL,
    .mmio_base     = 0x0000000000000000ULL,
    .mmio_size     = 0x0000000040000000ULL,
    .gic_dist_base = 0x08000000u,
    .gic_cpu_base  = 0x08010000u,
    .plic_base     = 0u,
    .lapic_base    = 0u,
    .uart_base     = 0x09000000u,
    .uart_irq      = 33u,
    .timer_irq     = 27u,
};

const cpu_soc_desc_t soc_x86_64_generic = {
    .name          = "x86-64 Generic (QEMU q35)",
    .arch          = CPU_ARCH_X86_64,
    .num_cores     = 4,
    .num_clusters  = 1,
    .dram_base     = 0x0000000000100000ULL,
    .dram_size     = 0x0000000004000000ULL,
    .mmio_base     = 0x00000000FEC00000ULL,
    .mmio_size     = 0x0000000000400000ULL,
    .gic_dist_base = 0u,
    .gic_cpu_base  = 0u,
    .plic_base     = 0u,
    .lapic_base    = 0xFEE00000u,
    .uart_base     = 0x3F8u,
    .uart_irq      = 4u,
    .timer_irq     = 0x20u,
};

const cpu_soc_desc_t soc_riscv64_generic = {
    .name          = "RISC-V RV64GC (QEMU virt)",
    .arch          = CPU_ARCH_RISCV64,
    .num_cores     = 4,
    .num_clusters  = 1,
    .dram_base     = 0x0000000080000000ULL,
    .dram_size     = 0x0000000004000000ULL,
    .mmio_base     = 0x0000000000000000ULL,
    .mmio_size     = 0x0000000080000000ULL,
    .gic_dist_base = 0u,
    .gic_cpu_base  = 0u,
    .plic_base     = 0x0C000000u,
    .lapic_base    = 0u,
    .uart_base     = 0x10000000u,
    .uart_irq      = 10u,
    .timer_irq     = 5u,
};

/* -- Early init (called before MMU / caches on) ------------- */
int cpu_soc_early_init(void)
{
    if (!g_soc) return CPU_ERR;

#if defined(UIOX_ARCH_ARM64)
    /* early UART for debug output */
    cpu_uart_init(UART_PL011, g_soc->uart_base, 115200u,
                   24000000u);
    cpu_uart_puts("[soc] early init: ARM Cortex-A76\n");

#elif defined(UIOX_ARCH_X86_64)
    cpu_uart_init(UART_16550, g_soc->uart_base, 115200u,
                   1843200u);
    cpu_uart_puts("[soc] early init: x86-64\n");
    x86_gdt_init();
    x86_idt_init();

#elif defined(UIOX_ARCH_RISCV64)
    cpu_uart_init(UART_16550, g_soc->uart_base, 115200u,
                   1843200u);
    cpu_uart_puts("[soc] early init: RISC-V RV64GC\n");
    riscv64_delegate_traps();
#endif
    return CPU_OK;
}

/* -- Late init (called after MMU / caches on) --------------- */
int cpu_soc_late_init(void)
{
    if (!g_soc) return CPU_ERR;

    /* feature detection */
    cpu_features_detect(&g_cpu_id);
    cpu_id_print(&g_cpu_id);

    /* cache init */
    cpu_cache_info_t cinfo;
    cpu_cache_probe(&cinfo);
    cpu_cache_print(&cinfo);

    /* IRQ subsystem */
    cpu_irq_init();

#if defined(UIOX_ARCH_ARM64)
    cortex_a76_init();
    gic_init(g_soc->gic_dist_base, g_soc->gic_cpu_base, 3u);
    cpu_drv_timer_init(TIMER_DRV_ARM_GENERIC, 0u);

#elif defined(UIOX_ARCH_X86_64)
    x86_cpu_init();
    pic8259_disable();
    lapic_init(g_soc->lapic_base);
    ioapic_init(0xFEC00000u);
    cpu_drv_timer_init(TIMER_DRV_X86_LAPIC, 0u);

#elif defined(UIOX_ARCH_RISCV64)
    riscv64_init();
    plic_init(g_soc->plic_base, 127u, 0u);
    cpu_drv_timer_init(TIMER_DRV_RISCV_CLINT, 0x02000000u);
#endif

    /* SMP */
    cpu_smp_init();
    cpu_smp_print_info();

    printf("[soc] late init complete\n");
    return CPU_OK;
}

int cpu_soc_init(const cpu_soc_desc_t *soc)
{
    g_soc = soc;
    cpu_soc_early_init();
    cpu_soc_late_init();
    return CPU_OK;
}

void cpu_soc_print_info(void)
{
    if (!g_soc) return;
    printf("[soc] %s\n", g_soc->name);
    printf("  arch=%s  cores=%u\n",
           cpu_arch_str(g_soc->arch), g_soc->num_cores);
    printf("  DRAM  base=0x%016llx  size=0x%08llx\n",
           (unsigned long long)g_soc->dram_base,
           (unsigned long long)g_soc->dram_size);
    printf("  UART  base=0x%08llx  irq=%u\n",
           (unsigned long long)g_soc->uart_base,
           g_soc->uart_irq);
    printf("  TIMER irq=%u\n", g_soc->timer_irq);
}
