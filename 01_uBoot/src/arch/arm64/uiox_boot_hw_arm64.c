/*
 * 01_uBoot/src/arch/arm64/uiox_boot_hw_arm64.c
 *
 * ARM64 hardware ops, driven by the board descriptor.
 * The QEMU-only read_block(address 0) is replaced by a call into the
 * board's storage backend, and all bases come from uiox_board_get().
 */
#include "uiox_boot.h"
#include "uiox_boot_board.h"

/* PL011 */
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

/* UART divisor depends on the UART clock, which the board bring-up
 * has already locked. 24 MHz / (16 * 115200) = 13. */
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

static void arm64_hw_init(void)
{
    /* Board bring-up FIRST: clocks, PLLs, pin-mux. Without this the
     * UART is not clocked and nothing prints. */
    (void)uiox_board_bringup();
    pl011_init(uiox_board_get()->uart_base, 24000000u);
}

/* --- Real storage: forward to the boot-media layer ----------------- */
/* The media layer decides VirtIO vs SDHCI vs NVMe; this file no longer
 * reads a fixed address. Implemented in boot_media. */
uiox_boot_err_t uiox_boot_media_read_block(uint32_t blk, uint32_t nblocks,
                                           void *buf);

int uiox_boot_hw_read_block(uint32_t blkno, void *buf)
{
    return uiox_boot_media_read_block(blkno, 1u, buf) == UIOX_BOOT_OK ? 0 : -1;
}

static void arm64_dcache_flush(uintptr_t s, size_t n)
{
    for (uintptr_t a = s & ~63u; a < s + n; a += 64u)
        __asm__ volatile("dc civac, %0" :: "r"(a) : "memory");
    __asm__ volatile("dsb sy" ::: "memory");
}
static void arm64_icache_inv(void)
{ __asm__ volatile("ic iallu; dsb sy; isb" ::: "memory"); }
static uint64_t arm64_get_ticks(void)
{ uint64_t v; __asm__ volatile("mrs %0, cntpct_el0" : "=r"(v)); return v; }
static void arm64_udelay(uint32_t us)
{ uint64_t f, s = arm64_get_ticks();
  __asm__ volatile("mrs %0, cntfrq_el0" : "=r"(f));
  while ((arm64_get_ticks() - s) < (uint64_t)us * f / 1000000u) { } }
static void __attribute__((noreturn)) arm64_reset(void)
{ for (;;) __asm__ volatile("wfe"); }
static void arm64_barrier(void) { __asm__ volatile("dsb sy" ::: "memory"); }

static const uiox_boot_hw_ops_t arm64_ops = {
    .init = arm64_hw_init, .uart_putc = pl011_putc,
    .dcache_flush = arm64_dcache_flush, .icache_inv = arm64_icache_inv,
    .get_ticks = arm64_get_ticks, .udelay = arm64_udelay,
    .reset = arm64_reset, .barrier = arm64_barrier,
};

void uiox_boot_hw_arm64_register(void) { uiox_boot_hw_register(&arm64_ops); }
