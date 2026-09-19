/*
 * 01_uBoot/src/arch/arm32/uiox_boot_hw_arm32.c
 *
 * ARM32 (ARMv7-A) hardware ops, driven by the board descriptor.
 * The QEMU-only read_block(address 0) is replaced by the media layer,
 * and all bases come from uiox_board_get().
 */
#include "uiox_boot.h"
#include "uiox_boot_board.h"

/* PL011 (same layout as arm64) */
#define PL011_DR    0x00u
#define PL011_FR    0x18u
#define PL011_IBRD  0x24u
#define PL011_FBRD  0x28u
#define PL011_LCR_H 0x2Cu
#define PL011_CR    0x30u
#define PL011_FR_TXFF   (1u << 5)
#define PL011_CR_UARTEN (1u << 0)
#define PL011_CR_TXE    (1u << 8)
#define PL011_CR_RXE    (1u << 9)

static inline void wr(uint64_t b, uint32_t o, uint32_t v)
{ *((volatile uint32_t *)(uintptr_t)(b + o)) = v; }
static inline uint32_t rd(uint64_t b, uint32_t o)
{ return *((volatile uint32_t *)(uintptr_t)(b + o)); }

static void pl011_init(uint64_t base, uint32_t uart_clk_hz)
{
    uint32_t div = (uart_clk_hz + (16u * 115200u) / 2u) / (16u * 115200u);
    wr(base, PL011_CR, 0u);
    wr(base, PL011_IBRD, div);
    wr(base, PL011_FBRD, 0u);
    wr(base, PL011_LCR_H, (3u << 5) | (1u << 4));  /* 8N1, FIFO */
    wr(base, PL011_CR, PL011_CR_UARTEN | PL011_CR_TXE | PL011_CR_RXE);
}

static void pl011_putc(char c)
{
    uint64_t base = uiox_board_get()->uart_base;
    while (rd(base, PL011_FR) & PL011_FR_TXFF) { }
    wr(base, PL011_DR, (uint32_t)(uint8_t)c);
}

static void arm32_hw_init(void)
{
    /* Board bring-up FIRST: clocks, PLLs, pin-mux. */
    (void)uiox_board_bringup();
    pl011_init(uiox_board_get()->uart_base, 24000000u);
}

/* --- Real storage: forward to the boot-media layer ----------------- */
uiox_boot_err_t uiox_boot_media_read_block(uint32_t blk, uint32_t nblocks,
                                           void *buf);

int uiox_boot_hw_read_block(uint32_t blkno, void *buf)
{
    return uiox_boot_media_read_block(blkno, 1u, buf) == UIOX_BOOT_OK ? 0 : -1;
}

/* --- Cache / maintenance (ARMv7 CP15) ------------------------------ */
static void arm32_dcache_flush(uintptr_t s, size_t n)
{
    uintptr_t end  = s + n;
    uintptr_t line = 32u;
    uintptr_t a    = s & ~(line - 1u);
    while (a < end) {
        __asm__ volatile("mcr p15, 0, %0, c7, c14, 1" :: "r"(a) : "memory");
        a += line;
    }
    __asm__ volatile("dsb" ::: "memory");
}
static void arm32_icache_inv(void)
{
    uint32_t z = 0u;
    __asm__ volatile("mcr p15, 0, %0, c7, c5, 0" :: "r"(z) : "memory");
    __asm__ volatile("dsb; isb" ::: "memory");
}
/* SP804 system timer read (board descriptor supplies the base). */
static uint64_t arm32_get_ticks(void)
{
    uint64_t base = uiox_board_get()->timer_base;
    if (!base) {                       /* fallback: cycle counter */
        uint32_t v;
        __asm__ volatile("mrc p15, 0, %0, c9, c13, 0" : "=r"(v));
        return (uint64_t)v;
    }
    return (uint64_t)rd(base + 0x04u, 0u);   /* Timer1Value */
}
static void arm32_udelay(uint32_t us)
{
    /* SP804 1 MHz tick → 1 µs (board brings the timer up). */
    uint64_t start = arm32_get_ticks();
    while ((arm32_get_ticks() - start) < (uint64_t)us) { }
}
static void __attribute__((noreturn)) arm32_reset(void)
{
    /* TODO(board): write the SoC watchdog reset register. */
    for (;;) __asm__ volatile("wfe");
}
static void arm32_barrier(void) { __asm__ volatile("dsb" ::: "memory"); }

static const uiox_boot_hw_ops_t arm32_ops = {
    .init = arm32_hw_init, .uart_putc = pl011_putc,
    .dcache_flush = arm32_dcache_flush, .icache_inv = arm32_icache_inv,
    .get_ticks = arm32_get_ticks, .udelay = arm32_udelay,
    .reset = arm32_reset, .barrier = arm32_barrier,
};

void uiox_boot_hw_arm32_register(void) { uiox_boot_hw_register(&arm32_ops); }
