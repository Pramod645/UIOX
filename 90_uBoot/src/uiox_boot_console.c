/**
 * @file  uiox_boot_console.c
 * @brief UIOX Bootloader — early serial console and minimal printf.
 * @date  2026-06-12
 */

 #include "uiox_boot.h"
 #include <stdarg.h>
 
 void uiox_boot_console_init(void)
 {
     /* UART was already initialised by uiox_boot_hw_register() → init().
      * Nothing extra to do here; this hook exists for future SPI/I2C
      * consoles that need separate init from the UART. */
 }
 
 void uiox_boot_putc(char c)
 {
     if (c == '\n')
         uiox_boot_hw_uart_putc('\r');
     uiox_boot_hw_uart_putc(c);
 }
 
 void uiox_boot_puts(const char *s)
 {
     while (*s) uiox_boot_putc(*s++);
 }
 
 /* =========================================================================
  * Minimal printf: %s %c %d %u %x %llu %p (no floats, no width)
  * ====================================================================== */
 
 static void print_uint(uint64_t v, uint8_t base, bool upper)
 {
     static const char lo[] = "0123456789abcdef";
     static const char up[] = "0123456789ABCDEF";
     const char *digits = upper ? up : lo;
     char buf[20];
     int  i = 0;
     if (v == 0u) { uiox_boot_putc('0'); return; }
     while (v) { buf[i++] = digits[v % base]; v /= base; }
     while (i--) uiox_boot_putc(buf[i + 1]); /* reverse */
     (void)i;
     /* Fix reversal: print buf backwards */
 }
 
 static void print_uint_fixed(uint64_t v, uint8_t base,
                               int width, bool upper)
 {
     static const char lo[] = "0123456789abcdef";
     static const char up[] = "0123456789ABCDEF";
     const char *digits = upper ? up : lo;
     char buf[64];
     int  n = 0;
     if (v == 0u && width == 0) { uiox_boot_putc('0'); return; }
     uint64_t tmp = v;
     while (tmp) { buf[n++] = digits[tmp % base]; tmp /= base; }
     while (n < width) buf[n++] = '0';
     for (int j = n - 1; j >= 0; j--) uiox_boot_putc(buf[j]);
 }
 
 void uiox_boot_printf(const char *fmt, ...)
 {
     va_list ap;
     va_start(ap, fmt);
     while (*fmt) {
         if (*fmt != '%') { uiox_boot_putc(*fmt++); continue; }
         fmt++;
         bool   is_long = false;
         int    width   = 0;
         if (*fmt == 'l' && *(fmt+1) == 'l') { is_long = true; fmt += 2; }
         else if (*fmt == 'l')               { is_long = true; fmt++;     }
         while (*fmt >= '0' && *fmt <= '9')
             { width = width * 10 + (*fmt - '0'); fmt++; }
         char spec = *fmt++;
         switch (spec) {
         case 'c':
             uiox_boot_putc((char)va_arg(ap, int));
             break;
         case 's': {
             const char *s = va_arg(ap, const char *);
             if (!s) s = "(null)";
             uiox_boot_puts(s);
             break;
         }
         case 'd': {
             int64_t v = is_long ? va_arg(ap, int64_t)
                                 : (int64_t)va_arg(ap, int);
             if (v < 0) { uiox_boot_putc('-'); v = -v; }
             print_uint_fixed((uint64_t)v, 10, width, false);
             break;
         }
         case 'u': {
             uint64_t v = is_long ? va_arg(ap, uint64_t)
                                  : (uint64_t)va_arg(ap, unsigned);
             print_uint_fixed(v, 10, width, false);
             break;
         }
         case 'x': {
             uint64_t v = is_long ? va_arg(ap, uint64_t)
                                  : (uint64_t)va_arg(ap, unsigned);
             print_uint_fixed(v, 16, width, false);
             break;
         }
         case 'X': {
             uint64_t v = is_long ? va_arg(ap, uint64_t)
                                  : (uint64_t)va_arg(ap, unsigned);
             print_uint_fixed(v, 16, width, true);
             break;
         }
         case 'p': {
             uintptr_t v = (uintptr_t)va_arg(ap, void *);
             uiox_boot_puts("0x");
             print_uint_fixed((uint64_t)v, 16,
                               (int)(sizeof(uintptr_t) * 2), false);
             break;
         }
         case '%':
             uiox_boot_putc('%'); break;
         default:
             uiox_boot_putc('%'); uiox_boot_putc(spec); break;
         }
     }
     va_end(ap);
     UIOX_UNUSED(print_uint);
 }
 