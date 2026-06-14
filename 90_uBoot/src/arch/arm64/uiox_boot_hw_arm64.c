/*
 * uiox_boot_hw_arm64.c - ARM64 HW ops implementation.
 * PL011 UART at 0x09000000 (QEMU virt machine default).
 */
#include "uiox_boot_hw.h"
#include "uiox_boot_types.h"

/* -- PL011 UART registers ----------------------------------- */
#define PL011_BASE   0x09000000ULL
#define PL011_DR     (PL011_BASE + 0x000u)
#define PL011_FR     (PL011_BASE + 0x018u)
#define PL011_IBRD   (PL011_BASE + 0x024u)
#define PL011_FBRD   (PL011_BASE + 0x028u)
#define PL011_LCR    (PL011_BASE + 0x02Cu)
#define PL011_CR     (PL011_BASE + 0x030u)
#define PL011_FR_TXFF (1u << 5)

static inline void mmio_w32(uboot_u64_t a, uboot_u32_t v)
{ *(volatile uboot_u32_t *)(uboot_addr_t)(a) = v; }
static inline uboot_u32_t mmio_r32(uboot_u64_t a)
{ return *(volatile uboot_u32_t *)(uboot_addr_t)(a); }

static int arm64_init(void)
{
    /* PL011: 115200 @ 24 MHz: IBRD=13, FBRD=1                  */
    mmio_w32(PL011_CR,   0u);          /* disable UART          */
    mmio_w32(PL011_IBRD, 13u);
    mmio_w32(PL011_FBRD,  1u);
    mmio_w32(PL011_LCR,  0x70u);       /* 8N1, FIFO enable      */
    mmio_w32(PL011_CR,   0x301u);      /* TX+RX+UART enable     */
    return UBOOT_OK;
}

static void arm64_uart_putc(char c)
{
    while (mmio_r32(PL011_FR) & PL011_FR_TXFF);
    mmio_w32(PL011_DR, (uboot_u32_t)(unsigned char)c);
}

static uboot_u32_t arm64_cpu_id(void)
{
    uboot_u64_t mpidr;
    __asm__ volatile("mrs %0, MPIDR_EL1" : "=r"(mpidr));
    return (uboot_u32_t)(mpidr & 0xFFu);
}

static void arm64_cache_flush(uboot_addr_t start, uboot_size_t len)
{
    uboot_u64_t a = (uboot_u64_t)start & ~63ULL;
    uboot_u64_t e = (uboot_u64_t)start + len;
    while (a < e) {
        __asm__ volatile("dc civac, %0" :: "r"(a) : "memory");
        a += 64u;
    }
    __asm__ volatile("dsb sy\nisb" ::: "memory");
}

static void arm64_mem_barrier(void)
{ __asm__ volatile("dsb sy\nisb" ::: "memory"); }

static void arm64_cpu_idle(void)
{ __asm__ volatile("wfi"); }

static void arm64_reset(void)
{
    /* PSCI SYSTEM_RESET via SMC */
    __asm__ volatile(
        "mov x0, #0x84000000\n\t"
        "movk x0, #0x0009, lsl #0\n\t"
        "smc #0\n\t" ::: "x0","memory");
    for (;;);
}

static uboot_u32_t arm64_mmio_r32(uboot_addr_t a)
{ return mmio_r32(a); }

static void arm64_mmio_w32(uboot_addr_t a, uboot_u32_t v)
{ mmio_w32(a, v); }

static const uboot_hw_ops_t arm64_ops = {
    .init         = arm64_init,
    .uart_putc    = arm64_uart_putc,
    .cpu_id       = arm64_cpu_id,
    .cache_flush  = arm64_cache_flush,
    .mem_barrier  = arm64_mem_barrier,
    .cpu_idle     = arm64_cpu_idle,
    .reset        = arm64_reset,
    .mmio_read32  = arm64_mmio_r32,
    .mmio_write32 = arm64_mmio_w32,
};

const uboot_hw_ops_t *g_hw_ops = NULL;

int uboot_hw_init_arm64(void)
{
    g_hw_ops = &arm64_ops;
    return arm64_init();
}
