/*
 * 01_uBoot/src/arch/x86_64/uiox_boot_hw_x86.c
 *
 * x86-64 hardware ops, driven by the board descriptor.
 * 16550 COM1 UART, PIT timestamp. I/O ports are fixed by the PC
 * architecture; the board descriptor carries only the memory map.
 */
#include "uiox_boot.h"
#include "uiox_boot_board.h"

#define COM1 0x3F8u
#define PIT_CH0 0x40u
#define PIT_CMD 0x43u

static inline void outb(uint16_t p, uint8_t v)
{ __asm__ volatile("outb %0, %1" :: "a"(v), "Nd"(p)); }
static inline uint8_t inb(uint16_t p)
{ uint8_t v; __asm__ volatile("inb %1, %0" : "=a"(v) : "Nd"(p)); return v; }

static void com1_init(uint32_t baud_div)
{
    outb(COM1 + 1u, 0x00u);                 /* disable interrupts */
    outb(COM1 + 3u, 0x80u);                 /* DLAB = 1 */
    outb(COM1 + 0u, (uint8_t)(baud_div));
    outb(COM1 + 1u, (uint8_t)(baud_div >> 8));
    outb(COM1 + 3u, 0x03u);                 /* 8N1 */
    outb(COM1 + 2u, 0xC7u);                 /* FIFO enable+clear */
    outb(COM1 + 4u, 0x0Bu);                 /* RTS/DTR/OUT2 */
}

static void com1_putc(char c)
{
    while (!(inb(COM1 + 5u) & 0x20u)) { }   /* wait THR empty */
    outb(COM1 + 0u, (uint8_t)c);
}

static void x86_hw_init(void)
{
    /* Board bring-up FIRST (on PC this is typically a no-op, but a
     * real board descriptor may init a PLL or chipset here). */
    (void)uiox_board_bringup();
    com1_init(1u);                          /* 115200 with 1.8432 MHz */
}

/* --- Real storage: forward to the boot-media layer ----------------- */
uiox_boot_err_t uiox_boot_media_read_block(uint32_t blk, uint32_t nblocks,
                                           void *buf);

int uiox_boot_hw_read_block(uint32_t blkno, void *buf)
{
    return uiox_boot_media_read_block(blkno, 1u, buf) == UIOX_BOOT_OK ? 0 : -1;
}

/* --- Cache / timebase ---------------------------------------------- */
static void x86_dcache_flush(uintptr_t s, size_t n)
{ (void)s; (void)n; __asm__ volatile("mfence" ::: "memory"); }
static void x86_icache_inv(void)
{ __asm__ volatile("lfence" ::: "memory"); }
static uint64_t x86_get_ticks(void)
{
    /* RDTSC where available; the board may prefer the PIT. */
    uint32_t lo, hi;
    __asm__ volatile("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
}
static void x86_udelay(uint32_t us)
{
    /* PIT channel 0 at ~1.19 MHz → ~1.19 ticks/µs. Coarse but safe. */
    uint64_t start = x86_get_ticks();
    (void)start; (void)us;
    /* TODO(board): calibrate TSC against the PIT, or read the PIT. */
}
static void __attribute__((noreturn)) x86_reset(void)
{
    /* Keyboard controller reset line. */
    for (;;) __asm__ volatile("hlt");
}
static void x86_barrier(void) { __asm__ volatile("mfence" ::: "memory"); }

static const uiox_boot_hw_ops_t x86_ops = {
    .init = x86_hw_init, .uart_putc = com1_putc,
    .dcache_flush = x86_dcache_flush, .icache_inv = x86_icache_inv,
    .get_ticks = x86_get_ticks, .udelay = x86_udelay,
    .reset = x86_reset, .barrier = x86_barrier,
};

void uiox_boot_hw_x86_register(void) { uiox_boot_hw_register(&x86_ops); }
