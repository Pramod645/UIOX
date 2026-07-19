/*
 * 10_Arch/riscv64/src/arch_init.c
 * RISC-V RV64 platform initialisation.
 *
 * Uses arch_csrr_* / arch_csrw_* inline functions from arch_defs.h
 * (no statement-expression macros — clean with -Wpedantic).
 */

 #include "arch_defs.h"
 #include "../../../20_DriverInterfaces/include/hw_types.h"
 #include "../../../20_DriverInterfaces/include/mmio.h"
 #include "../../../20_DriverInterfaces/include/irq.h"
 #include "../../../20_DriverInterfaces/include/cpu.h"
 #include "../../../03_SoC/include/uiox_soc_stdio.h"
 #include "../../../03_SoC/include/uiox_soc_string.h"
 
 /* ── PLIC context helper ─────────────────────────────────── */
 /* S-mode context for hart N = 2*N + 1 */
 #define PLIC_CTX_SMODE(hart)    (2u * (hart) + 1u)
 
 /* ── UART handler (NS16550A) ─────────────────────────────── */
 static void riscv_uart_handler(int irq, hw_context_t *ctx, void *id)
 {
     (void)ctx; (void)id;
     unsigned char rx = (unsigned char)mmio_read32(UART0_BASE + 0x00u);
     printf("  [riscv/uart_irq] IRQ%d rx=0x%02x '%c'\n",
            irq, rx & 0xFFu,
            (rx >= 0x20u && rx < 0x7Fu) ? (char)rx : '.');
     /* Complete PLIC claim */
     mmio_write32(PLIC_CLAIM(PLIC_CTX_SMODE(0u)), (unsigned int)irq);
 }
 
 /* ── Timer handler (CLINT mtime) ─────────────────────────── */
 static void riscv_timer_handler(int irq, hw_context_t *ctx, void *id)
 {
     (void)ctx; (void)id;
     /* Reload MTIMECMP for hart 0 */
     unsigned long mtime = *(volatile unsigned long *)(unsigned long)CLINT_MTIME;
     unsigned long freq  = 10000000UL;   /* 10 MHz nominal */
     *(volatile unsigned long *)(unsigned long)CLINT_MTIMECMP(0u) =
         mtime + freq / 100u;            /* 100 Hz tick    */
     printf("  [riscv/timer_irq] IRQ%d tick\n", irq);
 }
 
 /* ── CLINT init ──────────────────────────────────────────── */
 static void riscv_clint_init(void)
 {
     mmio_write32(CLINT_MSIP(0u), 0u);
     mmio_write32(CLINT_MTIMECMP(0u),      0xFFFFFFFFu);
     mmio_write32(CLINT_MTIMECMP(0u) + 4u, 0xFFFFFFFFu);
     printf("[riscv] CLINT @ 0x%08lx cleared\n", (unsigned long)CLINT_BASE);
 }
 
 /* ── PLIC init ───────────────────────────────────────────── */
 static void riscv_plic_init(void)
 {
     /* Set UART IRQ priority = 1 */
     mmio_write32(PLIC_PRIORITY(UART0_IRQ), 1u);
 
     /* Enable UART for hart 0 S-mode */
     unsigned int ctx  = PLIC_CTX_SMODE(0u);
     unsigned int word = UART0_IRQ / 32u;
     unsigned int bit  = UART0_IRQ % 32u;
     unsigned int en   = mmio_read32(PLIC_ENABLE(ctx, word));
     mmio_write32(PLIC_ENABLE(ctx, word), en | (1u << bit));
     mmio_write32(PLIC_THRESHOLD(ctx), 0u);  /* allow all priorities */
 
     printf("[riscv] PLIC @ 0x%08lx UART_IRQ=%u enabled\n",
            (unsigned long)PLIC_BASE, (unsigned int)UART0_IRQ);
 }
 
 /* ── NS16550A UART init ──────────────────────────────────── */
 static void riscv_uart_init(void)
 {
     mmio_write32(UART0_BASE + 0x04u, 0x00u);  /* IER: off          */
     mmio_write32(UART0_BASE + 0x03u, 0x83u);  /* LCR: 8N1 + DLAB  */
     mmio_write32(UART0_BASE + 0x00u, 0x01u);  /* DLL: divisor lo  */
     mmio_write32(UART0_BASE + 0x01u, 0x00u);  /* DLH: divisor hi  */
     mmio_write32(UART0_BASE + 0x03u, 0x03u);  /* LCR: 8N1, no DLAB*/
     mmio_write32(UART0_BASE + 0x02u, 0xC7u);  /* FCR: FIFO enable */
     mmio_write32(UART0_BASE + 0x04u, 0x01u);  /* IER: RX interrupt*/
     printf("[riscv] NS16550A UART @ 0x%08lx 115200 8N1\n",
            (unsigned long)UART0_BASE);
 }
 
 /* ── arch_init ───────────────────────────────────────────── */
 void arch_init(void)
 {
     printf("[riscv] arch_init start\n");
 
     riscv_clint_init();
     riscv_plic_init();
     riscv_uart_init();
 
     irq_register(UART0_IRQ,           riscv_uart_handler,  NULL);
     irq_register(ARCH_TIMER_IRQ_PHYS, riscv_timer_handler, NULL);
 
     /* Enable S-mode external interrupts (PLIC) and timer */
     arch_csrs_sie(SIE_SEIE | SIE_STIE);
 
     printf("[riscv] arch_init done\n");
 }
 