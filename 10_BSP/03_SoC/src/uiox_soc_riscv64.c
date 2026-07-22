/*
 * 02_FwHal/src/uiox_soc_riscv64.c
 * UIOX SoC abstraction — RISC-V 64 backend.
 */

 #include "../include/uiox_soc.h"
 #include "../include/uiox_soc_map.h"
 #include "../include/uiox_soc_stdio.h"    /* replaces <stdio.h>  */
 #include "../include/uiox_soc_string.h"   /* replaces <string.h> */
 #include "../../10_Arch/riscv64/include/arch_defs.h"
 //#include "../../10_Arch/arm32/include/arch_types.h"
 
 #include "../../10_Arch/arm32/include/mmio.h"
 #include "../../10_Arch/arm32/include/irq.h"
 
 /* ── CSR helpers ─────────────────────────────────────────────────────── */
 /* #define rv_csrr(reg)   ({ uiox_uint64_t _v; \
     __asm__ volatile("csrr %0, " #reg : "=r"(_v)); _v; }) */

/* REPLACE with per-CSR inline functions — ISO C11 compliant */
static inline uiox_uint64_t rv_read_misa(void)
{
    uiox_uint64_t v;
    __asm__ volatile("csrr %0, misa"      : "=r"(v));
    return v;
}
static inline uiox_uint64_t rv_read_mvendorid(void)
{
    uiox_uint64_t v;
    __asm__ volatile("csrr %0, mvendorid" : "=r"(v));
    return v;
}
static inline uiox_uint64_t rv_read_marchid(void)
{
    uiox_uint64_t v;
    __asm__ volatile("csrr %0, marchid"   : "=r"(v));
    return v;
}
static inline uiox_uint64_t rv_read_sie(void)
{
    uiox_uint64_t v;
    __asm__ volatile("csrr %0, sie"       : "=r"(v));
    return v;
}
static inline void rv_write_sie(uiox_uint64_t v)
{
    __asm__ volatile("csrw sie, %0" :: "r"(v) : "memory");
}

 /* #define rv_csrw(reg,v) \
     __asm__ volatile("csrw " #reg ", %0" \
                      :: "r"((uiox_uint64_t)(v)) : "memory") */
 
 /* ── CLINT init ──────────────────────────────────────────────────────── */
 static void rv_clint_init(uiox_uint32_t hart_id)
 {
     mmio_write32(SOC_CLINT_MSIP(hart_id), 0u);
     mmio_write32(SOC_CLINT_MTIMECMP(hart_id),      0xFFFFFFFFu);
     mmio_write32(SOC_CLINT_MTIMECMP(hart_id) + 4u, 0xFFFFFFFFu);
     printf("[soc/riscv] CLINT @ 0x%08lx  hart=%u  MTIMECMP cleared\n",
            (unsigned long)SOC_CLINT_BASE, hart_id);
 }
 
 /* ── PLIC init ───────────────────────────────────────────────────────── */
 static void rv_plic_init(void)
 {
     mmio_write32(SOC_PLIC_PRIORITY(SOC_UART_IRQ), 1u);
 
     uiox_uint32_t ctx  = 1u;
     uiox_uint32_t word = SOC_UART_IRQ / 32u;
     uiox_uint32_t bit  = SOC_UART_IRQ % 32u;
     uiox_uint32_t en   = mmio_read32(SOC_PLIC_ENABLE(ctx) + word * 4u);
     mmio_write32(SOC_PLIC_ENABLE(ctx) + word * 4u, en | (1u << bit));
     mmio_write32(SOC_PLIC_THRESHOLD(ctx), 0u);
 
     printf("[soc/riscv] PLIC @ 0x%08lx  UART_IRQ=%u enabled (ctx=%u)\n",
            (unsigned long)SOC_PLIC_BASE, SOC_UART_IRQ, ctx);
 }
 
 /* ── NS16550A UART init ──────────────────────────────────────────────── */
 static void rv_uart_init(void)
 {
     mmio_write32(SOC_UART0_BASE + 0x04u, 0x00u);
     mmio_write32(SOC_UART0_BASE + 0x03u, 0x83u);
     mmio_write32(SOC_UART0_BASE + 0x00u, 0x01u);
     mmio_write32(SOC_UART0_BASE + 0x01u, 0x00u);
     mmio_write32(SOC_UART0_BASE + 0x03u, 0x03u);
     mmio_write32(SOC_UART0_BASE + 0x02u, 0xC7u);
     mmio_write32(SOC_UART0_BASE + 0x04u, 0x01u);
     printf("[soc/riscv] NS16550A UART @ 0x%08lx  115200 8N1\n",
            (unsigned long)SOC_UART0_BASE);
 }
 
 /* ── SBI probe ───────────────────────────────────────────────────────── */
 static uiox_bool_t rv_sbi_probe_ext(uiox_uint64_t eid)
 {
     register uiox_uint64_t a0 __asm__("a0") = eid;
     register uiox_uint64_t a6 __asm__("a6") = 0u;
     register uiox_uint64_t a7 __asm__("a7") = 0x10u;
     __asm__ volatile("ecall" : "+r"(a0) : "r"(a6), "r"(a7) : "memory");
     return (a0 == 0u) ? UIOX_TRUE : UIOX_FALSE;
 }
 
 /* ── SoC identify ────────────────────────────────────────────────────── */
 static void rv_identify(uiox_soc_desc_t *desc)
 {
     //uiox_uint64_t misa      = rv_csrr(misa);
     //uiox_uint64_t mvendorid = rv_csrr(mvendorid);
     //uiox_uint64_t marchid   = rv_csrr(marchid);
     uiox_uint64_t misa      = rv_read_misa();
     uiox_uint64_t mvendorid = rv_read_mvendorid();
     uiox_uint64_t marchid   = rv_read_marchid();
     (void)marchid; (void)misa;
 
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
 
     if (rv_sbi_probe_ext(0x54494D45u))
         desc->capabilities |= UIOX_SOC_CAP_SBI;
     if (desc->l2_cache_kb > 0u)
         desc->capabilities |= UIOX_SOC_CAP_CACHE_L2;
     if (desc->num_cpus > 1u)
         desc->capabilities |= UIOX_SOC_CAP_SMP;
 }
 
 int uiox_soc_init_riscv64(uiox_soc_desc_t *desc)
 {
     if (!desc) return UIOX_SOC_ERR_INVAL;
 
     printf("[soc/riscv] Initialising RISC-V 64 SoC layer...\n");
     rv_identify(desc);
     printf("[soc/riscv] SoC: %s  CPUs=%u  DRAM=0x%llx+%llu MB\n",
            desc->name, desc->num_cpus,
            (unsigned long long)desc->dram_base,
            (unsigned long long)(desc->dram_size >> 20));
 
     uiox_clk_ctx_t *clk = uiox_soc_get_clk();
     uiox_soc_clk_init(clk, desc);
 
     rv_clint_init(0u);
     rv_plic_init();
     rv_uart_init();
 
     uiox_pm_ctx_t *pm = uiox_soc_get_pm();
     uiox_soc_pm_init(pm, desc);
 
     /* Enable S-mode external interrupts (PLIC → SEIE bit) */
     //uiox_uint64_t sie = rv_csrr(sie);
     uiox_uint64_t sie = rv_read_sie();
     //rv_csrw(sie, sie | (1u << 9));
     rv_write_sie(sie | (1u << 9));
 
     desc->initialized = true;
     printf("[soc/riscv] RISC-V 64 SoC layer ready.\n");
     return UIOX_SOC_OK;
 } 
 