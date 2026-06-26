/**
 * @file  uiox_fw_arch_x86.c
 * @brief UIOX Firmware — x86_64 (QEMU q35) HW ops.
 * @date  2026-06-21
 */

 #include "uiox_fw.h"

 static inline void _outb(uint16_t p, uint8_t v)
 { __asm__ volatile("outb %0,%1"::"a"(v),"dN"(p)); }
 static inline uint8_t _inb(uint16_t p)
 { uint8_t v; __asm__ volatile("inb %1,%0":"=a"(v):"dN"(p)); return v; }
 static inline void _outw(uint16_t p, uint16_t v)
 { __asm__ volatile("outw %0,%1"::"a"(v),"dN"(p)); }
 
 static uiox_fw_platform_t s_x86_plat = {
     .magic        = UIOX_FW_MAGIC,
     .version      = UIOX_FW_VERSION,
     .caps         = UIOX_FW_CAP_PIC8259 | UIOX_FW_CAP_16550 |
                     UIOX_FW_CAP_PIT8254  | UIOX_FW_CAP_ACPI,
     .arch         = UIOX_FW_ARCH_X86_64,
     .name         = "QEMU q35 (x86_64)",
     .uart_base    = Q35_COM1_PORT,
     .gic_dist_base= Q35_IOAPIC_BASE,
     .gic_cpu_base = Q35_LAPIC_BASE,
     .timer_base   = Q35_PIT_PORT,
     .gpio_base    = 0u,
     .uart_irq     = UIOX_IRQ_X86_COM1 + UIOX_IRQ_X86_REMAP_BASE,
     .timer_irq    = UIOX_IRQ_X86_TIMER + UIOX_IRQ_X86_REMAP_BASE,
     .gpio_irq     = 0u,
     .num_cpus     = 4u,
     .ram_base     = UIOX_MEM_X86_RAM_BASE,
     .ram_size     = UIOX_MEM_X86_RAM_SIZE,
 };
 
 static uiox_fw_err_t x86_init(uiox_fw_platform_t *p)
 {
     UIOX_FW_UNUSED(p);
     /* Remap 8259A PIC: IRQ0–7 → 0x20, IRQ8–15 → 0x28 */
     _outb(0x20u, 0x11u); _outb(0xA0u, 0x11u);
     _outb(0x21u, 0x20u); _outb(0xA1u, 0x28u);
     _outb(0x21u, 0x04u); _outb(0xA1u, 0x02u);
     _outb(0x21u, 0x01u); _outb(0xA1u, 0x01u);
     _outb(0x21u, 0xFFu); _outb(0xA1u, 0xFFu); /* mask all initially */
     return UIOX_FW_OK;
 }
 
 static void x86_deinit(uiox_fw_platform_t *p) { UIOX_FW_UNUSED(p); }
 
 static void x86_cache_enable(void)  { /* x86 caches always on after init */ }
 static void x86_cache_disable(void) { __asm__ volatile("wbinvd" ::: "memory"); }
 static void x86_tlb_flush(void)
 {
     uint64_t cr3;
     __asm__ volatile("movq %%cr3,%0; movq %0,%%cr3" : "=r"(cr3) :: "memory");
 }
 static void x86_dsb(void) { __asm__ volatile("mfence" ::: "memory"); }
 static void x86_isb(void) { __asm__ volatile(""       ::: "memory"); }
 
 static void x86_irq_init(uiox_fw_platform_t *p) { UIOX_FW_UNUSED(p); }
 
 static void x86_irq_enable(uint32_t irq)
 {
     /* Convert remapped vector back to physical IRQ# */
     uint8_t irqn = (uint8_t)(irq - 0x20u);
     if (irqn < 8u) {
         uint8_t mask = _inb(0x21u) & (uint8_t)(~(1u << irqn));
         _outb(0x21u, mask);
     } else if (irqn < 16u) {
         uint8_t mask = _inb(0xA1u) & (uint8_t)(~(1u << (irqn - 8u)));
         _outb(0xA1u, mask);
     }
 }
 
 static void x86_irq_disable(uint32_t irq)
 {
     uint8_t irqn = (uint8_t)(irq - 0x20u);
     if (irqn < 8u) {
         uint8_t mask = _inb(0x21u) | (uint8_t)(1u << irqn);
         _outb(0x21u, mask);
     } else if (irqn < 16u) {
         uint8_t mask = _inb(0xA1u) | (uint8_t)(1u << (irqn - 8u));
         _outb(0xA1u, mask);
     }
 }
 
 static void x86_irq_ack(uint32_t irq)
 {
     if (irq - 0x20u >= 8u) _outb(0xA0u, 0x20u);  /* slave EOI  */
     _outb(0x20u, 0x20u);                           /* master EOI */
 }
 
 static void x86_irq_global_en (void) { __asm__ volatile("sti" ::: "memory"); }
 static void x86_irq_global_dis(void) { __asm__ volatile("cli" ::: "memory"); }
 
 static void x86_uart_init(uiox_fw_platform_t *p)
 {
     uint16_t port = (uint16_t)p->uart_base;
     _outb(port + 1u, 0x00u);
     _outb(port + 3u, 0x80u);    /* DLAB=1 */
     _outb(port + 0u, 0x01u);    /* 115200 divisor lo */
     _outb(port + 1u, 0x00u);    /* 115200 divisor hi */
     _outb(port + 3u, 0x03u);    /* 8N1 */
     _outb(port + 2u, 0xC7u);    /* FIFO */
     _outb(port + 4u, 0x0Bu);    /* RTS+DTR+OUT2 */
 }
 
 static void x86_uart_putc(char c)
 {
     while (!(_inb((uint16_t)(Q35_COM1_PORT + 5u)) & 0x20u)) ;
     _outb((uint16_t)Q35_COM1_PORT, (uint8_t)c);
 }
 
 static void x86_timer_init(uiox_fw_platform_t *p, uint32_t hz)
 {
     UIOX_FW_UNUSED(p);
     uint16_t div = (uint16_t)(PIT_BASE_FREQ / hz);
     _outb(PIT_CMD, PIT_CMD_CH0_MODE3);
     _outb(PIT_CHANNEL0, (uint8_t)(div & 0xFFu));
     _outb(PIT_CHANNEL0, (uint8_t)(div >> 8u));
 }
 
 static uint64_t x86_timer_tick(void)
 {
     uint64_t v;
     __asm__ volatile("rdtsc; shlq $32,%%rdx; orq %%rdx,%%rax"
                      : "=a"(v) :: "rdx");
     return v;
 }
 
 static void __attribute__((noreturn)) x86_reset(void)
 {
     _outb(0x64u, 0xFEu);
     for (;;) __asm__ volatile("hlt");
 }
 
 static void __attribute__((noreturn)) x86_shutdown(void)
 {
     _outw((uint16_t)ACPI_PM1A_CNT_BLOCK,
           (uint16_t)(ACPI_S5_SLEEP_TYPE | ACPI_SLP_EN));
     for (;;) __asm__ volatile("hlt");
 }
 
 static const uiox_fw_hw_ops_t s_x86_ops = {
     .init           = x86_init,
     .deinit         = x86_deinit,
     .cache_enable   = x86_cache_enable,
     .cache_disable  = x86_cache_disable,
     .tlb_flush      = x86_tlb_flush,
     .barrier_dsb    = x86_dsb,
     .barrier_isb    = x86_isb,
     .irq_init       = x86_irq_init,
     .irq_enable     = x86_irq_enable,
     .irq_disable    = x86_irq_disable,
     .irq_ack        = x86_irq_ack,
     .irq_global_en  = x86_irq_global_en,
     .irq_global_dis = x86_irq_global_dis,
     .uart_init      = x86_uart_init,
     .uart_putc      = x86_uart_putc,
     .timer_init     = x86_timer_init,
     .timer_tick     = x86_timer_tick,
     .reset          = x86_reset,
     .shutdown       = x86_shutdown,
 };
 
 void uiox_fw_arch_register(void)
 { uiox_fw_hw_register(&s_x86_ops, &s_x86_plat); }
 