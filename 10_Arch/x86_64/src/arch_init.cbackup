/*
 * arch/x86_64/src/arch_init.c
 *
 * x86_64 platform initialisation.
 *
 *   1. 8259A PIC remap (IRQs 0-15 → vectors 0x20-0x2F)
 *   2. 8254 PIT timer  (100 Hz)
 *   3. 16550 UART COM1 init (115200 8N1)
 *   4. LAPIC enable
 *   5. IRQ handler registration
 */

 #include "arch_defs.h"
 //#include "../../../uiox_hw/include/hw_types.h"
 #include "../../../20_DriverInterfaces/include/hw_types.h"
 #include "../../../20_DriverInterfaces/include/mmio.h"
 #include "../../../20_DriverInterfaces/include/irq.h"
 #include "../../../20_DriverInterfaces/include/cpu.h"
 #include <stdio.h>
 #include <string.h>
 
 /* =============================================================
  * IRQ handlers
  * ============================================================= */
 static void x86_uart_handler(int irq, hw_context_t *ctx, void *id)
 {
     (void)ctx; (void)id;
     uint8_t ch = port_inb(COM1_PORT);
     printf("  [x86/uart_irq] IRQ%d rx=0x%02x '%c'\n",
            irq, ch, (ch >= 0x20 && ch < 0x7F) ? ch : '.');
     /* EOI to 8259A PIC */
     port_outb(PIC1_CMD, PIC_EOI);
 }
 
 static void x86_timer_handler(int irq, hw_context_t *ctx, void *id)
 {
     (void)ctx; (void)id;
     printf("  [x86/timer_irq] IRQ%d PIT tick\n", irq);
     port_outb(PIC1_CMD, PIC_EOI);
 }
 
 static void x86_disk_handler(int irq, hw_context_t *ctx, void *id)
 {
     (void)ctx; (void)id;
     uint8_t st = port_inb(IDE_STATUS_PORT);
     printf("  [x86/disk_irq] IRQ%d IDE status=0x%02x\n", irq, st);
     port_outb(PIC1_CMD, PIC_EOI);
 }
 
 /* =============================================================
  * 1. 8259A PIC remap
  * ============================================================= */
 static void pic_remap(void)
 {
     /* ICW1: start init sequence, edge triggered, cascade */
     port_outb(PIC1_CMD,  0x11);
     port_outb(PIC2_CMD,  0x11);
     /* ICW2: vector offsets */
     port_outb(PIC1_DATA, 0x20);  /* IRQ0-7  → INT 0x20-0x27 */
     port_outb(PIC2_DATA, 0x28);  /* IRQ8-15 → INT 0x28-0x2F */
     /* ICW3 */
     port_outb(PIC1_DATA, 0x04);  /* PIC1 slave at IRQ2       */
     port_outb(PIC2_DATA, 0x02);
     /* ICW4: 8086 mode */
     port_outb(PIC1_DATA, 0x01);
     port_outb(PIC2_DATA, 0x01);
     /* Mask all except timer, keyboard, cascade, COM1, IDE */
     port_outb(PIC1_DATA, 0xB8);  /* unmask 0,1,2,6           */
     port_outb(PIC2_DATA, 0xBF);  /* unmask 14                */
     printf("[x86] 8259A PIC remapped: IRQ0-7→0x20, IRQ8-15→0x28\n");
 }
 
 /* =============================================================
  * 2. 8254 PIT timer (100 Hz)
  * ============================================================= */
 static void pit_init(uint32_t hz)
 {
     /* Channel 0, mode 3 (square wave), binary */
     uint16_t divisor = (uint16_t)(1193182u / hz);
     port_outb(PIT_CMD_PORT, 0x36);
     port_outb(PIT_CH0_PORT, (uint8_t)(divisor & 0xFF));
     port_outb(PIT_CH0_PORT, (uint8_t)(divisor >> 8));
     printf("[x86] PIT init: %u Hz  divisor=%u\n", hz, divisor);
 }
 
 /* =============================================================
  * 3. 16550A UART COM1 (115200 8N1)
  * ============================================================= */
 static void uart_init_x86(void)
 {
     port_outb(COM1_PORT + 1, 0x00);  /* disable interrupts     */
     port_outb(COM1_PORT + 3, 0x80);  /* DLAB on                */
     port_outb(COM1_PORT + 0, 0x01);  /* divisor LSB → 115200   */
     port_outb(COM1_PORT + 1, 0x00);  /* divisor MSB            */
     port_outb(COM1_PORT + 3, 0x03);  /* 8N1, DLAB off          */
     port_outb(COM1_PORT + 2, 0xC7);  /* FIFO, clear, 14-byte   */
     port_outb(COM1_PORT + 4, 0x0B);  /* RTS, DTR, IRQ on       */
     port_outb(COM1_PORT + 1, 0x01);  /* enable RX interrupt    */
     printf("[x86] COM1 init: 115200 8N1  port=0x%03x\n", COM1_PORT);
 }
 
 /* =============================================================
  * 4. LAPIC enable (basic)
  * ============================================================= */
 static void lapic_init(void)
 {
     /*
      * Set spurious interrupt vector (0xFF) and enable LAPIC.
      * Bit 8 of LAPIC_SPURIOUS = APIC Software Enable.
      */
     mmio_write32(LAPIC_SPURIOUS, 0x1FFu);
     printf("[x86] LAPIC enabled  base=0x%08lx\n",
            (unsigned long)LAPIC_BASE);
 }
 
 /* =============================================================
  * arch_init — public entry point
  * ============================================================= */
 void arch_init(void)
 {
     printf("\n[arch_init] *** x86_64 platform ***\n");
 
     cpu_info_t info;
     cpu_identify(&info);
     cpu_print_info(&info);
 
     mmio_init();
     irq_init();
 
     pic_remap();
     pit_init(100);
     uart_init_x86();
     lapic_init();
 
     irq_request(PIT_IRQ,  x86_timer_handler, NULL, "pit-timer");
     irq_request(COM1_IRQ, x86_uart_handler,  NULL, "com1");
     irq_request(IDE_IRQ,  x86_disk_handler,  NULL, "ide");
 
     irq_enable(PIT_IRQ);
     irq_enable(COM1_IRQ);
     irq_enable(IDE_IRQ);
 
     cpu_irq_enable();
     printf("[arch_init] x86_64 platform ready\n");
 }
 
 void arch_fini(void)
 {
     cpu_irq_disable();
     irq_free(PIT_IRQ);
     irq_free(COM1_IRQ);
     irq_free(IDE_IRQ);
     printf("[arch_fini] x86_64 platform torn down\n");
 }
 