/*
 * 10_Arch/arm64/src/arch_init.c
 * ARMv8-A 64-bit architecture initialisation.
 *
 * Scope — architecture layer ONLY (ISA-defined operations):
 *   1. CPU identification via MIDR_EL1 / MPIDR_EL1
 *   2. Enable I/D caches via SCTLR_EL1
 *   3. Invalidate TLB and I-cache (architecture maintenance ops)
 *   4. Configure GIC-400 distributor + CPU interface
 *      (GIC register layout is ARM architecture-defined;
 *       base addresses from arch_defs.h QEMU default)
 *   5. Install VBAR_EL1 (exception vector base — ISA register)
 *   6. Configure generic timer tick (CNTFRQ_EL0 / CNTP_CTL_EL0)
 *   7. Register IRQ handlers via 20_DriverInterfaces
 *   8. Enable global interrupts (ISA: DAIF clear)
 *
 * NOT in this file — belongs in 03_SoC:
 *   PL011 baud rate config    → 03_SoC/uiox_soc_arm64.c
 *   VirtIO device init        → 03_SoC/uiox_soc_arm64.c
 *   Clock PLL / CCM setup     → 03_SoC/uiox_soc_clk.c
 *   Power domain control      → 03_SoC/uiox_soc_pm.c
 *   SoC MMIO map details      → 03_SoC/include/uiox_soc_map.h
 *
 * Kernel call chain:
 *   uiox_kernel_main()
 *     → arch_init()       ← THIS FILE
 *     → uiox_soc_init()   ← 03_SoC (called next by kernel)
 */

 #include "arch_defs.h"
 #include "hw_types.h"   // resolved via -I<arch>/include (already in Makefile)
#include "mmio.h"
#include "irq.h"
#include "cpu.h"
 #include "../../../03_SoC/include/uiox_soc_stdio.h"   /* replaces <stdio.h>  */
 #include "../../../03_SoC/include/uiox_soc_string.h"  /* replaces <string.h> */
 
 /* ── Forward declarations of IRQ handlers ────────────────────────────
  * Implementations live in 20_DriverInterfaces or 03_SoC.
  * arch_init only wires them to IRQ numbers.
  * ──────────────────────────────────────────────────────────────────── */
 extern void uart0_irq_handler (int irq, hw_context_t *ctx, void *id);
 extern void timer0_irq_handler(int irq, hw_context_t *ctx, void *id);
 
 /* =========================================================================
  * 1. CPU identification
  *    ISA system registers: MIDR_EL1, MPIDR_EL1 — same on every ARMv8-A chip
  * ====================================================================== */
 static void arm64_cpu_identify(void)
 {
     unsigned long midr  = 0UL;
     unsigned long mpidr = 0UL;
     __asm__ volatile("mrs %0, midr_el1"  : "=r"(midr));
     __asm__ volatile("mrs %0, mpidr_el1" : "=r"(mpidr));
 
     unsigned int part  = (unsigned int)((midr >>  4u) & 0xFFFu);
     unsigned int var   = (unsigned int)((midr >> 20u) & 0xFu);
     unsigned int rev   = (unsigned int)((midr >>  0u) & 0xFu);
     unsigned int cpu   = (unsigned int)( mpidr        & 0x3u);
     unsigned int clust = (unsigned int)((mpidr >>  8u) & 0xFu);
 
     printf("[arm64] CPU[%u:%u] MIDR=0x%016lx (part=0x%03x r%up%u)\n",
            clust, cpu, midr, part, var, rev);
 }
 
 /* =========================================================================
  * 2. Cache enable + TLB/I-cache invalidation
  *    All operations are ISA-defined CP system registers / instructions.
  *    Same instructions on BCM2711, RK3588, i.MX8, QEMU — all ARMv8-A.
  * ====================================================================== */
 static void arm64_cache_enable(void)
 {
     /* Invalidate I-cache and branch predictor */
     arch_ic_iallu();
     arch_dsb_sy();
 
     /* Invalidate entire TLB */
     arch_tlbi_vmalle1is();
     arch_dsb_sy();
     arch_isb();
 
     /* Enable I-cache (bit 12) and D-cache (bit 2) in SCTLR_EL1 */
     unsigned long sctlr;
     __asm__ volatile("mrs %0, sctlr_el1" : "=r"(sctlr));
     sctlr |= SCTLR_EL1_I | SCTLR_EL1_C;
     __asm__ volatile("msr sctlr_el1, %0" :: "r"(sctlr) : "memory");
     arch_isb();
 
     printf("[arm64] I/D caches enabled (SCTLR_EL1=0x%016lx)\n", sctlr);
 }
 
 /* =========================================================================
  * 3. GIC-400 initialisation
  *    GIC register layout is ARM Generic Interrupt Controller architecture.
  *    Base addresses GIC_DIST_BASE / GIC_CPU_BASE come from arch_defs.h
  *    (QEMU virt defaults); 03_SoC may override via uiox_soc_map.h.
  * ====================================================================== */
 static void arm64_gic_init(void)
 {
     /* Step 1 — disable distributor before configuration */
     mmio_write32(GIC_DIST_CTLR, 0x0u);
 
     /* Step 2 — enable all SPI interrupt lines */
     mmio_write32(GIC_DIST_ISENABLER0 + 0x04u, 0xFFFFFFFFu);
     mmio_write32(GIC_DIST_ISENABLER0 + 0x08u, 0xFFFFFFFFu);
 
     /* Step 3 — set all priorities to mid-level */
     for (unsigned int i = 0u; i < 64u; i++)
         mmio_write32(GIC_DIST_IPRIORITYR0 + i * 4u, 0xA0A0A0A0u);
 
     /* Step 4 — CPU interface: allow all priorities, enable */
     mmio_write32(GIC_CPU_PMR,  0xFFu);
     mmio_write32(GIC_CPU_CTLR, 0x1u);
 
     /* Step 5 — re-enable distributor */
     mmio_write32(GIC_DIST_CTLR, 0x1u);
 
     printf("[arm64] GIC-400 init (DIST=0x%08lx CPU=0x%08lx)\n",
            (unsigned long)GIC_DIST_BASE,
            (unsigned long)GIC_CPU_BASE);
 }
 
 /* =========================================================================
  * 4. VBAR_EL1 — Exception Vector Base Address Register
  *    ISA register: same MRS/MSR encoding on every ARMv8-A chip.
  *    The vector table itself is provided by the kernel linker script.
  * ====================================================================== */
 extern unsigned long _vector_table[];   /* kernel linker symbol */
 
 static void arm64_vbar_install(void)
 {
     __asm__ volatile("msr vbar_el1, %0" :: "r"(_vector_table) : "memory");
     arch_isb();
     printf("[arm64] VBAR_EL1 installed @ %p\n", (void *)_vector_table);
 }
 
 /* =========================================================================
  * 5. Generic timer  (ISA — CNTFRQ_EL0, CNTP_TVAL_EL0, CNTP_CTL_EL0)
  *    The generic timer is defined by the ARMv8-A architecture.
  *    Every ARMv8-A chip has it; the IRQ PPI number is from arch_defs.h.
  *    Frequency comes from CNTFRQ_EL0 programmed by the bootloader.
  *    Actual period = freq / 100  →  100 Hz tick.
  * ====================================================================== */
 static void arm64_timer_init(void)
 {
     unsigned long freq;
     __asm__ volatile("mrs %0, cntfrq_el0" : "=r"(freq));
 
     unsigned long tval = freq / 100u;   /* 100 Hz */
     __asm__ volatile("msr cntp_tval_el0, %0" :: "r"(tval));
     __asm__ volatile("msr cntp_ctl_el0,  %0" :: "r"(1UL));  /* enable */
 
     printf("[arm64] Generic timer: %lu Hz / 100 = %lu ticks (PPI %u)\n",
            freq, tval, ARCH_TIMER_IRQ_PHYS);
 }
 
 /* =========================================================================
  * arch_init — called by uiox_kernel_main() before uiox_soc_init()
  * ====================================================================== */
 int arch_init(void)
 {
     printf("\n[arch_init] *** ARM64 (ARMv8-A) ***\n");
 
     /* 1. Identify CPU via MIDR_EL1 / MPIDR_EL1 */
     arm64_cpu_identify();
 
     /* 2. Enable caches and invalidate stale TLB/I-cache entries */
     arm64_cache_enable();
 
     /* 3. Configure GIC-400 (architecture-defined register layout) */
     arm64_gic_init();
 
     /* 4. Install exception vector table (ISA register VBAR_EL1) */
     arm64_vbar_install();
 
     /* 5. Start generic timer at 100 Hz (ISA system registers) */
     arm64_timer_init();
 
     /* 6. Initialise driver-interface layer */
     mmio_init();
     irq_init();
 
     /* 7. Register IRQ handlers
      *    ARCH_TIMER_IRQ_PHYS and UART0_IRQ come from arch_defs.h.
      *    Handler implementations are in 20_DriverInterfaces / 03_SoC. */
     irq_request(ARCH_TIMER_IRQ_PHYS, timer0_irq_handler, NULL, "timer");
     irq_request(UART0_IRQ,           uart0_irq_handler,  NULL, "uart0");
 
     irq_enable(ARCH_TIMER_IRQ_PHYS);
     irq_enable(UART0_IRQ);
 
     /* 8. Unmask IRQs at CPU level — clear DAIF.I (ISA operation) */
     __asm__ volatile("msr daifclr, #2" ::: "memory");
 
     printf("[arch_init] ARM64 architecture layer ready\n");
     /* uiox_soc_init() is called next by uiox_kernel_main() */
     return 0;
 }
 
 void arch_fini(void)
 {
     /* Mask IRQs at CPU level */
     __asm__ volatile("msr daifset, #2" ::: "memory");
     irq_free(ARCH_TIMER_IRQ_PHYS);
     irq_free(UART0_IRQ);
     printf("[arch_fini] ARM64 architecture layer torn down\n");
 }
 