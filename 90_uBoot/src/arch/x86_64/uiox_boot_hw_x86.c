/*
 * uiox_boot_hw_x86.c - x86-64 HW ops.
 * COM1 (0x3F8) 16550 UART.
 */
#include "uiox_boot_hw.h"
#include "uiox_boot_types.h"

#define COM1_BASE  0x3F8u
#define COM1_RBR   (COM1_BASE + 0)
#define COM1_THR   (COM1_BASE + 0)
#define COM1_IER   (COM1_BASE + 1)
#define COM1_FCR   (COM1_BASE + 2)
#define COM1_LCR   (COM1_BASE + 3)
#define COM1_LSR   (COM1_BASE + 5)
#define LSR_THRE   (1u << 5)

static inline void outb(uboot_u16_t p, uboot_u8_t v)
{ __asm__ volatile("outb %0,%1" :: "a"(v), "Nd"(p)); }

static inline uboot_u8_t inb(uboot_u16_t p)
{ uboot_u8_t v; __asm__ volatile("inb %1,%0" : "=a"(v) : "Nd"(p)); return v; }

static int x86_init(void)
{
    outb(COM1_IER, 0x00);   /* disable interrupts               */
    outb(COM1_LCR, 0x80);   /* enable DLAB                      */
    outb(COM1_RBR, 0x01);   /* baud divisor lo (115200)         */
    outb(COM1_IER, 0x00);   /* baud divisor hi                  */
    outb(COM1_LCR, 0x03);   /* 8N1                              */
    outb(COM1_FCR, 0xC7);   /* enable FIFO                      */
    return UBOOT_OK;
}

static void x86_uart_putc(char c)
{
    while (!(inb(COM1_LSR) & LSR_THRE));
    outb(COM1_THR, (uboot_u8_t)c);
}

static uboot_u32_t x86_cpu_id(void)
{
    uboot_u32_t eax,ebx,ecx,edx;
    __asm__ volatile("cpuid"
        : "=a"(eax),"=b"(ebx),"=c"(ecx),"=d"(edx)
        : "a"(1u),"c"(0u));
    return (ebx >> 24) & 0xFFu;
}

static void x86_cache_flush(uboot_addr_t s, uboot_size_t n)
{
    uboot_u64_t a = (uboot_u64_t)s & ~63ULL;
    uboot_u64_t e = (uboot_u64_t)s + n;
    while (a < e) {
        __asm__ volatile("clflushopt (%0)" :: "r"((void*)(uboot_addr_t)a) : "memory");
        a += 64u;
    }
    __asm__ volatile("mfence" ::: "memory");
}

static void x86_mem_barrier(void) { __asm__ volatile("mfence" ::: "memory"); }
static void x86_cpu_idle(void)    { __asm__ volatile("hlt"); }
static void x86_reset(void)
{
    outb(0x64, 0xFE);   /* keyboard controller reset line       */
    for(;;);
}

static uboot_u32_t x86_mmio_r32(uboot_addr_t a)
{ return *(volatile uboot_u32_t *)(uboot_addr_t)(a); }
static void x86_mmio_w32(uboot_addr_t a, uboot_u32_t v)
{ *(volatile uboot_u32_t *)(uboot_addr_t)(a) = v; }

static const uboot_hw_ops_t x86_ops = {
    .init=x86_init, .uart_putc=x86_uart_putc,
    .cpu_id=x86_cpu_id, .cache_flush=x86_cache_flush,
    .mem_barrier=x86_mem_barrier, .cpu_idle=x86_cpu_idle,
    .reset=x86_reset, .mmio_read32=x86_mmio_r32,
    .mmio_write32=x86_mmio_w32,
};

const uboot_hw_ops_t *g_hw_ops = NULL;

int uboot_hw_init_x86(void)
{
    g_hw_ops = &x86_ops;
    return x86_init();
}
