/*
 * 02_FwHal/src/uiox_soc_x86.c
 * UIOX SoC abstraction — x86-64 backend.
 *
 * Detects CPU vendor/model via CPUID, configures LAPIC, HPET,
 * COM1 UART, then populates the SoC descriptor.
 */

 #include "../include/uiox_soc.h"
 #include "../include/uiox_soc_map.h"
 #include "../include/uiox_soc_stdio.h"    /* replaces <stdio.h>  */
 #include "../include/uiox_soc_string.h"   /* replaces <string.h> */
 #include "../../10_Arch/x86_64/include/arch_defs.h"
 #include "../../20_DriverInterfaces/include/hw_types.h"
 #include "../../20_DriverInterfaces/include/mmio.h"
 #include "../../20_DriverInterfaces/include/irq.h"
 
 /* ── CPUID helper ────────────────────────────────────────────────────── */
 static void x86_cpuid(uiox_uint32_t  leaf,
                        uiox_uint32_t  subleaf,
                        uiox_uint32_t *eax,
                        uiox_uint32_t *ebx,
                        uiox_uint32_t *ecx,
                        uiox_uint32_t *edx)
 {
     __asm__ volatile(
         "cpuid"
         : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
         : "a"(leaf), "c"(subleaf)
     );
 }
 
 /* ── Port I/O helpers ────────────────────────────────────────────────── */
 static inline void x86_outb(uiox_uint16_t port, uiox_uint8_t val)
 { __asm__ volatile("outb %0,%1" :: "a"(val), "Nd"(port)); }
 
 static inline uiox_uint8_t x86_inb(uiox_uint16_t port)
 {
     uiox_uint8_t v;
     __asm__ volatile("inb %1,%0" : "=a"(v) : "Nd"(port));
     return v;
 }
 
 static inline void x86_outw(uiox_uint16_t port, uiox_uint16_t val)
 { __asm__ volatile("outw %0,%1" :: "a"(val), "Nd"(port)); }
 
 /* ── LAPIC init ──────────────────────────────────────────────────────── */
 static void x86_lapic_init(void)
 {
     uiox_uint32_t lo, hi;
     __asm__ volatile("rdmsr" : "=a"(lo), "=d"(hi) : "c"(0x1Bu));
     lo |= (1u << 11);
     __asm__ volatile("wrmsr" :: "c"(0x1Bu), "a"(lo), "d"(hi));
 
     mmio_write32(SOC_LAPIC_BASE + 0x0F0u, 0x1FFu);
     printf("[soc/x86]  LAPIC enabled at 0x%08lx\n",
            (unsigned long)SOC_LAPIC_BASE);
 }
 
 /* ── Legacy 16550A COM1 UART init ────────────────────────────────────── */
 static void x86_uart_init(void)
 {
     x86_outb(SOC_UART0_PORT + 1u, 0x00u);
     x86_outb(SOC_UART0_PORT + 3u, 0x80u);
     x86_outb(SOC_UART0_PORT + 0u, 0x01u);
     x86_outb(SOC_UART0_PORT + 1u, 0x00u);
     x86_outb(SOC_UART0_PORT + 3u, 0x03u);
     x86_outb(SOC_UART0_PORT + 2u, 0xC7u);
     x86_outb(SOC_UART0_PORT + 4u, 0x0Bu);
     x86_outb(SOC_UART0_PORT + 1u, 0x01u);
     printf("[soc/x86]  COM1 UART @ 0x%04x  115200 8N1\n",
            (unsigned)SOC_UART0_PORT);
     /* suppress unused-function warnings on some build configs */
     (void)x86_inb;
     (void)x86_outw;
 }
 
 /* ── HPET init ───────────────────────────────────────────────────────── */
 static void x86_hpet_init(void)
 {
     uiox_uint32_t caps_lo = mmio_read32(SOC_HPET_BASE + 0x000u);
     if (caps_lo == 0u || caps_lo == 0xFFFFFFFFu) {
         printf("[soc/x86]  HPET not present — skipping\n");
         return;
     }
     uiox_uint32_t cfg = mmio_read32(SOC_HPET_BASE + 0x010u);
     mmio_write32(SOC_HPET_BASE + 0x010u, cfg | 0x1u);
     printf("[soc/x86]  HPET enabled at 0x%08lx\n",
            (unsigned long)SOC_HPET_BASE);
 }
 
 /* ── CPUID-based SoC identify ────────────────────────────────────────── */
 static void x86_identify(uiox_soc_desc_t *desc)
 {
     uiox_uint32_t eax, ebx, ecx, edx;
     char vendor[13];
 
     /* Zero the vendor buffer without memset/string.h */
     for (int i = 0; i < 13; i++) vendor[i] = 0;
 
     /* Vendor string from leaf 0 */
     x86_cpuid(0u, 0u, &eax, &ebx, &ecx, &edx);
     uiox_memcpy_u32(vendor + 0, &ebx);
     uiox_memcpy_u32(vendor + 4, &edx);
     uiox_memcpy_u32(vendor + 8, &ecx);
 
     desc->soc_id = UIOX_SOC_X86_QEMU_Q35;
     snprintf(desc->name, UIOX_SOC_NAME_LEN,
              "x86-64 [%s] maxleaf=%u", vendor, eax);
 
     /* Logical CPU count from leaf 1 EBX[23:16] */
     x86_cpuid(1u, 0u, &eax, &ebx, &ecx, &edx);
     desc->num_cpus     = (ebx >> 16u) & 0xFFu;
     if (desc->num_cpus == 0u) desc->num_cpus = 1u;
     desc->num_clusters = 1u;
     desc->cpu_freq_khz = 0u;
 
     desc->l1_dcache_kb = 32u;
     desc->l1_icache_kb = 32u;
     desc->l2_cache_kb  = 256u;
     desc->l3_cache_kb  = 0u;
 
     desc->dram_base    = SOC_DRAM_BASE;
     desc->dram_size    = SOC_DRAM_SIZE;
 
     desc->capabilities = UIOX_SOC_CAP_MMU      | UIOX_SOC_CAP_CACHE_L1
                        | UIOX_SOC_CAP_CACHE_L2 | UIOX_SOC_CAP_SMP
                        | UIOX_SOC_CAP_APIC     | UIOX_SOC_CAP_UART
                        | UIOX_SOC_CAP_ACPI     | UIOX_SOC_CAP_EFI
                        | UIOX_SOC_CAP_PCIE;
 
     /* RDRAND (leaf 1, ECX bit 30) */
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
     printf("[soc/x86]  SoC: %s  CPUs=%u\n",
            desc->name, desc->num_cpus);
 
     uiox_clk_ctx_t *clk = uiox_soc_get_clk();
     uiox_soc_clk_init(clk, desc);
 
     x86_lapic_init();
     x86_uart_init();
     x86_hpet_init();
 
     uiox_pm_ctx_t *pm = uiox_soc_get_pm();
     uiox_soc_pm_init(pm, desc);
 
     desc->initialized = true;
     printf("[soc/x86]  x86-64 SoC layer ready.\n");
     return UIOX_SOC_OK;
 }
 