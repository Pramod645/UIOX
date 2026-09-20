/*
 * 01_uBoot/src/arch/x86_64/uiox_boot_hw_x86.c
 * x86-64 hardware ops. 16550 COM1 is port I/O, fixed by the PC arch.
 */
#include "uiox_boot.h"
#include "uiox_boot_board.h"
#include "uiox_boot_media.h"

#define COM1 0x3F8u

static inline void outb(uint16_t p, uint8_t v)
{ __asm__ volatile("outb %0, %1" :: "a"(v), "Nd"(p)); }
static inline uint8_t inb(uint16_t p)
{ uint8_t v; __asm__ volatile("inb %1, %0" : "=a"(v) : "Nd"(p)); return v; }

static void com1_init(uint32_t baud_div)
{
    outb(COM1 + 1u, 0x00u);
    outb(COM1 + 3u, 0x80u);
    outb(COM1 + 0u, (uint8_t)(baud_div));
    outb(COM1 + 1u, (uint8_t)(baud_div >> 8));
    outb(COM1 + 3u, 0x03u);
    outb(COM1 + 2u, 0xC7u);
    outb(COM1 + 4u, 0x0Bu);
}

static void com1_putc(char c)
{
    while (!(inb(COM1 + 5u) & 0x20u)) { }
    outb(COM1 + 0u, (uint8_t)c);
}

static void x86_hw_init(void)
{
    (void)uiox_board_bringup();
    com1_init(1u);
}

int uiox_boot_hw_read_block(uint32_t blkno, void *buf)
{
    return uiox_boot_media_read_block(blkno, 1u, buf) == UIOX_BOOT_OK ? 0 : -1;
}

static void x86_dcache_flush(uintptr_t s, size_t n)
{ (void)s; (void)n; __asm__ volatile("mfence" ::: "memory"); }
static void x86_icache_inv(void)
{ __asm__ volatile("lfence" ::: "memory"); }
static uint64_t x86_get_ticks(void)
{
    uint32_t lo, hi;
    __asm__ volatile("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
}
static void x86_udelay(uint32_t us) { (void)us; /* TODO: TSC calibration */ }
static void __attribute__((noreturn)) x86_reset(void)
{ for (;;) __asm__ volatile("hlt"); }
static void x86_barrier(void) { __asm__ volatile("mfence" ::: "memory"); }

static const uiox_boot_hw_ops_t x86_ops = {
    .init = x86_hw_init, .uart_putc = com1_putc,
    .dcache_flush = x86_dcache_flush, .icache_inv = x86_icache_inv,
    .get_ticks = x86_get_ticks, .udelay = x86_udelay,
    .reset = x86_reset, .barrier = x86_barrier,
};

void uiox_boot_hw_x86_register(void) { uiox_boot_hw_register(&x86_ops); }
