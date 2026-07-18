/*
 * 02_FwHal/src/uiox_soc_arm32.c
 * UIOX SoC abstraction — ARM32 (ARMv7-A / Cortex-A) backend.
 */

 #include "../include/uiox_soc.h"
 #include "../include/uiox_soc_map.h"
 #include "../include/uiox_soc_stdio.h"    /* replaces <stdio.h>  */
 #include "../include/uiox_soc_string.h"   /* replaces <string.h> */
 #include "../../10_Arch/arm32/include/arch_defs.h"
 #include "../../20_DriverInterfaces/include/hw_types.h"
 #include "../../20_DriverInterfaces/include/mmio.h"
 #include "../../20_DriverInterfaces/include/irq.h"
 
 /* ── CP15 helpers ────────────────────────────────────────────────────── */
 
 static uiox_uint32_t arm32_read_midr(void)
 {
     uiox_uint32_t midr = 0u;
     __asm__ volatile("mrc p15, 0, %0, c0, c0, 0"
                      : "=r"(midr) :: "memory");
     return midr;
 }
 
 static void arm32_dsb(void)  { __asm__ volatile("dsb" ::: "memory"); }
 static void arm32_isb(void)  { __asm__ volatile("isb" ::: "memory"); }
 
 static void arm32_icache_invalidate(void)
 {
     __asm__ volatile("mcr p15, 0, %0, c7, c5,  0" :: "r"(0u));
     __asm__ volatile("mcr p15, 0, %0, c7, c5,  6" :: "r"(0u));
     arm32_dsb(); arm32_isb();
 }
 
 static void arm32_cache_enable(void)
 {
     uiox_uint32_t sctlr;
     __asm__ volatile("mrc p15, 0, %0, c1, c0, 0" : "=r"(sctlr));
     sctlr |= (1u << 12) | (1u << 2);
     __asm__ volatile("mcr p15, 0, %0, c1, c0, 0" :: "r"(sctlr) : "memory");
     arm32_isb();
     printf("[soc/arm32] I/D caches enabled\n");
 }
 
 /* ── MIDR decode ─────────────────────────────────────────────────────── */
 #define MIDR32_PARTNUM(m)  (((m) >> 4) & 0xFFFu)
 #define MIDR32_PART_A7     0xC07u
 #define MIDR32_PART_A9     0xC09u
 
 /* ── GIC-400 init ────────────────────────────────────────────────────── */
 static void arm32_gic400_init(void)
 {
     mmio_write32(SOC_GIC_DIST_BASE + 0x000u, 0x0u);
     mmio_write32(SOC_GIC_DIST_BASE + 0x100u, 0xFFFFFFFFu);
     mmio_write32(SOC_GIC_DIST_BASE + 0x104u, 0xFFFFFFFFu);
     mmio_write32(SOC_GIC_CPU_BASE  + 0x004u, 0xFFu);
     mmio_write32(SOC_GIC_CPU_BASE  + 0x000u, 0x1u);
     mmio_write32(SOC_GIC_DIST_BASE + 0x000u, 0x1u);
     printf("[soc/arm32] GIC-400 init (DIST=0x%08lx CPU=0x%08lx)\n",
            (unsigned long)SOC_GIC_DIST_BASE,
            (unsigned long)SOC_GIC_CPU_BASE);
 }
 
 /* ── PL011 UART init ─────────────────────────────────────────────────── */
 static void arm32_uart0_clk_init(const uiox_clk_ctx_t *clk)
 {
     uiox_uint32_t ref  = uiox_soc_clk_get_hz(clk, UIOX_SOC_CLK_UART0);
     uiox_uint32_t baud = 115200u;
     uiox_uint32_t ibrd = UIOX_SOC_UART_IBRD(ref, baud);
     uiox_uint32_t fbrd = UIOX_SOC_UART_FBRD(ref, baud);
 
     mmio_write32(SOC_UART0_BASE + 0x030u, 0x0u);
     mmio_write32(SOC_UART0_BASE + 0x024u, ibrd);
     mmio_write32(SOC_UART0_BASE + 0x028u, fbrd);
     mmio_write32(SOC_UART0_BASE + 0x02Cu, 0x70u);
     mmio_write32(SOC_UART0_BASE + 0x038u, (1u << 4));
     mmio_write32(SOC_UART0_BASE + 0x030u, 0x301u);
 
     printf("[soc/arm32] UART0 @ 0x%08lx  %u baud (IBRD=%u FBRD=%u)\n",
            (unsigned long)SOC_UART0_BASE, baud, ibrd, fbrd);
 }
 
 /* ── SP804 timer init ────────────────────────────────────────────────── */
 static void arm32_timer_init(void)
 {
     mmio_write32(SOC_TIMER0_BASE + 0x008u, 0x00u);
     mmio_write32(SOC_TIMER0_BASE + 0x000u, 1000u);
     mmio_write32(SOC_TIMER0_BASE + 0x018u, 1000u);
     mmio_write32(SOC_TIMER0_BASE + 0x008u, 0xE2u);
     printf("[soc/arm32] SP804 timer @ 0x%08lx  1 ms tick\n",
            (unsigned long)SOC_TIMER0_BASE);
 }
 
 /* ── SoC identify ────────────────────────────────────────────────────── */
 static void arm32_identify(uiox_soc_desc_t *desc)
 {
     uiox_uint32_t midr    = arm32_read_midr();
     uiox_uint32_t partnum = MIDR32_PARTNUM(midr);
 
     switch (partnum) {
     case MIDR32_PART_A7:
         desc->soc_id  = UIOX_SOC_BCM2836;
         strncpy(desc->name, "BCM2836 (Cortex-A7)", UIOX_SOC_NAME_LEN - 1);
         desc->num_cpus     = 4u;
         desc->cpu_freq_khz = 900000u;
         desc->l1_icache_kb = 32u;
         desc->l1_dcache_kb = 32u;
         desc->l2_cache_kb  = 512u;
         desc->capabilities |= UIOX_SOC_CAP_GIC_V2 | UIOX_SOC_CAP_EMMC
                              | UIOX_SOC_CAP_USB | UIOX_SOC_CAP_ETH;
         break;
 
     case MIDR32_PART_A9:
         desc->soc_id  = UIOX_SOC_IMX6Q;
         strncpy(desc->name, "NXP i.MX 6Quad (Cortex-A9)",
                 UIOX_SOC_NAME_LEN - 1);
         desc->num_cpus     = 4u;
         desc->cpu_freq_khz = 1200000u;
         desc->l1_icache_kb = 32u;
         desc->l1_dcache_kb = 32u;
         desc->l2_cache_kb  = 1024u;
         desc->capabilities |= UIOX_SOC_CAP_GIC_V2 | UIOX_SOC_CAP_TRNG
                              | UIOX_SOC_CAP_TRUSTZONE | UIOX_SOC_CAP_EMMC
                              | UIOX_SOC_CAP_USB | UIOX_SOC_CAP_ETH;
         break;
 
     default:
         desc->soc_id  = UIOX_SOC_QEMU_VIRT_A32;
         strncpy(desc->name, "QEMU virt (arm32)", UIOX_SOC_NAME_LEN - 1);
         desc->num_cpus     = 1u;
         desc->cpu_freq_khz = 400000u;
         desc->l1_icache_kb = 32u;
         desc->l1_dcache_kb = 32u;
         desc->l2_cache_kb  = 0u;
         desc->capabilities |= UIOX_SOC_CAP_GIC_V2 | UIOX_SOC_CAP_DTB
                              | UIOX_SOC_CAP_UART;
         break;
     }
 
     desc->dram_base     = SOC_DRAM_BASE;
     desc->dram_size     = SOC_DRAM_SIZE;
     desc->num_clusters  = (desc->num_cpus + 3u) / 4u;
     desc->capabilities |= UIOX_SOC_CAP_MMU    | UIOX_SOC_CAP_CACHE_L1
                         |  UIOX_SOC_CAP_CACHE_L2 | UIOX_SOC_CAP_SMP;
     if (desc->l2_cache_kb == 0u)
         desc->capabilities &= ~UIOX_SOC_CAP_CACHE_L2;
 }
 
 int uiox_soc_init_arm32(uiox_soc_desc_t *desc)
 {
     if (!desc) return UIOX_SOC_ERR_INVAL;
 
     printf("[soc/arm32] Initialising ARM32 SoC layer...\n");
 
     arm32_icache_invalidate();
     arm32_cache_enable();
     arm32_identify(desc);
 
     printf("[soc/arm32] SoC: %s  CPUs=%u  DRAM=0x%llx+%llu MB\n",
            desc->name, desc->num_cpus,
            (unsigned long long)desc->dram_base,
            (unsigned long long)(desc->dram_size >> 20));
 
     uiox_clk_ctx_t *clk = uiox_soc_get_clk();
     uiox_soc_clk_init(clk, desc);
 
     arm32_gic400_init();
     arm32_uart0_clk_init(clk);
     arm32_timer_init();
 
     uiox_pm_ctx_t *pm = uiox_soc_get_pm();
     uiox_soc_pm_init(pm, desc);
 
     desc->initialized = true;
     printf("[soc/arm32] ARM32 SoC layer ready.\n");
     return UIOX_SOC_OK;
 }
 