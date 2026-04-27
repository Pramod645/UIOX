/*
 * arch/arm32/src/arch_init.c
 *
 * ARMv7-A 32-bit platform initialisation.
 */

 #include "arch_defs.h"
 #include "../../../20_DriverInterfaces/include/hw_types.h"
 #include "../../../20_DriverInterfaces/include/mmio.h"
 #include "../../../20_DriverInterfaces/include/irq.h"
 #include "../../../20_DriverInterfaces/include/cpu.h"
 #include <stdio.h>
 #include <string.h>
 
 static void arm32_uart_handler(int irq, hw_context_t *ctx, void *id)
 {
     (void)ctx; (void)id;
     uint32_t rx = mmio_read32(UART0_DR);
     printf("  [arm32/uart_irq] IRQ%d rx=0x%02x '%c'\n",
            irq, rx & 0xFF,
            (rx >= 0x20 && rx < 0x7F) ? (char)rx : '.');
     mmio_write32(UART0_ICR, 0x7FFu);
 }
 
 static void arm32_timer_handler(int irq, hw_context_t *ctx, void *id)
 {
     (void)ctx; (void)id;
     mmio_write32(TIMER0_INTCLR, 1u);
     printf("  [arm32/timer_irq] IRQ%d tick\n", irq);
 }
 
 static void arm32_disk_handler(int irq, hw_context_t *ctx, void *id)
 {
     (void)ctx; (void)id;
     printf("  [arm32/disk_irq] IRQ%d IDE complete\n", irq);
 }
 
 static void gic_init_arm32(void)
 {
     mmio_write32(GIC_DIST_CTLR,         0x1u);
     mmio_write32(GIC_DIST_ISENABLER0 + 4u,
                  (1u << (UART0_IRQ  - 32)) |
                  (1u << (TIMER0_IRQ - 32)));
     mmio_write32(GIC_CPU_BASE + 0x004,  0xFFu);
     mmio_write32(GIC_CPU_CTLR,          0x1u);
     printf("[arm32] GIC enabled\n");
 }
 
 static void uart_init_arm32(void)
 {
     mmio_write32(UART0_CR,    0u);
     mmio_write32(UART0_IBRD,  13u);
     mmio_write32(UART0_FBRD,  1u);
     mmio_write32(UART0_LCR_H, 0x70u);
     mmio_write32(UART0_IMSC,  0x10u);
     mmio_write32(UART0_CR,    0x301u);
     printf("[arm32] UART0 init: 115200 8N1\n");
 }
 
 static void sp804_init_arm32(uint32_t hz)
 {
     uint32_t load = 1000000u / hz;
     mmio_write32(TIMER0_LOAD, load);
     mmio_write32(TIMER0_CTRL, 0xE2u);
     printf("[arm32] SP804 timer: %u Hz\n", hz);
 }
 
 void arch_init(void)
 {
     printf("\n[arch_init] *** ARM32 (ARMv7-A) platform ***\n");
 
     cpu_info_t info;
     cpu_identify(&info);
     cpu_print_info(&info);
 
     mmio_init();
     irq_init();
 
     gic_init_arm32();
     uart_init_arm32();
     sp804_init_arm32(100);
 
     printf("[arm32] VBAR installed (simulated via irq_dispatch)\n");
 
     irq_request(UART0_IRQ,  arm32_uart_handler,  NULL, "uart0");
     irq_request(TIMER0_IRQ, arm32_timer_handler, NULL, "timer0");
     irq_request(IDE_IRQ,    arm32_disk_handler,  NULL, "ide");
 
     irq_enable(UART0_IRQ);
     irq_enable(TIMER0_IRQ);
     irq_enable(IDE_IRQ);
 
     cpu_irq_enable();
     printf("[arch_init] ARM32 platform ready\n");
 }
 
 void arch_fini(void)
 {
     cpu_irq_disable();
     irq_free(UART0_IRQ);
     irq_free(TIMER0_IRQ);
     irq_free(IDE_IRQ);
     printf("[arch_fini] ARM32 platform torn down\n");
 }
 