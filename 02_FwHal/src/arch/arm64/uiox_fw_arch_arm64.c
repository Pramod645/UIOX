/**
 * @file  uiox_fw_arch_arm64.c
 * @brief UIOX Firmware — ARM64 (QEMU virt) platform HW ops implementation.
 * @date  2026-06-21
 */

/* src/arch/arm64/uiox_fw_arch_arm64.c — add at top */
#include "uiox_fw_arch_arm64.h"
#include "uiox_fw.h"


 /* ── Platform descriptor ───────────────────────────────────────────── */
 
 static uiox_fw_platform_t s_arm64_plat = {
     .magic        = UIOX_FW_MAGIC,
     .version      = UIOX_FW_VERSION,
     .caps         = UIOX_FW_CAP_GIC400  | UIOX_FW_CAP_PL011  |
                     UIOX_FW_CAP_ARM_GT  | UIOX_FW_CAP_GPIO    |
                     UIOX_FW_CAP_PSCI    | UIOX_FW_CAP_DTB,
     .arch         = UIOX_FW_ARCH_ARM64,
     .name         = "QEMU virt (ARM64)",
     .uart_base    = UIOX_MEM_ARM64_UART0,
     .gic_dist_base= UIOX_MEM_ARM64_GIC_DIST,
     .gic_cpu_base = UIOX_MEM_ARM64_GIC_CPU,
     .timer_base   = 0u,  /* ARM Generic Timer — no MMIO base */
     .gpio_base    = UIOX_MEM_ARM64_GPIO,
     .uart_irq     = UIOX_IRQ_ARM64_UART0,
     .timer_irq    = UIOX_IRQ_ARM64_TIMER0,
     .gpio_irq     = UIOX_IRQ_ARM64_GPIO,
     .num_cpus     = 4u,
     .ram_base     = UIOX_MEM_ARM64_RAM_BASE,
     .ram_size     = UIOX_MEM_ARM64_RAM_SIZE,
 };
 
 /* ── HW ops implementations ─────────────────────────────────────────── */
 
 static uiox_fw_err_t arm64_init(uiox_fw_platform_t *p)
 {
     UIOX_FW_UNUSED(p);
     /* GIC-400 minimal enable */
     fw_mmio_write32(UIOX_MEM_ARM64_GIC_DIST + 0x000u, 1u);  /* GICD_CTLR */
     fw_mmio_write32(UIOX_MEM_ARM64_GIC_CPU  + 0x004u, 0xF0u);/* GICC_PMR  */
     fw_mmio_write32(UIOX_MEM_ARM64_GIC_CPU  + 0x000u, 1u);  /* GICC_CTLR */
     return UIOX_FW_OK;
 }
 
 static void arm64_deinit(uiox_fw_platform_t *p) { UIOX_FW_UNUSED(p); }
 
 static void arm64_cache_enable(void)
 {
     uint64_t sctlr;
     __asm__ volatile("mrs %0, sctlr_el1" : "=r"(sctlr));
     sctlr |= (1ULL << 2) | (1ULL << 12);   /* C=1 (D-cache), I=1 (I-cache) */
     __asm__ volatile("msr sctlr_el1, %0; isb" :: "r"(sctlr) : "memory");
 }
 
 static void arm64_cache_disable(void)
 {
     uint64_t sctlr;
     __asm__ volatile("mrs %0, sctlr_el1" : "=r"(sctlr));
     sctlr &= ~((1ULL << 2) | (1ULL << 12));
     __asm__ volatile("msr sctlr_el1, %0; isb" :: "r"(sctlr) : "memory");
 }
 
 static void arm64_tlb_flush(void)
 { __asm__ volatile("tlbi vmalle1is; dsb sy; isb" ::: "memory"); }
 
 static void arm64_dsb(void)
 { __asm__ volatile("dsb sy"  ::: "memory"); }
 
 static void arm64_isb(void)
 { __asm__ volatile("isb"     ::: "memory"); }
 
 static void arm64_irq_init(uiox_fw_platform_t *p) { UIOX_FW_UNUSED(p); }
 
 static void arm64_irq_enable(uint32_t irq)
 {
     fw_mmio_write32(UIOX_MEM_ARM64_GIC_DIST +
                     0x100u + (irq / 32u) * 4u,
                     1u << (irq % 32u));
 }
 
 static void arm64_irq_disable(uint32_t irq)
 {
     fw_mmio_write32(UIOX_MEM_ARM64_GIC_DIST +
                     0x180u + (irq / 32u) * 4u,
                     1u << (irq % 32u));
 }
 
 static void arm64_irq_ack(uint32_t irq)
 {
     fw_mmio_write32(UIOX_MEM_ARM64_GIC_CPU + 0x010u, irq);  /* GICC_EOIR */
 }
 
 static void arm64_irq_global_en(void)
 { __asm__ volatile("msr daifclr, #0xf" ::: "memory"); }
 
 static void arm64_irq_global_dis(void)
 { __asm__ volatile("msr daifset, #0xf" ::: "memory"); }
 
 static void arm64_uart_init(uiox_fw_platform_t *p)
 {
     uintptr_t b = p->uart_base;
     fw_mmio_write32(b + PL011_CR, 0u);
     fw_mmio_write32(b + PL011_IBRD, 13u);   /* 115200 @ 24 MHz          */
     fw_mmio_write32(b + PL011_FBRD,  1u);
     fw_mmio_write32(b + PL011_LCR_H, PL011_LCR_WLEN8 | PL011_LCR_FEN);
     fw_mmio_write32(b + PL011_IMSC, 0u);
     fw_mmio_write32(b + PL011_CR, PL011_CR_UARTEN | PL011_CR_TXE | PL011_CR_RXE);
 }
 
 static void arm64_uart_putc(char c)
 {
     while (fw_mmio_read32(UIOX_MEM_ARM64_UART0 + PL011_FR) & PL011_FR_TXFF) ;
     fw_mmio_write32(UIOX_MEM_ARM64_UART0 + PL011_DR, (uint32_t)(uint8_t)c);
 }
 
 static void arm64_timer_init(uiox_fw_platform_t *p, uint32_t hz)
 {
     UIOX_FW_UNUSED(p);
     uint64_t freq;
     __asm__ volatile("mrs %0, cntfrq_el0" : "=r"(freq));
     uint64_t interval = freq / hz;
     __asm__ volatile("msr cntp_tval_el0, %0" :: "r"(interval));
     __asm__ volatile("msr cntp_ctl_el0, %0"  :: "r"((uint64_t)1u));
 }
 
 static uint64_t arm64_timer_tick(void)
 {
     uint64_t v;
     __asm__ volatile("isb; mrs %0, cntpct_el0" : "=r"(v) :: "memory");
     return v;
 }
 
 static void __attribute__((noreturn)) arm64_reset(void)
 {
     register uint64_t x0 __asm__("x0") = 0x84000009u; /* PSCI SYSTEM_RESET */
     __asm__ volatile("hvc #0" :: "r"(x0));
     for (;;) __asm__ volatile("wfi");
 }
 
 static void __attribute__((noreturn)) arm64_shutdown(void)
 {
     register uint64_t x0 __asm__("x0") = 0x84000008u; /* PSCI SYSTEM_OFF */
     __asm__ volatile("hvc #0" :: "r"(x0));
     for (;;) __asm__ volatile("wfi");
 }
 
 static const uiox_fw_hw_ops_t s_arm64_ops = {
     .init           = arm64_init,
     .deinit         = arm64_deinit,
     .cache_enable   = arm64_cache_enable,
     .cache_disable  = arm64_cache_disable,
     .tlb_flush      = arm64_tlb_flush,
     .barrier_dsb    = arm64_dsb,
     .barrier_isb    = arm64_isb,
     .irq_init       = arm64_irq_init,
     .irq_enable     = arm64_irq_enable,
     .irq_disable    = arm64_irq_disable,
     .irq_ack        = arm64_irq_ack,
     .irq_global_en  = arm64_irq_global_en,
     .irq_global_dis = arm64_irq_global_dis,
     .uart_init      = arm64_uart_init,
     .uart_putc      = arm64_uart_putc,
     .timer_init     = arm64_timer_init,
     .timer_tick     = arm64_timer_tick,
     .reset          = arm64_reset,
     .shutdown       = arm64_shutdown,
 };
 
 void uiox_fw_arch_register(void)
 {
     uiox_fw_hw_register(&s_arm64_ops, &s_arm64_plat);
 }
 /* src/arch/arm64/uiox_fw_arch_arm64.c — must contain exactly this */

void uiox_fw_hw_arm64_register(void)
 {
     uiox_fw_hw_register(&s_arm64_ops, &s_arm64_plat);
 }
