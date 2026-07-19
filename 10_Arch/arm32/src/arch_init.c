/*
 * 10_Arch/arm32/src/arch_init.c
 * ARMv7-A 32-bit platform initialisation.
 */

 #include "arch_defs.h"
 #include "../../../20_DriverInterfaces/include/hw_types.h"
 #include "../../../20_DriverInterfaces/include/mmio.h"
 #include "../../../20_DriverInterfaces/include/irq.h"
 #include "../../../20_DriverInterfaces/include/cpu.h"
 #include "../../../03_SoC/include/uiox_soc_stdio.h"
 #include "../../../03_SoC/include/uiox_soc_string.h"
 
 /* ── Cache enable via CP15 ───────────────────────────────── */
 static void arm32_cache_enable(void)
 {
     unsigned int sctlr;
     __asm__ volatile("mrc p15,0,%0,c1,c0,0" : "=r"(sctlr));
     sctlr |= SCTLR_I | SCTLR_C;
     __asm__ volatile("mcr p15,0,%0,c1,c0,0" :: "r"(sctlr) : "memory");
     arch_isb();
     printf("[arm32] I/D caches enabled\n");
 }
 
 /* ── GIC-400 ─────────────────────────────────────────────── */
 static void arm32_gic_init(void)
 {
     mmio_write32(GIC_DIST_CTLR, 0x0u);
     mmio_write32(GIC_DIST_ISENABLER0, 0xFFFFFFFFu);
     for (unsigned int i = 0u; i < 64u; i++)
         mmio_write32(GIC_DIST_IPRIORITYR0 + i * 4u, 0xA0A0A0A0u);
     mmio_write32(GIC_CPU_PMR,  0xFFu);
     mmio_write32(GIC_CPU_CTLR, 0x1u);
     mmio_write32(GIC_DIST_CTLR, 0x1u);
     printf("[arm32] GIC-400 init (DIST=0x%08lx CPU=0x%08lx)\n",
            (unsigned long)GIC_DIST_BASE,
            (unsigned long)GIC_CPU_BASE);
 }
 
 /* ── PL011 UART ──────────────────────────────────────────── */
 static void arm32_uart_handler(int irq, hw_context_t *ctx, void *id)
 {
     (void)ctx; (void)id;
     unsigned int rx = mmio_read32(UART0_BASE + 0x000u);
     printf("  [arm32/uart_irq] IRQ%d rx=0x%02x\n", irq, rx & 0xFFu);
     mmio_write32(UART0_BASE + 0x044u, 0x7FFu);
 }
 
 static void arm32_uart_init(void)
 {
     mmio_write32(UART0_BASE + 0x030u, 0x0u);
     mmio_write32(UART0_BASE + 0x024u, 13u);
     mmio_write32(UART0_BASE + 0x028u, 1u);
     mmio_write32(UART0_BASE + 0x02Cu, 0x70u);
     mmio_write32(UART0_BASE + 0x038u, (1u << 4));
     mmio_write32(UART0_BASE + 0x030u, 0x301u);
     printf("[arm32] PL011 UART @ 0x%08lx 115200 8N1\n",
            (unsigned long)UART0_BASE);
 }
 
 /* ── SP804 Timer (1 ms tick) ─────────────────────────────── */
 static void arm32_timer_handler(int irq, hw_context_t *ctx, void *id)
 {
     (void)ctx; (void)id;
     mmio_write32(TIMER0_BASE + 0x00Cu, 0x1u); /* IntClr */
     printf("  [arm32/timer_irq] IRQ%d tick\n", irq);
 }
 
 static void arm32_timer_init(void)
 {
     mmio_write32(TIMER0_BASE + 0x008u, 0x00u);   /* disable     */
     mmio_write32(TIMER0_BASE + 0x000u, 1000u);   /* load 1 ms   */
     mmio_write32(TIMER0_BASE + 0x018u, 1000u);   /* bgload      */
     mmio_write32(TIMER0_BASE + 0x008u, 0xE2u);   /* enable, IRQ */
     printf("[arm32] SP804 timer @ 0x%08lx 1 ms tick\n",
            (unsigned long)TIMER0_BASE);
 }
 
 /* ── arch_init ───────────────────────────────────────────── */
 void arch_init(void)
 {
     printf("[arm32] arch_init start\n");
 
     arm32_cache_enable();
     arm32_gic_init();
     arm32_uart_init();
     arm32_timer_init();
 
     irq_register(ARCH_TIMER_IRQ_PHYS, arm32_timer_handler, NULL);
     irq_register(UART0_IRQ,           arm32_uart_handler,  NULL);
     irq_global_enable();
 
     printf("[arm32] arch_init done\n");
 }
 