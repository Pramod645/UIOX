/*
 * uiox_boot_hw_x86.c  —  x86-64 HW ops (COM1 16550 @ 0x3F8).
 */
#include "uiox_boot_hw.h"

#define COM1 0x3F8u
static inline void ob(uboot_u16_t p,uboot_u8_t v)
{__asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p));}
static inline uboot_u8_t ib(uboot_u16_t p)
{uboot_u8_t v;__asm__ volatile("inb %1,%0":"=a"(v):"Nd"(p));return v;}

static int x86_init(void)
{
    ob(COM1+1,0x00); ob(COM1+3,0x80);
    ob(COM1+0,0x01); ob(COM1+1,0x00);
    ob(COM1+3,0x03); ob(COM1+2,0xC7);
    return UBOOT_OK;
}
static void x86_putc(char c)
{while(!(ib(COM1+5)&0x20));ob(COM1,(uboot_u8_t)c);}
static uboot_u32_t x86_cpuid(void)
{uboot_u32_t a,b,c,d;
__asm__("cpuid":"=a"(a),"=b"(b),"=c"(c),"=d"(d):"a"(1),"c"(0));
return (b>>24)&0xFFu;}
static void x86_flush(uboot_addr_t s,uboot_size_t n)
{uboot_u64_t a=(uboot_u64_t)s&~63ULL,e=(uboot_u64_t)s+n;
while(a<e){__asm__("clflushopt (%0)"::"r"((void*)(uboot_addr_t)a):"memory");a+=64;}
__asm__("mfence":::"memory");}
static void x86_barrier(void){__asm__("mfence":::"memory");}
static void x86_idle(void){__asm__("hlt");}
static void x86_reset(void){ob(0x64,0xFE);for(;;);}
static uboot_u32_t x86_r32(uboot_addr_t a)
{return*(volatile uboot_u32_t*)(uboot_addr_t)(a);}
static void x86_w32(uboot_addr_t a,uboot_u32_t v)
{*(volatile uboot_u32_t*)(uboot_addr_t)(a)=v;}

static const uboot_hw_ops_t x86_ops={
    .init=x86_init,.uart_putc=x86_putc,.cpu_id=x86_cpuid,
    .cache_flush=x86_flush,.mem_barrier=x86_barrier,
    .cpu_idle=x86_idle,.reset=x86_reset,
    .mmio_read32=x86_r32,.mmio_write32=x86_w32,
};
const uboot_hw_ops_t *g_hw_ops=NULL;
int uboot_hw_init_x86(void){g_hw_ops=&x86_ops;return x86_init();}
