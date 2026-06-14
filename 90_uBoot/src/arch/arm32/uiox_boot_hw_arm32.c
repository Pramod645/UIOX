/*
 * uiox_boot_hw_arm32.c  —  ARMv7-A HW ops (PL011 @ 0x10009000).
 */
#include "uiox_boot_hw.h"

#define UART 0x10009000u
static inline void mw(uboot_u32_t a,uboot_u32_t v)
{*(volatile uboot_u32_t*)(uboot_addr_t)(a)=v;}
static inline uboot_u32_t mr(uboot_u32_t a)
{return*(volatile uboot_u32_t*)(uboot_addr_t)(a);}

static int a32_init(void)
{
    mw(UART+0x030u,0); mw(UART+0x024u,13); mw(UART+0x028u,1);
    mw(UART+0x02Cu,0x70u); mw(UART+0x030u,0x301u);
    return UBOOT_OK;
}
static void a32_putc(char c)
{while(mr(UART+0x018u)&(1u<<5));mw(UART,(uboot_u8_t)c);}
static uboot_u32_t a32_cpuid(void)
{uboot_u32_t v;__asm__("mrc p15,0,%0,c0,c0,5":"=r"(v));return v&0xFFu;}
static void a32_flush(uboot_addr_t s,uboot_size_t n)
{uboot_u32_t a=(uboot_u32_t)s&~31u,e=(uboot_u32_t)s+n;
while(a<e){__asm__("mcr p15,0,%0,c7,c14,1"::"r"(a):"memory");a+=32;}
__asm__("dsb\nisb":::"memory");}
static void a32_barrier(void){__asm__("dsb\nisb":::"memory");}
static void a32_idle(void){__asm__("wfi");}
static void a32_reset(void){for(;;);}
static uboot_u32_t a32_r32(uboot_addr_t a){return mr(a);}
static void        a32_w32(uboot_addr_t a,uboot_u32_t v){mw(a,v);}

static const uboot_hw_ops_t arm32_ops={
    .init=a32_init,.uart_putc=a32_putc,.cpu_id=a32_cpuid,
    .cache_flush=a32_flush,.mem_barrier=a32_barrier,
    .cpu_idle=a32_idle,.reset=a32_reset,
    .mmio_read32=a32_r32,.mmio_write32=a32_w32,
};
const uboot_hw_ops_t *g_hw_ops=NULL;
int uboot_hw_init_arm32(void){g_hw_ops=&arm32_ops;return a32_init();}
