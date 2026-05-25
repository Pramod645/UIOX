/*
 * io.c  —  x86_64 PIC (8259A) and serial port setup.
 *
 * Mirrors: 10_Arch/arm32/src/io.c
 */

 #include "../include/arch.h"

 /* ── 8259A PIC initialisation ────────────────────────────── */
 /*
  * pic_init()
  * Remaps both PICs so that:
  *   Master IRQ0–7  → vectors offset1 … offset1+7
  *   Slave  IRQ8–15 → vectors offset2 … offset2+7
  *
  * ICW1 → ICW2 → ICW3 → ICW4 sequence (Intel 8259A datasheet)
  */
 void pic_init(uint8_t offset1, uint8_t offset2)
 {
     /* save masks */
     uint8_t mask1 = inb(PIC1_DATA);
     uint8_t mask2 = inb(PIC2_DATA);
 
     /* ICW1: start init, edge triggered, cascade, need ICW4 */
     outb(PIC1_CMD,  0x11);  io_delay();
     outb(PIC2_CMD,  0x11);  io_delay();
 
     /* ICW2: vector offset */
     outb(PIC1_DATA, offset1); io_delay();
     outb(PIC2_DATA, offset2); io_delay();
 
     /* ICW3: master has slave on IRQ2; slave id = 2 */
     outb(PIC1_DATA, 0x04);  io_delay();
     outb(PIC2_DATA, 0x02);  io_delay();
 
     /* ICW4: 8086 mode */
     outb(PIC1_DATA, 0x01);  io_delay();
     outb(PIC2_DATA, 0x01);  io_delay();
 
     /* restore saved masks */
     outb(PIC1_DATA, mask1);
     outb(PIC2_DATA, mask2);
 }
 
 /* ── EOI ─────────────────────────────────────────────────── */
 void pic_send_eoi(uint8_t irq)
 {
     if (irq >= 8)
         outb(PIC2_CMD, PIC_EOI);
     outb(PIC1_CMD,  PIC_EOI);
 }
 
 /* ── IRQ masking ─────────────────────────────────────────── */
 void pic_mask_irq(uint8_t irq)
 {
     uint16_t port;
     uint8_t  val;
     if (irq < 8) {
         port = PIC1_DATA;
     } else {
         port = PIC2_DATA;
         irq -= 8;
     }
     val  = inb(port) | (uint8_t)(1 << irq);
     outb(port, val);
 }
 
 void pic_unmask_irq(uint8_t irq)
 {
     uint16_t port;
     uint8_t  val;
     if (irq < 8) {
         port = PIC1_DATA;
     } else {
         port = PIC2_DATA;
         irq -= 8;
     }
     val  = inb(port) & (uint8_t)~(1 << irq);
     outb(port, val);
 }
 