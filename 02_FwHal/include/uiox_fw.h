/**
 * @file  uiox_fw.h
 * @brief UIOX Firmware -- umbrella header (types + hw + devsw + clocks + logging).
 */
#ifndef UIOX_FW_H
#define UIOX_FW_H

#include "uiox_fw_types.h"
#include "uiox_fw_hw.h"
#include "uiox_fw_devsw.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ── Clock registry ───────────────────────────────────────────────────────── */
typedef enum {
    UIOX_FW_CLK_CPU=0, UIOX_FW_CLK_BUS=1, UIOX_FW_CLK_UART0=2,
    UIOX_FW_CLK_UART1=3, UIOX_FW_CLK_TIMER0=4, UIOX_FW_CLK_GPIO=5,
    UIOX_FW_CLK_I2C=6, UIOX_FW_CLK_SPI=7, UIOX_FW_CLK_ETH=8,
    UIOX_FW_CLK_STORAGE=9, UIOX_FW_CLK_MAX=10,
} uiox_fw_clk_id_t;

typedef struct {
    uiox_fw_clk_id_t id;
    uint32_t         freq_hz;
    bool             enabled;
    char             name[16];
} uiox_fw_clock_t;

uiox_fw_err_t uiox_fw_clock_init    (void);
uiox_fw_err_t uiox_fw_clock_enable  (uiox_fw_clk_id_t id);
uiox_fw_err_t uiox_fw_clock_disable (uiox_fw_clk_id_t id);
uint32_t      uiox_fw_clock_get_hz  (uiox_fw_clk_id_t id);
uiox_fw_err_t uiox_fw_clock_set_hz  (uiox_fw_clk_id_t id, uint32_t hz);
void          uiox_fw_clock_print   (void);

/* ── Memory helpers (aliases) ─────────────────────────────────────────────── */
static inline void *uiox_fw_memset(void *d,int c,size_t n){return uiox_memset(d,c,n);}
static inline void *uiox_fw_memcpy(void *d,const void *s,size_t n){return uiox_memcpy(d,s,n);}
static inline int   uiox_fw_memcmp(const void *a,const void *b,size_t n){return uiox_memcmp(a,b,n);}

/* ── Debug output ─────────────────────────────────────────────────────────── */
__attribute__((weak)) void uiox_fw_putchar(char c){(void)c;}

static inline void _fw_puts(const char *s)
{if(!s)s="(null)";while(*s)uiox_fw_putchar(*s++);}

static inline void _fw_putu(uint32_t v,int base,int upper)
{char buf[12];int i=11;
 const char *d=upper?"0123456789ABCDEF":"0123456789abcdef";
 buf[i]='\0';if(!v){buf[--i]='0';}else{while(v){buf[--i]=d[v%base];v/=base;}}
 _fw_puts(&buf[i]);}

static __attribute__((unused)) void uiox_fw_printf(const char *fmt,...)
{__builtin_va_list ap;__builtin_va_start(ap,fmt);
 for(;*fmt;fmt++){
   if(*fmt!='%'){uiox_fw_putchar(*fmt);continue;}
   fmt++;if(*fmt=='-')fmt++;while(*fmt>='0'&&*fmt<='9')fmt++;
   switch(*fmt){
   case 's':_fw_puts(__builtin_va_arg(ap,const char*));break;
   case 'c':uiox_fw_putchar((char)__builtin_va_arg(ap,int));break;
   case 'd':{int v=__builtin_va_arg(ap,int);
             if(v<0){uiox_fw_putchar('-');v=-v;}
             _fw_putu((uint32_t)v,10,0);break;}
   case 'u':_fw_putu(__builtin_va_arg(ap,uint32_t),10,0);break;
   case 'x':_fw_putu(__builtin_va_arg(ap,uint32_t),16,0);break;
   case 'X':_fw_putu(__builtin_va_arg(ap,uint32_t),16,1);break;
   case 'p':_fw_puts("0x");
            _fw_putu((uint32_t)(uintptr_t)__builtin_va_arg(ap,void*),16,0);break;
   case '%':uiox_fw_putchar('%');break;
   default:uiox_fw_putchar('%');uiox_fw_putchar(*fmt);break;}}
 __builtin_va_end(ap);}

#ifndef UIOX_FW_NO_LOG
# define FW_LOG(mod,fmt,...) \
    uiox_fw_printf("[FW][" mod "] " fmt "\n",##__VA_ARGS__)
# define FW_ERR(fmt,...) \
    uiox_fw_printf("[FW][ERR] " fmt "\n",##__VA_ARGS__)
#else
# define FW_LOG(mod,fmt,...) ((void)0)
# define FW_ERR(fmt,...)     ((void)0)
#endif

#ifdef __cplusplus
}
#endif
#endif /* UIOX_FW_H */
