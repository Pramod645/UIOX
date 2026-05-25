/*
 * arch/arm64/src/arch_init.c
 *
 * ARM64 (AArch64) platform initialisation.
 *
 * Called from main() before any uiox_fs / uiox_dev / uiox_hw
 * subsystem is started.  Performs:
 *
 *   1. GIC-400 distributor + CPU interface init
 *   2. PL011 UART init (115200 8N1)
 *   3. SP804 timer init (100 Hz)
 *   4. Exception vector table installation  (VBAR_EL1)
 *   5. IRQ handler registration for UART, timer, disk
 *   6. CPU info dump
 */

 #include "arch_defs.h"          /* arch/arm64/include/             */
 #include "../../../20_DriverInterfaces/include/hw_types.h"
 #include "../../../20_DriverInterfaces/include/mmio.h"
 #include "../../../20_DriverInterfaces/include/irq.h"
 #include "../../../20_DriverInterfaces/include/cpu.h"
 #include <stdio.h>
 #include <string.h>
 
 /* =============================================================
  * Forward declarations for platform interrupt handlers
  * ============================================================= */
 static void arm64_uart_handler(int irq, hw_context_t *ctx, void *id);
 static void arm64_timer_handler(int irq, hw_context_t *ctx, void *id);
 static void arm64_disk_handler (int irq, hw_context_t *ctx, void *id);
 
 /* =============================================================
  * 1.  GIC-400 initialisation
  * ============================================================= */
 static void gic_init(void)
 {
     printf("[arm64] GIC init: DIST=0x%08lx  CPU=0x%08lx\n",
            (unsigned long)GIC_DIST_BASE,
            (unsigned long)GIC_CPU_BASE);
 
     /*
      * Distributor:
      *   Enable Group 0 and Group 1 interrupts (GICD_CTLR = 0x3).
      *   Enable specific SPI lines for UART, timer, disk.
      */
     mmio_write32(GIC_DIST_CTLR, 0x3u);
 
     /* Enable UART SPI (IRQ 33 → bit 1 of ISENABLER1) */
     mmio_write32(GIC_DIST_ISENABLER0 + 4u, 1u << (UART0_IRQ  - 32));
 
     /* Enable Timer SPI (IRQ 34 → bit 2 of ISENABLER1) */
     mmio_write32(GIC_DIST_ISENABLER0 + 4u, 1u << (TIMER0_IRQ - 32));
 
     /* Enable VirtIO-blk SPI (IRQ 48 → ISENABLER1 bit 16) */
     mmio_write32(GIC_DIST_ISENABLER0 + 4u, 1u << (VIRTIO_BLK_IRQ - 32));
 
     /*
      * CPU interface:
      *   Set priority mask to 0xFF (accept all priorities).
      *   Enable CPU interface (GICC_CTLR = 0x1).
      */
     mmio_write32(GIC_CPU_BASE + 0x004, 0xFFu);  /* GICC_PMR  */
     mmio_write32(GIC_CPU_CTLR,         0x1u);   /* GICC_CTLR */
 
     printf("[arm64] GIC enabled\n");
 }
 
 /* =============================================================
  * 2.  PL011 UART initialisation  (115200 8N1)
  * ============================================================= */
 static void uart_init(void)
 {
     /*
      * Assuming 24 MHz UARTCLK:
      *   IBRD = 13, FBRD = 1  → 115200 baud
      * LCR_H: WLEN=8 (0x60), FEN (FIFO enable, 0x10) → 0x70
      * CR:    TXE | RXE | UARTEN → 0x301
      */
     mmio_write32(UART0_CR,   0u);          /* disable UART       */
     mmio_write32(UART0_IBRD, 13u);
     mmio_write32(UART0_FBRD, 1u);
     mmio_write32(UART0_LCR_H, 0x70u);     /* 8N1, FIFO on       */
     mmio_write32(UART0_IMSC,  0x10u);      /* enable RX interrupt */
     mmio_write32(UART0_CR,   0x301u);      /* TX+RX+enable       */
 
     printf("[arm64] UART0 init: 115200 8N1  base=0x%08lx\n",
            (unsigned long)UART0_BASE);
 }
 
 /* =============================================================
  * 3.  SP804 timer initialisation  (100 Hz)
  * ============================================================= */
 static void sp804_init(uint32_t hz)
 {
     /*
      * SP804 runs on a 1 MHz reference clock in QEMU virt.
      * Load = 1000000 / hz
      */
     uint32_t load = 1000000u / hz;
 
     mmio_write32(TIMER0_LOAD, load);
     /*
      * Control: enable(7) | periodic(6) | IRQ-enable(5) | 32-bit(1)
      *          = 0b11100010 = 0xE2
      */
     mmio_write32(TIMER0_CTRL, 0xE2u);
 
     printf("[arm64] SP804 timer init: %u Hz  load=%u\n", hz, load);
 }
 
 /* =============================================================
  * 4.  Exception vector table  (VBAR_EL1)
  *
  * On a real target this is an assembly file (vectors.S) aligned
  * to 2 KB.  Here we register software-level IRQ handlers through
  * the uiox_hw IRQ layer instead, which is functionally equivalent
  * for the simulation.
  * ============================================================= */
 static void vbar_init(void)
 {
     /*
      * Real: write_sysreg(vector_table_addr, vbar_el1);
      *       arch_isb();
      *
      * Simulation: the uiox_hw irq_dispatch() function plays the
      * role of the hardware vector table; we just log the intent.
      */
     printf("[arm64] VBAR_EL1: vector table installed "
            "(simulated via irq_dispatch)\n");
 }
 
 /* =============================================================
  * 5.  IRQ handler registration
  * ============================================================= */
 static void arm64_uart_handler(int irq, hw_context_t *ctx, void *id)
 {
     (void)ctx; (void)id;
     uint32_t rx = mmio_read32(UART0_DR);
     printf("  [arm64/uart_irq] IRQ%d rx=0x%02x '%c'\n",
            irq, rx & 0xFF,
            (rx >= 0x20 && rx < 0x7F) ? (char)rx : '.');
     mmio_write32(UART0_ICR, 0x7FFu);   /* clear all pending     */
 }
 
 static void arm64_timer_handler(int irq, hw_context_t *ctx, void *id)
 {
     (void)ctx; (void)id;
     mmio_write32(TIMER0_INTCLR, 1u);   /* clear timer interrupt */
     printf("  [arm64/timer_irq] IRQ%d tick\n", irq);
 }
 
 static void arm64_disk_handler(int irq, hw_context_t *ctx, void *id)
 {
     (void)ctx; (void)id;
     printf("  [arm64/disk_irq] IRQ%d virtio-blk complete\n", irq);
 }
 
 /* =============================================================
  * arch_init  —  public entry point
  * ============================================================= */
 void arch_init(void)
 {
     printf("\n[arch_init] *** ARM64 (AArch64) platform ***\n");
 
     /* CPU info */
     cpu_info_t info;
     cpu_identify(&info);
     cpu_print_info(&info);
 
     /* MMIO regions */
     mmio_init();
 
     /* IRQ subsystem */
     irq_init();
 
     /* Platform hardware */
     gic_init();
     uart_init();
     sp804_init(100);
     vbar_init();
 
     /* Register IRQ handlers */
     irq_request(UART0_IRQ,      arm64_uart_handler,  NULL, "uart0");
     irq_request(TIMER0_IRQ,     arm64_timer_handler, NULL, "timer0");
     irq_request(VIRTIO_BLK_IRQ, arm64_disk_handler,  NULL, "virtio-blk");
 
     irq_enable(UART0_IRQ);
     irq_enable(TIMER0_IRQ);
     irq_enable(VIRTIO_BLK_IRQ);
 
     /* Enable CPU IRQs */
     cpu_irq_enable();
 
     printf("[arch_init] ARM64 platform ready\n");
 }
 
 /* arch_fini — teardown (called at end of main) */
 void arch_fini(void)
 {
     cpu_irq_disable();
     irq_free(UART0_IRQ);
     irq_free(TIMER0_IRQ);
     irq_free(VIRTIO_BLK_IRQ);
     printf("[arch_fini] ARM64 platform torn down\n");
 }
 