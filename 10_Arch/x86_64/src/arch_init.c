/*
 * 10_Arch/x86_64/src/arch_init.c
 * AMD64 / Intel 64-bit platform initialisation.
 */

 #include "arch_defs.h"
 #include "../../../20_DriverInterfaces/include/hw_types.h"
 #include "../../../20_DriverInterfaces/include/mmio.h"
 #include "../../../20_DriverInterfaces/include/irq.h"
 #include "../../../20_DriverInterfaces/include/cpu.h"
 #include "../../../03_SoC/include/uiox_soc_stdio.h"
 #include "../../../03_SoC/include/uiox_soc_string.h"
 
 /* ── 8259A PIC remap ─────────────────────────────────────── */
 static void x86_pic_remap(void)
 {
     /* Remap master: IRQ 0-7 → vectors 0x20-0x27
      * Remap slave:  IRQ 8-15 → vectors 0x28-0x2F */
     arch_outb(0x20, 0x11);  /* ICW1: init + ICW4 needed */
     arch_outb(0xA0, 0x11);
     arch_outb(0x21, 0x20);  /* ICW2: master base 0x20   */
     arch_outb(0xA1, 0x28);  /* ICW2: slave  base 0x28   */
     arch_outb(0x21, 0x04);  /* ICW3: slave on IRQ2       */
     arch_outb(0xA1, 0x02);
     arch_outb(0x21, 0x01);  /* ICW4: 8086 mode           */
     arch_outb(0xA1, 0x01);
     arch_outb(0x21, 0xFF);  /* mask all (LAPIC takes over)*/
     arch_outb(0xA1, 0xFF);
     printf("[x86] 8259A PIC remapped and masked\n");
 }
 
 /* ── Local APIC enable ───────────────────────────────────── */
 static void x86_lapic_init(void)
 {
     unsigned int lo, hi;
     __asm__ volatile("rdmsr" : "=a"(lo), "=d"(hi) : "c"(MSR_IA32_APIC_BASE));
     lo |= MSR_IA32_APIC_BASE_EN;
     __asm__ volatile("wrmsr" :: "c"(MSR_IA32_APIC_BASE), "a"(lo), "d"(hi));
     mmio_write32(LAPIC_SPURIOUS, LAPIC_SPURIOUS_ENABLE);
     printf("[x86] LAPIC enabled at 0x%08lx\n", (unsigned long)LAPIC_BASE);
 }
 
 /* ── COM1 UART ───────────────────────────────────────────── */
 static void x86_uart_handler(int irq, hw_context_t *ctx, void *id)
 {
     (void)ctx; (void)id;
     unsigned char rx = arch_inb(0x3F8u);
     printf("  [x86/uart_irq] IRQ%d rx=0x%02x '%c'\n",
            irq, rx & 0xFFu,
            (rx >= 0x20u && rx < 0x7Fu) ? (char)rx : '.');
 }
 
 static void x86_uart_init(void)
 {
     arch_outb(0x3F9u, 0x00u); /* IER: all off             */
     arch_outb(0x3FBu, 0x80u); /* LCR: DLAB on             */
     arch_outb(0x3F8u, 0x01u); /* DLL: 115200 baud         */
     arch_outb(0x3F9u, 0x00u); /* DLH                      */
     arch_outb(0x3FBu, 0x03u); /* LCR: 8N1, DLAB off       */
     arch_outb(0x3FAu, 0xC7u); /* FCR: FIFO enable         */
     arch_outb(0x3FCu, 0x0Bu); /* MCR: RTS+DTR+OUT2        */
     arch_outb(0x3F9u, 0x01u); /* IER: RX interrupt        */
     printf("[x86] COM1 UART @ 0x3F8 115200 8N1\n");
 }
 
 /* ── HPET ────────────────────────────────────────────────── */
 static void x86_hpet_init(void)
 {
     unsigned int caps = mmio_read32(LAPIC_BASE - 0x00100000u); /* placeholder */
     (void)caps;
     /* Set ENABLE_CNF in HPET GCFG */
     unsigned int cfg = mmio_read32(0xFED00010u);
     mmio_write32(0xFED00010u, cfg | 0x1u);
     printf("[x86] HPET enabled at 0xFED00000\n");
 }
 
 /* ── PIT timer IRQ handler ───────────────────────────────── */
 static void x86_pit_handler(int irq, hw_context_t *ctx, void *id)
 {
     (void)ctx; (void)id;
     /* Send EOI to LAPIC */
     mmio_write32(LAPIC_EOI, 0u);
     printf("  [x86/pit_irq] IRQ%d tick\n", irq);
 }
 
 /* ── arch_init ───────────────────────────────────────────── */
 void arch_init(void)
 {
     printf("[x86] arch_init start\n");
 
     x86_pic_remap();
     x86_lapic_init();
     x86_uart_init();
     x86_hpet_init();
 
     irq_register(0,  x86_pit_handler,  NULL);  /* IRQ0 = PIT timer */
     irq_register(4,  x86_uart_handler, NULL);  /* IRQ4 = COM1      */
     irq_global_enable();
 
     printf("[x86] arch_init done\n");
 }
 