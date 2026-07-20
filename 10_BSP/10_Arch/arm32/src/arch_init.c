/*
 * 10_Arch/arm32/src/arch_init.c
 * ARMv7-A 32-bit architecture initialisation.
 *
 * Scope — architecture layer only:
 *   1. Read MIDR / MPIDR (CPU identity)
 *   2. Enable I/D caches via CP15 SCTLR
 *   3. Set ACTLR.SMP so cache maintenance broadcasts work
 *   4. Configure GIC-400 distributor + CPU interface
 *      (register layout is ISA-defined, base from arch_defs.h)
 *   5. Install VBAR (exception vector base) — ISA register
 *   6. Register IRQ handlers via 20_DriverInterfaces
 *   7. Enable global interrupts
 *
 * NOT in this file (all in 03_SoC):
 *   - UART baud rate programming   → uiox_soc_arm32.c
 *   - SP804 / private timer load   → uiox_soc_arm32.c
 *   - Clock PLL configuration      → uiox_soc_clk.c
 *   - Power domain control         → uiox_soc_pm.c
 *   - SCU / L2C-310 enable         → uiox_soc_arm32.c
 *
 * Called by:
 *   uiox_kernel_main() → arch_init() → uiox_soc_init()
 */

 #include "arch_defs.h"
 #include "../../../20_DriverInterfaces/include/hw_types.h"
 #include "../../../20_DriverInterfaces/include/mmio.h"
 #include "../../../20_DriverInterfaces/include/irq.h"
 #include "../../../20_DriverInterfaces/include/cpu.h"
 #include "../../../03_SoC/include/uiox_soc_stdio.h"    /* no <stdio.h>   */
 #include "../../../03_SoC/include/uiox_soc_string.h"   /* no <string.h>  */
 
 /* ── Forward declarations of IRQ handlers ────────────────────────────
  * The handlers themselves live in the driver layer (20_DriverInterfaces)
  * or the SoC layer (03_SoC).  arch_init only registers them.
  * ──────────────────────────────────────────────────────────────────── */
 extern void uart0_irq_handler (int irq, hw_context_t *ctx, void *id);
 extern void timer0_irq_handler(int irq, hw_context_t *ctx, void *id);
 
 /* =========================================================================
  * 1. CPU identification  (ISA — CP15 read, same on every ARMv7-A)
  * ====================================================================== */
 static void arm32_cpu_identify(void)
 {
     unsigned int midr  = 0u;
     unsigned int mpidr = 0u;
     __asm__ volatile("mrc p15,0,%0,c0,c0,0" : "=r"(midr));
     __asm__ volatile("mrc p15,0,%0,c0,c0,5" : "=r"(mpidr));
 
     unsigned int part  = (midr >>  4u) & 0xFFFu;
     unsigned int var   = (midr >> 20u) & 0xFu;
     unsigned int rev   = (midr >>  0u) & 0xFu;
     unsigned int cpu   =  mpidr        & 0x3u;
     unsigned int clust = (mpidr >>  8u) & 0xFu;
 
     printf("[arm32] CPU[%u:%u] MIDR=0x%08x (part=0x%03x r%up%u)\n",
            clust, cpu, midr, part, var, rev);
 }
 
 /* =========================================================================
  * 2. Cache enable  (ISA — CP15 SCTLR, same instruction on all ARMv7-A)
  * ====================================================================== */
 static void arm32_cache_enable(void)
 {
     unsigned int sctlr;
     __asm__ volatile("mrc p15,0,%0,c1,c0,0" : "=r"(sctlr));
     sctlr |= SCTLR_I | SCTLR_C;               /* I-cache + D-cache     */
     __asm__ volatile("mcr p15,0,%0,c1,c0,0" :: "r"(sctlr) : "memory");
     arch_isb();
     printf("[arm32] I/D caches enabled (SCTLR=0x%08x)\n", sctlr);
 }
 
 /* =========================================================================
  * 3. SMP coherency bit  (ISA — CP15 ACTLR)
  * ====================================================================== */
 static void arm32_smp_enable(void)
 {
     unsigned int actlr;
     __asm__ volatile("mrc p15,0,%0,c1,c0,1" : "=r"(actlr));
     if (!(actlr & (1u << 6))) {                /* ACTLR.SMP             */
         actlr |= (1u << 6);
         __asm__ volatile("mcr p15,0,%0,c1,c0,1" :: "r"(actlr) : "memory");
         arch_isb();
         arch_dsb_sy();
     }
     printf("[arm32] ACTLR.SMP set (0x%08x)\n", actlr);
 }
 
 /* =========================================================================
  * 4. GIC-400 initialisation
  *    Register layout is architecture-defined (ARM GIC spec).
  *    Base addresses come from arch_defs.h (QEMU default) and may be
  *    overridden by 03_SoC/include/uiox_soc_map.h per chip.
  * ====================================================================== */
 static void arm32_gic_init(void)
 {
     /* Disable distributor before configuration */
     mmio_write32(GIC_DIST_CTLR, 0x0u);
 
     /* Enable all SPI interrupt lines (banks 1 and 2 for IRQs 32–95) */
     mmio_write32(GIC_DIST_ISENABLER0 + 0x04u, 0xFFFFFFFFu);
     mmio_write32(GIC_DIST_ISENABLER0 + 0x08u, 0xFFFFFFFFu);
 
     /* Set all priorities to a safe mid-level value */
     for (unsigned int i = 0u; i < 64u; i++)
         mmio_write32(GIC_DIST_IPRIORITYR0 + i * 4u, 0xA0A0A0A0u);
 
     /* CPU interface: allow all priority levels, then enable */
     mmio_write32(GIC_CPU_PMR,  0xFFu);
     mmio_write32(GIC_CPU_CTLR, 0x1u);
 
     /* Re-enable distributor */
     mmio_write32(GIC_DIST_CTLR, 0x1u);
 
     printf("[arm32] GIC-400 init (DIST=0x%08lx CPU=0x%08lx)\n",
            (unsigned long)GIC_DIST_BASE,
            (unsigned long)GIC_CPU_BASE);
 }
 
 /* =========================================================================
  * 5. VBAR — Exception Vector Base Address Register  (ISA register)
  *    The actual vector table is built by the kernel (33_ProcessControlSubsystem).
  *    arch_init installs a minimal reset-safe table.
  * ====================================================================== */
 extern unsigned int _vector_table[];   /* defined in boot.S / linker script */
 
 static void arm32_vbar_install(void)
 {
     /* VBAR is a CP15 register — same on every ARMv7-A implementation */
     __asm__ volatile("mcr p15,0,%0,c12,c0,0"
                      :: "r"((unsigned int)_vector_table) : "memory");
     arch_isb();
     printf("[arm32] VBAR installed @ 0x%08x\n",
            (unsigned int)_vector_table);
 }
 
 /* =========================================================================
  * arch_init — called by uiox_kernel_main() before uiox_soc_init()
  *
  * Returns after ISA-level setup is done.
  * uiox_soc_init() is called by the kernel immediately after to do
  * the SoC-level work (clocks, UART baud, PM, PCIe etc.)
  * ====================================================================== */
 void arch_init(void)
 {
     printf("\n[arch_init] *** ARM32 (ARMv7-A) ***\n");
 
     /* 1. Identify CPU */
     arm32_cpu_identify();
 
     /* 2. Enable caches (ISA operation) */
     arm32_cache_enable();
 
     /* 3. Enable SMP coherency bit (ISA operation) */
     arm32_smp_enable();
 
     /* 4. Configure GIC-400 (architecture-defined register layout) */
     arm32_gic_init();
 
     /* 5. Install exception vector table (ISA register) */
     arm32_vbar_install();
 
     /* 6. Initialise driver-interface layer */
     mmio_init();
     irq_init();
 
     /* 7. Register IRQ handlers
      *    Handler implementations live in 20_DriverInterfaces or 03_SoC.
      *    IRQ numbers come from arch_defs.h (architecture default) and
      *    may be overridden by uiox_soc_map.h per chip.            */
     irq_request(UART0_IRQ,  uart0_irq_handler,  NULL, "uart0");
     irq_request(TIMER0_IRQ, timer0_irq_handler, NULL, "timer0");
 
     irq_enable(UART0_IRQ);
     irq_enable(TIMER0_IRQ);
 
     /* 8. Unmask global CPU interrupts (ISA operation — CPSR.I clear) */
     cpu_irq_enable();
 
     printf("[arch_init] ARM32 architecture layer ready\n");
     /* uiox_soc_init() follows immediately in uiox_kernel_main() */
 }
 
 void arch_fini(void)
 {
     cpu_irq_disable();
     irq_free(UART0_IRQ);
     irq_free(TIMER0_IRQ);
     printf("[arch_fini] ARM32 architecture layer torn down\n");
 }
 