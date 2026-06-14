/*
 * uiox_boot_hw_arm32.c - ARMv7-A HW ops.
 * PL011 UART at 0x10009000 (QEMU versatilepb).
 */
#include "uiox_boot_hw.h"
#include "uiox_boot_types.h"

#define UART_BASE  0x10009000u
#define UART_DR    (UART_BASE + 0x000u)
#define UART_FR    (UART_BASE + 0x018u)
#define UART_IBRD  (UART_BASE + 0x024u)
#define UART_FBRD  (UART_BASE + 0x028u)
#define UART_LCR   (UART_BASE + 0x02Cu)
#define UART_CR    (UART_BASE + 0x030u)
#define PL011_FR_TXFF (1u << 5)

static inline void mw(uboot_u32_t a, uboot_u32_t v)
{ *(volatile uboot_u32_t *)(uboot_addr_t)(a) = v; }
static inline uboot_u32_t mr(uboot_u32_t a)
{ return *(volatile uboot_u32_t *)(uboot_addr_t)(a); }

static int arm32_init(void)
{
    mw(UART_CR,   0u);
    mw(UART_IBRD, 13u);
    mw(UART_FBRD,  1u);
    mw(UART_LCR,  0x70u);
    mw(UART_CR,   0x301u);
    return UBOOT_OK;
}

static void arm32_uart_putc(char c)
{
    while (mr(UART_FR) & PL011_FR_TXFF);
    mw(UART_DR, (uboot_u32_t)(unsigned char)c);
}

static uboot_u32_t arm32_cpu_id(void)
{
    uboot_u32_t mpidr;
    __asm__ volatile("mrc p15, 0, %0, c0, c0, 5" : "=r"(mpidr));
    return mpidr & 0xFFu;
}

static void arm32_cache_flush(uboot_addr_t s, uboot_size_t n)
{
    uboot_u32_t a = (uboot_u32_t)s & ~31u;
    uboot_u32_t e = (uboot_u32_t)s + n;
    while (a < e) {
        __asm__ volatile("mcr p15,0,%0,c7,c14,1" :: "r"(a) : "memory");
        a += 32u;
    }
    __asm__ volatile("dsb\nisb" ::: "memory");
}

static void arm32_mem_barrier(void)
{ __asm__ volatile("dsb\nisb" ::: "memory"); }

static void arm32_cpu_idle(void)  { __asm__ volatile("wfi"); }
static void arm32_reset(void)     { for(;;); }

static uboot_u32_t arm32_mmio_r32(uboot_addr_t a) { return mr(a); }
static void arm32_mmio_w32(uboot_addr_t a, uboot_u32_t v) { mw(a,v); }

static const uboot_hw_ops_t arm32_ops = {
    .init=arm32_init, .uart_putc=arm32_uart_putc,
    .cpu_id=arm32_cpu_id, .cache_flush=arm32_cache_flush,
    .mem_barrier=arm32_mem_barrier, .cpu_idle=arm32_cpu_idle,
    .reset=arm32_reset, .mmio_read32=arm32_mmio_r32,
    .mmio_write32=arm32_mmio_w32,
};

const uboot_hw_ops_t *g_hw_ops = NULL;

int uboot_hw_init_arm32(void)
{
    g_hw_ops = &arm32_ops;
    return arm32_init();
}
