/*
 * 01_uBoot/src/arch/riscv64/uiox_boot_hw_riscv64.c
 *
 * RISC-V RV64GC hardware ops, driven by the board descriptor.
 * NS16550A UART, CLINT timebase, PLIC. Bases from uiox_board_get().
 */
#include "uiox_boot.h"
#include "uiox_boot_board.h"

/* NS16550A registers */
#define NS_RBR_THR 0x00u
#define NS_IER     0x01u
#define NS_LCR     0x03u
#define NS_MCR     0x04u
#define NS_LSR     0x05u
#define NS_DLL     0x00u
#define NS_DLM     0x01u
#define NS_FCR     0x02u
#define NS_LSR_THRE (1u << 5)

static inline void w8(uint64_t b, uint32_t o, uint8_t v)
{ *((volatile uint8_t *)(uintptr_t)(b + o)) = v; }
static inline uint8_t r8(uint64_t b, uint32_t o)
{ return *((volatile uint8_t *)(uintptr_t)(b + o)); }

static void ns16550_init(uint64_t base, uint32_t uart_clk_hz)
{
    /* divisor = clk / (16 * baud); 3.6864 MHz → 2 */
    uint32_t div = (uart_clk_hz + (16u * 115200u) / 2u) / (16u * 115200u);
    w8(base, NS_IER, 0x00u);
    w8(base, NS_LCR, 0x80u);            /* DLAB = 1 */
    w8(base, NS_DLL, (uint8_t)(div));
    w8(base, NS_DLM, (uint8_t)(div >> 8));
    w8(base, NS_LCR, 0x03u);            /* 8N1 */
    w8(base, NS_FCR, 0xC7u);            /* enable+clear FIFO */
    w8(base, NS_MCR, 0x0Bu);            /* RTS, DTR, OUT2 */
}

static void ns16550_putc(char c)
{
    uint64_t base = uiox_board_get()->uart_base;
    while (!(r8(base, NS_LSR) & NS_LSR_THRE)) { }
    w8(base, NS_RBR_THR, (uint8_t)c);
}

static void riscv64_hw_init(void)
{
    /* Board bring-up FIRST: clocks, PLLs, pin-mux. */
    (void)uiox_board_bringup();
    ns16550_init(uiox_board_get()->uart_base, 3686400u);
}

/* --- Real storage: forward to the boot-media layer ----------------- */
uiox_boot_err_t uiox_boot_media_read_block(uint32_t blk, uint32_t nblocks,
                                           void *buf);

int uiox_boot_hw_read_block(uint32_t blkno, void *buf)
{
    return uiox_boot_media_read_block(blkno, 1u, buf) == UIOX_BOOT_OK ? 0 : -1;
}

/* --- Fences / timebase --------------------------------------------- */
static void riscv64_dcache_flush(uintptr_t s, size_t n)
{ (void)s; (void)n; __asm__ volatile("fence rw, rw" ::: "memory"); }
static void riscv64_icache_inv(void)
{ __asm__ volatile("fence.i" ::: "memory"); }
static uint64_t riscv64_get_ticks(void)
{
    /* CLINT mtime at clk_base + 0xBFF8 (board supplies the base). */
    uint64_t base = uiox_board_get()->clk_base;
    return *((volatile uint64_t *)(uintptr_t)(base + 0xBFF8u));
}
static void riscv64_udelay(uint32_t us)
{
    /* QEMU virt CLINT runs at 10 MHz → 10 ticks/µs. */
    uint64_t start = riscv64_get_ticks();
    while ((riscv64_get_ticks() - start) < (uint64_t)us * 10u) { }
}
static void __attribute__((noreturn)) riscv64_reset(void)
{
    /* SiFive test finisher if the board provides one. */
    uint64_t tb = uiox_board_get()->clk_base;
    if (tb) *((volatile uint32_t *)(uintptr_t)(0x00100000u)) = 0x7777u;
    (void)tb;
    for (;;) __asm__ volatile("wfi");
}
static void riscv64_barrier(void)
{ __asm__ volatile("fence rw, rw" ::: "memory"); }

static const uiox_boot_hw_ops_t riscv64_ops = {
    .init = riscv64_hw_init, .uart_putc = ns16550_putc,
    .dcache_flush = riscv64_dcache_flush, .icache_inv = riscv64_icache_inv,
    .get_ticks = riscv64_get_ticks, .udelay = riscv64_udelay,
    .reset = riscv64_reset, .barrier = riscv64_barrier,
};

void uiox_boot_hw_riscv64_register(void) { uiox_boot_hw_register(&riscv64_ops); }
