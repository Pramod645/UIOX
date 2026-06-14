/*
 * uiox_boot_hw_arm64.c  —  ARM64 HW ops (PL011 @ 0x09000000).
 */
#include "uiox_boot_hw.h"

#define PL011_BASE   0x09000000ULL
#define PL011_DR    (PL011_BASE + 0x000u)
#define PL011_FR    (PL011_BASE + 0x018u)
#define PL011_IBRD  (PL011_BASE + 0x024u)
#define PL011_FBRD  (PL011_BASE + 0x028u)
#define PL011_LCR   (PL011_BASE + 0x02Cu)
#define PL011_CR    (PL011_BASE + 0x030u)
#define PL011_TXFF  (1u << 5)

static inline void     mw(uboot_u64_t a, uboot_u32_t v)
{ *(volatile uboot_u32_t*)(uboot_addr_t)(a)=v; }
static inline uboot_u32_t mr(uboot_u64_t a)
{ return *(volatile uboot_u32_t*)(uboot_addr_t)(a); }

static int a64_init(void)
{
    mw(PL011_CR,   0u);
    mw(PL011_IBRD, 13u);
    mw(PL011_FBRD,  1u);
    mw(PL011_LCR,  0x70u);  /* 8N1 + FIFO */
    mw(PL011_CR,   0x301u); /* TX+RX+UART */
    return UBOOT_OK;
}
static void a64_putc(char c)
{ while(mr(PL011_FR)&PL011_TXFF); mw(PL011_DR,(uboot_u8_t)c); }

static uboot_u32_t a64_cpuid(void)
{ uboot_u64_t v; __asm__("mrs %0,MPIDR_EL1":"=r"(v)); return v&0xFFu; }

static void a64_flush(uboot_addr_t s, uboot_size_t n)
{
    uboot_u64_t a=(uboot_u64_t)s&~63ULL, e=(uboot_u64_t)s+n;
    while(a<e){__asm__("dc civac,%0"::"r"(a):"memory");a+=64;}
    __asm__("dsb sy\nisb":::"memory");
}
static void a64_barrier(void){ __asm__("dsb sy\nisb":::"memory"); }
static void a64_idle(void)   { __asm__("wfi"); }
static void a64_reset(void)
{
    __asm__ volatile("mov x0,#0x84000009\nsmc #0":::"x0","memory");
    for(;;);
}
static uboot_u32_t a64_r32(uboot_addr_t a){ return mr(a); }
static void        a64_w32(uboot_addr_t a,uboot_u32_t v){ mw(a,v); }

static const uboot_hw_ops_t arm64_ops = {
    .init=a64_init,.uart_putc=a64_putc,.cpu_id=a64_cpuid,
    .cache_flush=a64_flush,.mem_barrier=a64_barrier,
    .cpu_idle=a64_idle,.reset=a64_reset,
    .mmio_read32=a64_r32,.mmio_write32=a64_w32,
};
const uboot_hw_ops_t *g_hw_ops = NULL;
int uboot_hw_init_arm64(void){ g_hw_ops=&arm64_ops; return a64_init(); }
