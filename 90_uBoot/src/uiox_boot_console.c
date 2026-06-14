/*
 * uiox_boot_console.c  —  Serial console output (no libc).
 */
#include "uiox_boot_console.h"
#include "uiox_boot_hw.h"
#include <stdarg.h>

void uboot_putc(char c)
{ if(c=='\n') uboot_uart_putc('\r'); uboot_uart_putc(c); }

void uboot_puts(const char *s)
{ while(*s) uboot_putc(*s++); }

void uboot_puthex8(uboot_u8_t v)
{
    static const char h[]="0123456789ABCDEF";
    uboot_putc(h[(v>>4)&0xF]); uboot_putc(h[v&0xF]);
}
void uboot_puthex16(uboot_u16_t v){uboot_puthex8(v>>8);uboot_puthex8(v);}
void uboot_puthex32(uboot_u32_t v){uboot_puthex16(v>>16);uboot_puthex16(v);}
void uboot_puthex64(uboot_u64_t v)
{uboot_puthex32((uboot_u32_t)(v>>32));uboot_puthex32((uboot_u32_t)v);}

void uboot_putdec(uboot_u32_t v)
{
    if(!v){uboot_putc('0');return;}
    char b[12]; int i=0;
    while(v){b[i++]=(char)('0'+v%10);v/=10;}
    while(i--) uboot_putc(b[i]);
}

void uboot_printf(const char *fmt, ...)
{
    va_list ap; va_start(ap,fmt);
    while(*fmt){
        if(*fmt!='%'){uboot_putc(*fmt++);continue;}
        fmt++;
        switch(*fmt++){
        case 'c': uboot_putc((char)va_arg(ap,int));      break;
        case 's': uboot_puts(va_arg(ap,const char*));    break;
        case 'u': uboot_putdec(va_arg(ap,uboot_u32_t)); break;
        case 'x': uboot_puthex32(va_arg(ap,uboot_u32_t));break;
        case 'X': uboot_puthex64(va_arg(ap,uboot_u64_t));break;
        case 'd': {int v=va_arg(ap,int);
                   if(v<0){uboot_putc('-');v=-v;}
                   uboot_putdec((uboot_u32_t)v);break;}
        case '%': uboot_putc('%'); break;
        default:  uboot_putc('?'); break;
        }
    }
    va_end(ap);
}

void uboot_banner(void)
{
    uboot_puts("\r\n");
    uboot_puts("============================================\r\n");
    uboot_puts("  UIOX Bootloader v1.0  (ARM64/ARM32/x86)\r\n");
    uboot_puts("  [github.com](https://github.com/Pramod645/UIOX\r\n)");
    uboot_puts("============================================\r\n");
}
