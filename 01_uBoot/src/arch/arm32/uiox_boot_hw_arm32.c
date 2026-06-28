/**
 * @file  uiox_boot_hw_arm32.c
 * @brief UIOX Bootloader — ARMv7-A hardware ops (PL011 @ versatilepb).
 * @date  2026-06-12
 */

 #include "uiox_boot.h"

 /* PL011 on QEMU versatilepb: base 0x101F1000, 24 MHz, 115200 baud */
 static void pl011_arm32_init(void)
 {
     uintptr_t base = UIOX_PL011_BASE_ARM32;
     mmio_write32(base + PL011_CR, 0u);
     mmio_write32(base + PL011_IBRD, 13u);
     mmio_write32(base + PL011_FBRD,  1u);
     mmio_write32(base + PL011_LCR_H, PL011_LCR_WLEN8 | PL011_LCR_FEN);
     mmio_write32(base + PL011_CR,
                  PL011_CR_UARTEN | PL011_CR_TXE | PL011_CR_RXE);
 }
 
 static void pl011_arm32_putc(char c)
 {
     uintptr_t base = UIOX_PL011_BASE_ARM32;
     while (mmio_read32(base + PL011_FR) & PL011_FR_TXFF)
         ;
     mmio_write32(base + PL011_DR, (uint32_t)(uint8_t)c);
 }
 
 static void arm32_dcache_flush(uintptr_t start, size_t len)
 {
     uintptr_t end  = start + len;
     uintptr_t line = 32u;
     uintptr_t addr = start & ~(line - 1u);
     while (addr < end) {
         __asm__ volatile("mcr p15, 0, %0, c7, c14, 1" :: "r"(addr));
         addr += line;
     }
     __asm__ volatile("dsb" ::: "memory");
 }
 
 static void arm32_icache_inv(void)
 {
     uint32_t z = 0u;
     __asm__ volatile("mcr p15, 0, %0, c7, c5, 0" :: "r"(z));
     __asm__ volatile("dsb; isb" ::: "memory");
 }
 
 static uint64_t arm32_get_ticks(void)
 {
     /* SP804 Timer 1 on versatilepb: base 0x101E2000 */
     return (uint64_t)mmio_read32(0x101E2004u);  /* Timer1Value */
 }
 
 static void arm32_udelay(uint32_t us)
 {
     /* 1 MHz SP804 tick = 1 µs */
     uint64_t start = arm32_get_ticks();
     while ((arm32_get_ticks() - start) < (uint64_t)us)
         ;
 }
 
 static void arm32_barrier(void)
 {
     __asm__ volatile("dsb; isb" ::: "memory");
 }
 
 static void __attribute__((noreturn)) arm32_reset(void)
 {
     /* Watchdog reset via versatilepb system controller */
     mmio_write32(0x10000000u + 0x040u, 0x07Du); /* LOCK */
     mmio_write32(0x10000000u + 0x004u, 0x01u);  /* Reset */
     for (;;) __asm__ volatile("wfi");
 }
 
 static const uiox_boot_hw_ops_t arm32_ops = {
     .init         = pl011_arm32_init,
     .uart_putc    = pl011_arm32_putc,
     .dcache_flush = arm32_dcache_flush,
     .icache_inv   = arm32_icache_inv,
     .get_ticks    = arm32_get_ticks,
     .udelay       = arm32_udelay,
     .reset        = arm32_reset,
     .barrier      = arm32_barrier,
 };
 
 void uiox_boot_hw_arm32_register(void)
 {
     uiox_boot_hw_register(&arm32_ops);
 }
 