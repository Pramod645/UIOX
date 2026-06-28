/**
 * @file  uiox_boot_hw_x86.c
 * @brief UIOX Bootloader — x86_64 hardware ops (16550 UART, PIT timer).
 * @date  2026-06-12
 */

 #include "uiox_boot.h"

 /* =========================================================================
  * x86 port I/O
  * ====================================================================== */
 
 static inline void outb(uint16_t port, uint8_t val)
 { __asm__ volatile("outb %0,%1" :: "a"(val), "dN"(port)); }
 
 static inline uint8_t inb(uint16_t port)
 {
     uint8_t v;
     __asm__ volatile("inb %1,%0" : "=a"(v) : "dN"(port));
     return v;
 }
 
 /* =========================================================================
  * 16550 COM1 UART — 115200 8N1
  * ====================================================================== */
 
 static void com1_init(void)
 {
     outb(UIOX_COM1_PORT + COM1_IER, 0x00u); /* disable interrupts        */
     outb(UIOX_COM1_PORT + COM1_LCR, 0x80u); /* DLAB on                   */
     outb(UIOX_COM1_PORT + COM1_DLL, 0x01u); /* 115200 divisor lo         */
     outb(UIOX_COM1_PORT + COM1_DLM, 0x00u); /* 115200 divisor hi         */
     outb(UIOX_COM1_PORT + COM1_LCR, 0x03u); /* 8N1, DLAB off             */
     outb(UIOX_COM1_PORT + COM1_FCR, 0xC7u); /* FIFO enable, clear, 14B   */
     outb(UIOX_COM1_PORT + COM1_MCR, 0x0Bu); /* RTS, DTR, OUT2            */
 }
 
 static void com1_putc(char c)
 {
     while (!(inb(UIOX_COM1_PORT + COM1_LSR) & COM1_LSR_THRE))
         ;
     outb(UIOX_COM1_PORT + COM1_THR, (uint8_t)c);
 }
 
 /* =========================================================================
  * 8259A PIC remap (vectors 0x20–0x2F so they don't clash with exceptions)
  * ====================================================================== */
 
 static void pic_remap(void)
 {
     outb(0x20u, 0x11u);  /* Init master PIC              */
     outb(0xA0u, 0x11u);  /* Init slave  PIC              */
     outb(0x21u, 0x20u);  /* Master offset → 0x20         */
     outb(0xA1u, 0x28u);  /* Slave  offset → 0x28         */
     outb(0x21u, 0x04u);  /* Master: slave at IRQ2        */
     outb(0xA1u, 0x02u);  /* Slave:  cascade identity     */
     outb(0x21u, 0x01u);  /* 8086 mode                    */
     outb(0xA1u, 0x01u);
     outb(0x21u, 0xFFu);  /* Mask all IRQs (boot only)    */
     outb(0xA1u, 0xFFu);
 }
 
 /* =========================================================================
  * PIT 8254 — used as microsecond delay reference (ch0 mode3, 100 Hz)
  * ====================================================================== */
 
 #define PIT_CHANNEL0    0x40u
 #define PIT_CMD         0x43u
 #define PIT_FREQ_HZ     1193182u
 #define PIT_100HZ_DIV   (PIT_FREQ_HZ / 100u)
 
 static volatile uint64_t s_pit_ticks = 0u;
 
 static void pit_init(void)
 {
     uint16_t div = (uint16_t)PIT_100HZ_DIV;
     outb(PIT_CMD, 0x36u);            /* ch0, lo/hi, mode3, binary        */
     outb(PIT_CHANNEL0, (uint8_t)(div & 0xFFu));
     outb(PIT_CHANNEL0, (uint8_t)(div >> 8u));
 }
 
 static uint64_t x86_get_ticks(void)
 {
     uint64_t t;
     __asm__ volatile("rdtsc; shlq $32,%%rdx; orq %%rdx,%%rax"
                      : "=a"(t) :: "rdx");
     return t;
 }
 
 static void x86_udelay(uint32_t us)
 {
     /* Busy-wait using TSC.  Assume ~3 GHz → 3000 ticks/µs */
     uint64_t wait = (uint64_t)us * 3000u;
     uint64_t start = x86_get_ticks();
     while ((x86_get_ticks() - start) < wait)
         ;
 }
 
 static void x86_dcache_flush(uintptr_t start, size_t len)
 {
     uintptr_t end  = start + len;
     uintptr_t line = 64u;
     uintptr_t addr = start & ~(line - 1u);
     while (addr < end) {
         __asm__ volatile("clflush (%0)" :: "r"(addr) : "memory");
         addr += line;
     }
     __asm__ volatile("mfence" ::: "memory");
 }
 
 static void x86_icache_inv(void)
 {
     __asm__ volatile("mfence" ::: "memory");
 }
 
 static void x86_barrier(void)
 {
     __asm__ volatile("mfence" ::: "memory");
 }
 
 static void __attribute__((noreturn)) x86_reset(void)
 {
     /* Triple-fault via null IDT load */
     struct { uint16_t limit; uint64_t base; } __attribute__((packed))
         idtr = { 0, 0 };
     __asm__ volatile("lidt %0; int3" :: "m"(idtr));
     for (;;) __asm__ volatile("hlt");
 }
 
 static void x86_hw_init(void)
 {
     pic_remap();
     pit_init();
     com1_init();
 }
 
 static const uiox_boot_hw_ops_t x86_ops = {
     .init         = x86_hw_init,
     .uart_putc    = com1_putc,
     .dcache_flush = x86_dcache_flush,
     .icache_inv   = x86_icache_inv,
     .get_ticks    = x86_get_ticks,
     .udelay       = x86_udelay,
     .reset        = x86_reset,
     .barrier      = x86_barrier,
 };
 
 void uiox_boot_hw_x86_register(void)
 {
     uiox_boot_hw_register(&x86_ops);
 }
 