/*
 * 10_Arch/arm64/src/arch_init.c
 * ARMv8-A 64-bit platform initialisation.
 *
 * Responsibilities (architecture layer only):
 *   1. Enable I/D caches (SCTLR_EL1)
 *   2. Configure GIC-400 distributor + CPU interface
 *   3. Configure PL011 UART (baud divisors from arch_defs)
 *   4. Register IRQ handlers via 20_DriverInterfaces
 *   5. Enable generic timer tick
 *
 * Does NOT touch:
 *   - UART base address selection (SoC layer — 03_SoC)
 *   - Clock PLL configuration   (SoC layer)
 *   - Power domains             (SoC layer)
 */

 #include "arch_defs.h"
 #include "../../../20_DriverInterfaces/include/hw_types.h"
 #include "../../../20_DriverInterfaces/include/mmio.h"
 #include "../../../20_DriverInterfaces/include/irq.h"
 #include "../../../20_DriverInterfaces/include/cpu.h"
 #include "../../../03_SoC/include/uiox_soc_stdio.h"   /* replaces <stdio.h>  */
 #include "../../../03_SoC/include/uiox_soc_string.h"  /* replaces <string.h> */
 
 /* =========================================================================
  * Internal helpers
  * ====================================================================== */
 
 static void arm64_cache_enable(void)
 {
     unsigned long sctlr;
     __asm__ volatile("mrs %0, sctlr_el1" : "=r"(sctlr));
     sctlr |= SCTLR_EL1_I | SCTLR_EL1_C;
     __asm__ volatile("msr sctlr_el1, %0" :: "r"(sctlr) : "memory");
     arch_isb();
 }
 
 /* =========================================================================
  * GIC-400 initialisation
  * Uses GIC_DIST_BASE / GIC_CPU_BASE from arch_defs.h.
  * ====================================================================== */
 static void arm64_gic_init(void)
 {
     /* 1. Disable distributor */
     mmio_write32(GIC_DIST_CTLR, 0x0u);
 
     /* 2. Enable all SPIs */
     mmio_write32(GIC_DIST_ISENABLER0, 0xFFFFFFFFu);
 
     /* 3. Set all priorities to mid-level */
     for (unsigned int i = 0u; i < 64u; i++)
         mmio_write32(GIC_DIST_IPRIORITYR0 + i * 4u, 0xA0A0A0A0u);
 
     /* 4. CPU interface: allow all priorities, enable */
     mmio_write32(GIC_CPU_PMR,  0xFFu);
     mmio_write32(GIC_CPU_CTLR, 0x1u);
 
     /* 5. Re-enable distributor */
     mmio_write32(GIC_DIST_CTLR, 0x1u);
 
     printf("[arm64] GIC-400 init (DIST=0x%08lx CPU=0x%08lx)\n",
            (unsigned long)GIC_DIST_BASE,
            (unsigned long)GIC_CPU_BASE);
 }
 
 /* =========================================================================
  * PL011 UART IRQ handler
  * ====================================================================== */
 static void arm64_uart_handler(int irq, hw_context_t *ctx, void *id)
 {
     (void)ctx; (void)id;
     unsigned int rx = mmio_read32(UART0_BASE + 0x000u); /* DR */
     printf("  [arm64/uart_irq] IRQ%d rx=0x%02x '%c'\n",
            irq, rx & 0xFFu,
            (rx >= 0x20u && rx < 0x7Fu) ? (char)rx : '.');
     mmio_write32(UART0_BASE + 0x044u, 0x7FFu);          /* ICR */
 }
 
 /* =========================================================================
  * Generic timer IRQ handler
  * ====================================================================== */
 static void arm64_timer_handler(int irq, hw_context_t *ctx, void *id)
 {
     (void)ctx; (void)id;
     /*
      * Reload the timer: write CNTP_TVAL_EL0 = freq / 100 (100 Hz).
      * Stub: just log — real reload in timer driver.
      */
     printf("  [arm64/timer_irq] IRQ%d tick\n", irq);
 }
 
 /* =========================================================================
  * PL011 UART initialisation (115200 8N1, 24 MHz ref clock)
  * ====================================================================== */
 static void arm64_uart_init(void)
 {
     /* PL011 registers at UART0_BASE (defined in arch_defs.h) */
     mmio_write32(UART0_BASE + 0x030u, 0x0u);    /* CR:   disable         */
     mmio_write32(UART0_BASE + 0x024u, 13u);      /* IBRD: 24MHz/115200/16 */
     mmio_write32(UART0_BASE + 0x028u, 1u);       /* FBRD                  */
     mmio_write32(UART0_BASE + 0x02Cu, 0x70u);    /* LCR_H: 8N1 + FIFO    */
     mmio_write32(UART0_BASE + 0x038u, (1u<<4));  /* IMSC: RX interrupt    */
     mmio_write32(UART0_BASE + 0x030u, 0x301u);   /* CR:   TX+RX+EN        */
     printf("[arm64] PL011 UART @ 0x%08lx 115200 8N1\n",
            (unsigned long)UART0_BASE);
 }
 
 /* =========================================================================
  * Generic timer initialisation (100 Hz tick)
  * ====================================================================== */
 static void arm64_timer_init(void)
 {
     unsigned long freq;
     __asm__ volatile("mrs %0, cntfrq_el0" : "=r"(freq));
     unsigned long tval = freq / 100u;   /* 100 Hz */
     __asm__ volatile("msr cntp_tval_el0, %0" :: "r"(tval));
     __asm__ volatile("msr cntp_ctl_el0,  %0" :: "r"(1UL));
     printf("[arm64] Generic timer: %lu Hz / 100 = %lu ticks\n", freq, tval);
 }
 
 /* =========================================================================
  * arch_init — called from the SoC bootstrap (03_SoC/uiox_soc_main.c)
  * ====================================================================== */
 void arch_init(void)
 {
     printf("[arm64] arch_init start\n");
 
     arm64_cache_enable();
     arm64_gic_init();
     arm64_uart_init();
     arm64_timer_init();
 
     /* Register IRQ handlers via 20_DriverInterfaces */
     irq_register(ARCH_TIMER_IRQ_PHYS, arm64_timer_handler, NULL);
     irq_register(UART0_IRQ,           arm64_uart_handler,  NULL);
     irq_global_enable();
 
     printf("[arm64] arch_init done\n");
 }
 