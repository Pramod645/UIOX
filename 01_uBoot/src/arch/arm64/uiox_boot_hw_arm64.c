/**
 * @file  uiox_boot_hw_arm64.c
 * @brief UIOX Bootloader — AArch64 hardware ops implementation.
 *        PL011 UART, GIC-400 minimal init, cache ops, timer.
 * @date  2026-06-12
 */

 #include "uiox_boot.h"

 /* =========================================================================
  * PL011 UART — QEMU virt: base 0x09000000, 24 MHz clock, 115200 baud
  * IBRD = 24000000 / (16 × 115200) = 13  FBRD = 1
  * ====================================================================== */
 
 static void pl011_init(void)
 {
     uintptr_t base = UIOX_PL011_BASE_ARM64;
     /* Disable UART */
     mmio_write32(base + PL011_CR, 0u);
     /* Set baud: IBRD=13, FBRD=1 */
     mmio_write32(base + PL011_IBRD, 13u);
     mmio_write32(base + PL011_FBRD,  1u);
     /* 8N1, FIFO enable */
     mmio_write32(base + PL011_LCR_H, PL011_LCR_WLEN8 | PL011_LCR_FEN);
     /* Enable UART, TX, RX */
     mmio_write32(base + PL011_CR,
                  PL011_CR_UARTEN | PL011_CR_TXE | PL011_CR_RXE);
 }
 
 static void pl011_putc(char c)
 {
     uintptr_t base = UIOX_PL011_BASE_ARM64;
     /* Wait for TX FIFO not full */
     while (mmio_read32(base + PL011_FR) & PL011_FR_TXFF)
         ;
     mmio_write32(base + PL011_DR, (uint32_t)(uint8_t)c);
 }
 
 /* =========================================================================
  * GIC-400 minimal init (enable distributor + CPU interface)
  * ====================================================================== */
 
 static void gic_init(void)
 {
     /* Enable GIC distributor */
     mmio_write32(UIOX_GICD_BASE_ARM64 + GICD_CTLR, 1u);
     /* Set CPU interface priority mask: accept all */
     mmio_write32(UIOX_GICC_BASE_ARM64 + GICC_PMR, 0xFFu);
     /* Enable CPU interface */
     mmio_write32(UIOX_GICC_BASE_ARM64 + GICC_CTLR, 1u);
 }
 
 /* =========================================================================
  * AArch64 cache ops
  * ====================================================================== */
 
 static void arm64_dcache_flush(uintptr_t start, size_t len)
 {
     uintptr_t end  = start + len;
     uintptr_t line = 64u;   /* typical cache line size */
     uintptr_t addr = start & ~(line - 1u);
     while (addr < end) {
         __asm__ volatile("dc civac, %0" :: "r"(addr) : "memory");
         addr += line;
     }
     __asm__ volatile("dsb sy" ::: "memory");
 }
 
 static void arm64_icache_inv(void)
 {
     __asm__ volatile("ic iallu; dsb sy; isb" ::: "memory");
 }
 
 /* =========================================================================
  * System counter (CNTPCT_EL0)
  * ====================================================================== */
 
 static uint64_t arm64_get_ticks(void)
 {
     uint64_t t;
     __asm__ volatile("isb; mrs %0, cntpct_el0" : "=r"(t) :: "memory");
     return t;
 }
 
 static void arm64_udelay(uint32_t us)
 {
     /* CNTFRQ_EL0 gives the timer frequency */
     uint64_t freq;
     __asm__ volatile("mrs %0, cntfrq_el0" : "=r"(freq));
     uint64_t ticks = ((uint64_t)us * freq) / 1000000u;
     uint64_t start = arm64_get_ticks();
     while ((arm64_get_ticks() - start) < ticks)
         ;
 }
 
 static void arm64_barrier(void)
 {
     __asm__ volatile("dsb sy; isb" ::: "memory");
 }
 
 static void __attribute__((noreturn)) arm64_reset(void)
 {
     /* PSCI SYSTEM_RESET via HVC (QEMU virt supports PSCI 1.0) */
     register uint64_t x0 __asm__("x0") = 0x84000009u;  /* SYSTEM_RESET */
     __asm__ volatile("hvc #0" :: "r"(x0));
     for (;;) __asm__ volatile("wfi");
 }
 
 /* =========================================================================
  * Ops table registration
  * ====================================================================== */
 
 static const uiox_boot_hw_ops_t arm64_ops = {
     .init         = pl011_init,
     .uart_putc    = pl011_putc,
     .dcache_flush = arm64_dcache_flush,
     .icache_inv   = arm64_icache_inv,
     .get_ticks    = arm64_get_ticks,
     .udelay       = arm64_udelay,
     .reset        = arm64_reset,
     .barrier      = arm64_barrier,
 };
 
 /**
  * Called by uiox_boot_entry_arm64.S before uiox_boot_main().
  * Actually called implicitly — uiox_boot_hw_init() selects the right
  * arch ops via the compile-time ARCH define; for arm64 we auto-register.
  */
 void uiox_boot_hw_arm64_register(void)
 {
     gic_init();
     uiox_boot_hw_register(&arm64_ops);
 }
 