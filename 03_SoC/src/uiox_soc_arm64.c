/*
 * 02_FwHal/src/uiox_soc_arm64.c
 * UIOX SoC abstraction — ARM64 (AArch64 / ARMv8-A) backend.
 */

 #include "../include/uiox_soc.h"
 #include "../include/uiox_soc_map.h"     /* ← ADD: SOC_GIC_DIST_BASE,  */
                                          /*         SOC_GIC_CPU_BASE,   */
                                          /*         SOC_UART0_BASE,     */
                                          /*         SOC_DRAM_BASE etc.  */
 #include "../include/uiox_soc_stdio.h"    /* replaces <stdio.h>  */
 #include "../include/uiox_soc_string.h"   /* replaces <string.h> */
 #include "../../10_Arch/arm64/include/arch_defs.h"
 #include "../../20_DriverInterfaces/include/hw_types.h"
 #include "../../20_DriverInterfaces/include/mmio.h"
 #include "../../20_DriverInterfaces/include/irq.h"
 
 /* ── MIDR_EL1 decode ─────────────────────────────────────────────────── */
 #define MIDR_PARTNUM(m)     (((m) >> 4)  & 0xFFFu)
 #define MIDR_IMPLEMENTER(m) (((m) >> 24) & 0xFFu)
 
 #define MIDR_IMPL_ARM   0x41u
 #define MIDR_PART_A53   0xD03u
 #define MIDR_PART_A55   0xD05u
 #define MIDR_PART_A72   0xD08u
 #define MIDR_PART_A76   0xD0Bu
 #define MIDR_PART_A78   0xD41u
 
 static uiox_uint64_t arm64_read_midr(void)
 {
     uiox_uint64_t midr = 0u;
     __asm__ volatile("mrs %0, midr_el1" : "=r"(midr) :: "memory");
     return midr;
 }
 
 /* ── GICv3 system register init ──────────────────────────────────────── */
 static void arm64_gicv3_sre_enable(void)
 {
     uiox_uint64_t sre;
     __asm__ volatile("mrs %0, ICC_SRE_EL1" : "=r"(sre));
     sre |= 0x7u;
     __asm__ volatile("msr ICC_SRE_EL1, %0" :: "r"(sre) : "memory");
     arch_isb();
 }
 
 /* ── GIC-400 distributor + CPU interface init ────────────────────────── */
 static void arm64_gic400_init(void)
 {
     mmio_write32(SOC_GIC_DIST_BASE + 0x000u, 0x0u);
     mmio_write32(SOC_GIC_DIST_BASE + 0x100u, 0xFFFFFFFFu);
     mmio_write32(SOC_GIC_CPU_BASE  + 0x004u, 0xFFu);
     mmio_write32(SOC_GIC_CPU_BASE  + 0x000u, 0x1u);
     mmio_write32(SOC_GIC_DIST_BASE + 0x000u, 0x1u);
     printf("[soc/arm64] GIC-400 init (DIST=0x%08lx CPU=0x%08lx)\n",
            (unsigned long)SOC_GIC_DIST_BASE,
            (unsigned long)SOC_GIC_CPU_BASE);
 }
 
 /* ── PL011 UART clock configuration ─────────────────────────────────── */
 static void arm64_uart0_clk_init(const uiox_clk_ctx_t *clk)
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
 
     printf("[soc/arm64] UART0 @ 0x%08lx  %u baud (IBRD=%u FBRD=%u)\n",
            (unsigned long)SOC_UART0_BASE, baud, ibrd, fbrd);
 }
 
 /* ── Identify SoC from MIDR_EL1 ─────────────────────────────────────── */
 static void arm64_identify(uiox_soc_desc_t *desc)
 {
     uiox_uint64_t midr    = arm64_read_midr();
     uiox_uint32_t partnum = (uiox_uint32_t)MIDR_PARTNUM(midr);
 
     switch (partnum) {
     case MIDR_PART_A72:
         desc->soc_id  = UIOX_SOC_BCM2711;
         strncpy(desc->name, "BCM2711 (Cortex-A72)", UIOX_SOC_NAME_LEN - 1);
         desc->num_cpus     = 4u;
         desc->cpu_freq_khz = 1500000u;
         desc->l1_icache_kb = 48u;
         desc->l1_dcache_kb = 32u;
         desc->l2_cache_kb  = 1024u;
         desc->capabilities |= UIOX_SOC_CAP_GIC_V2 | UIOX_SOC_CAP_TRNG
                              | UIOX_SOC_CAP_TRUSTZONE | UIOX_SOC_CAP_EMMC
                              | UIOX_SOC_CAP_USB | UIOX_SOC_CAP_ETH;
         break;
 
     case MIDR_PART_A76:
         desc->soc_id  = UIOX_SOC_BCM2712;
         strncpy(desc->name, "BCM2712 (Cortex-A76)", UIOX_SOC_NAME_LEN - 1);
         desc->num_cpus     = 4u;
         desc->cpu_freq_khz = 2400000u;
         desc->l1_icache_kb = 64u;
         desc->l1_dcache_kb = 64u;
         desc->l2_cache_kb  = 512u;
         desc->l3_cache_kb  = 2048u;
         desc->capabilities |= UIOX_SOC_CAP_GIC_V3 | UIOX_SOC_CAP_TRNG
                              | UIOX_SOC_CAP_TRUSTZONE | UIOX_SOC_CAP_PCIE
                              | UIOX_SOC_CAP_USB | UIOX_SOC_CAP_ETH;
         break;
 
     default:
         desc->soc_id  = UIOX_SOC_QEMU_VIRT_A64;
         strncpy(desc->name, "QEMU virt (arm64)", UIOX_SOC_NAME_LEN - 1);
         desc->num_cpus     = 1u;
         desc->cpu_freq_khz = 1000000u;
         desc->l1_icache_kb = 32u;
         desc->l1_dcache_kb = 32u;
         desc->capabilities |= UIOX_SOC_CAP_GIC_V2 | UIOX_SOC_CAP_DTB
                              | UIOX_SOC_CAP_UART;
         break;
     }
 
     desc->dram_base     = SOC_DRAM_BASE;
     desc->dram_size     = SOC_DRAM_SIZE;
     desc->num_clusters  = (desc->num_cpus + 3u) / 4u;
     desc->capabilities |= UIOX_SOC_CAP_MMU    | UIOX_SOC_CAP_CACHE_L1
                         |  UIOX_SOC_CAP_CACHE_L2 | UIOX_SOC_CAP_SMP;
 }
 
 int uiox_soc_init_arm64(uiox_soc_desc_t *desc)
 {
     if (!desc) return UIOX_SOC_ERR_INVAL;
 
     printf("[soc/arm64] Initialising ARM64 SoC layer...\n");
     arm64_identify(desc);
     printf("[soc/arm64] SoC: %s  CPUs=%u  DRAM=0x%llx+%llu MB\n",
            desc->name, desc->num_cpus,
            (unsigned long long)desc->dram_base,
            (unsigned long long)(desc->dram_size >> 20));
 
     uiox_clk_ctx_t *clk = uiox_soc_get_clk();
     uiox_soc_clk_init(clk, desc);
 
     if (desc->capabilities & UIOX_SOC_CAP_GIC_V3)
         arm64_gicv3_sre_enable();
     arm64_gic400_init();
     arm64_uart0_clk_init(clk);
 
     uiox_pm_ctx_t *pm = uiox_soc_get_pm();
     uiox_soc_pm_init(pm, desc);
 
     desc->initialized = true;
     printf("[soc/arm64] ARM64 SoC layer ready.\n");
     return UIOX_SOC_OK;
 }
 