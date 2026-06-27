/**
 * @file  uiox_fw_arch_arm32.c
 * @brief UIOX Firmware — ARM32 (QEMU versatilepb) HW ops.
 * @date  2026-06-21
 */

/* src/arch/arm32/uiox_fw_arch_arm32.c — add at top */
#include "uiox_fw_arch_arm32.h"
#include "uiox_fw.h"

/*
 * udiv32_soft() — software 32-bit unsigned divide.
 * Uses only bit-shifts and subtracts — no __aeabi_uidiv.
 * Add this near the top of each affected .c file,
 * or put it in a shared uiox_fw_math.h header.
 */
static inline uint32_t udiv32_soft_a(uint32_t n, uint32_t d,
    uint32_t *rem_out)
{
uint32_t q = 0u, r = 0u;
if (d == 0u) { if (rem_out) *rem_out = 0u; return 0u; }
for (int i = 31; i >= 0; i--) {
r = (r << 1u) | ((n >> (uint32_t)i) & 1u);
if (r >= d) { r -= d; q |= (1u << (uint32_t)i); }
}
if (rem_out) *rem_out = r;
return q;
}



 static uiox_fw_platform_t s_arm32_plat = {
     .magic        = UIOX_FW_MAGIC,
     .version      = UIOX_FW_VERSION,
     .caps         = UIOX_FW_CAP_PL011 | UIOX_FW_CAP_SP804 | UIOX_FW_CAP_GPIO,
     .arch         = UIOX_FW_ARCH_ARM32,
     .name         = "QEMU versatilepb (ARM32)",
     .uart_base    = UIOX_MEM_ARM32_UART0,
     .gic_dist_base= UIOX_MEM_ARM32_VIC,
     .gic_cpu_base = UIOX_MEM_ARM32_VIC,
     .timer_base   = UIOX_MEM_ARM32_TIMER0,
     .gpio_base    = UIOX_MEM_ARM32_GPIO,
     .uart_irq     = UIOX_IRQ_ARM32_UART0,
     .timer_irq    = UIOX_IRQ_ARM32_TIMER0,
     .gpio_irq     = UIOX_IRQ_ARM32_GPIO,
     .num_cpus     = 1u,
     .ram_base     = UIOX_MEM_ARM32_RAM_BASE,
     .ram_size     = UIOX_MEM_ARM32_RAM_SIZE,
 };
 
 static uiox_fw_err_t arm32_init(uiox_fw_platform_t *p) { UIOX_FW_UNUSED(p); return UIOX_FW_OK; }
 static void          arm32_deinit(uiox_fw_platform_t *p) { UIOX_FW_UNUSED(p); }
 
 static void arm32_cache_enable(void)
 {
     uint32_t r;
     __asm__ volatile("mrc p15,0,%0,c1,c0,0" : "=r"(r));
     r |= (1u << 2) | (1u << 12);  /* C=1, I=1 */
     __asm__ volatile("mcr p15,0,%0,c1,c0,0; isb" :: "r"(r) : "memory");
 }
 
 static void arm32_cache_disable(void)
 {
     uint32_t r;
     __asm__ volatile("mrc p15,0,%0,c1,c0,0" : "=r"(r));
     r &= ~((1u << 2) | (1u << 12));
     __asm__ volatile("mcr p15,0,%0,c1,c0,0; isb" :: "r"(r) : "memory");
 }
 
 static void arm32_tlb_flush(void)
 {
     uint32_t z = 0u;
     __asm__ volatile("mcr p15,0,%0,c8,c7,0; dsb; isb" :: "r"(z) : "memory");
 }
 
 static void arm32_dsb(void) { __asm__ volatile("dsb" ::: "memory"); }
 static void arm32_isb(void) { __asm__ volatile("isb" ::: "memory"); }
 
 /* VIC interrupt enable at offset 0x010 */
 #define VPB_VIC_INTENABLE   0x010u
 #define VPB_VIC_INTENCLEAR  0x014u
 
 static void arm32_irq_init (uiox_fw_platform_t *p) { UIOX_FW_UNUSED(p); }
 
 static void arm32_irq_enable(uint32_t irq)
 { fw_mmio_write32(UIOX_MEM_ARM32_VIC + VPB_VIC_INTENABLE, 1u << irq); }
 
 static void arm32_irq_disable(uint32_t irq)
 { fw_mmio_write32(UIOX_MEM_ARM32_VIC + VPB_VIC_INTENCLEAR, 1u << irq); }
 
 static void arm32_irq_ack   (uint32_t irq) { UIOX_FW_UNUSED(irq); }
 
 static void arm32_irq_global_en(void)
 { __asm__ volatile("cpsie if" ::: "memory"); }
 
 static void arm32_irq_global_dis(void)
 { __asm__ volatile("cpsid if" ::: "memory"); }
 
 static void arm32_uart_init(uiox_fw_platform_t *p)
 {
     uintptr_t b = p->uart_base;
     fw_mmio_write32(b + PL011_CR, 0u);
     fw_mmio_write32(b + PL011_IBRD, 13u);
     fw_mmio_write32(b + PL011_FBRD,  1u);
     fw_mmio_write32(b + PL011_LCR_H, PL011_LCR_WLEN8 | PL011_LCR_FEN);
     fw_mmio_write32(b + PL011_IMSC, 0u);
     fw_mmio_write32(b + PL011_CR, PL011_CR_UARTEN | PL011_CR_TXE | PL011_CR_RXE);
 }
 
 static void arm32_uart_putc(char c)
 {
     while (fw_mmio_read32(UIOX_MEM_ARM32_UART0 + PL011_FR) & PL011_FR_TXFF) ;
     fw_mmio_write32(UIOX_MEM_ARM32_UART0 + PL011_DR, (uint32_t)(uint8_t)c);
 }
 
 static void arm32_timer_init(uiox_fw_platform_t *p, uint32_t hz)
 {
     uintptr_t b = p->timer_base;
     fw_mmio_write32(b + SP804_TIMER1_CTRL, 0u);
#if defined(__arm__)
    uint32_t loadk = udiv32_soft_a(SP804_CLOCK_HZ, hz, NULL);
    fw_mmio_write32(b + SP804_TIMER1_LOAD, loadk);
#else
     fw_mmio_write32(b + SP804_TIMER1_LOAD, SP804_CLOCK_HZ / hz);
#endif
     fw_mmio_write32(b + SP804_TIMER1_INTCLR, 1u);
     fw_mmio_write32(b + SP804_TIMER1_CTRL,
                     SP804_CTRL_EN | SP804_CTRL_PERIODIC |
                     SP804_CTRL_IE | SP804_CTRL_32BIT);
 }
 
 static uint64_t arm32_timer_tick(void)
 {
     return (uint64_t)(0xFFFFFFFFu -
            fw_mmio_read32(UIOX_MEM_ARM32_TIMER0 + SP804_TIMER1_VALUE));
 }
 
 static void __attribute__((noreturn)) arm32_reset(void)
 {
     fw_mmio_write32(0x10000040u, 0x100u);
     for (;;) __asm__ volatile("wfi");
 }
 
 static void __attribute__((noreturn)) arm32_shutdown(void)
 {
     for (;;) __asm__ volatile("wfi");
 }
 
 static const uiox_fw_hw_ops_t s_arm32_ops = {
     .init           = arm32_init,
     .deinit         = arm32_deinit,
     .cache_enable   = arm32_cache_enable,
     .cache_disable  = arm32_cache_disable,
     .tlb_flush      = arm32_tlb_flush,
     .barrier_dsb    = arm32_dsb,
     .barrier_isb    = arm32_isb,
     .irq_init       = arm32_irq_init,
     .irq_enable     = arm32_irq_enable,
     .irq_disable    = arm32_irq_disable,
     .irq_ack        = arm32_irq_ack,
     .irq_global_en  = arm32_irq_global_en,
     .irq_global_dis = arm32_irq_global_dis,
     .uart_init      = arm32_uart_init,
     .uart_putc      = arm32_uart_putc,
     .timer_init     = arm32_timer_init,
     .timer_tick     = arm32_timer_tick,
     .reset          = arm32_reset,
     .shutdown       = arm32_shutdown,
 };
 
 void uiox_fw_arch_register(void)
 { uiox_fw_hw_register(&s_arm32_ops, &s_arm32_plat); }
 