/**
 * @file  uiox_boot_console.c
 * @brief UIOX Bootloader — early serial console and minimal printf.
 *
 * ARM32 bare-metal: ZERO use of / or % operators anywhere.
 * All division is done with udiv32() which uses only shifts and
 * subtractions — no __aeabi_uidivmod / __aeabi_idivmod / __aeabi_uldivmod.
 *
 * @version 1.0.2
 * @date    2026-06-19
 */

 #include "uiox_boot.h"

 /* va_list from compiler built-ins — no <stdarg.h> needed */
 typedef __builtin_va_list  va_list;
 #define va_start(v,l)   __builtin_va_start(v,l)
 #define va_end(v)       __builtin_va_end(v)
 #define va_arg(v,t)     __builtin_va_arg(v,t)
 
 /* =========================================================================
  * Software 32-bit unsigned division — NO / or % operator.
  *
  * Returns quotient.  Writes remainder to *rem_out (may be NULL).
  * Uses binary long-division (shift-and-subtract) — pure 32-bit,
  * never calls any compiler helper.
  * ====================================================================== */
 static uint32_t udiv32(uint32_t n, uint32_t d, uint32_t *rem_out)
 {
     uint32_t q = 0u;
     uint32_t r = 0u;
 
     if (d == 0u) {
         /* division by zero — return 0 to avoid infinite loop    */
         if (rem_out) *rem_out = 0u;
         return 0u;
     }
 
     for (int i = 31; i >= 0; i--) {
         r = (r << 1u) | ((n >> (uint32_t)i) & 1u);
         if (r >= d) {
             r -= d;
             q |= (1u << (uint32_t)i);
         }
     }
 
     if (rem_out) *rem_out = r;
     return q;
 }
 
 /* =========================================================================
  * 64-bit ÷ 32-bit long division — also zero / or % operators.
  *
  * Uses udiv32() on each 32-bit half.  Returns quotient, writes
  * remainder to *rem_out (may be NULL).
  * ====================================================================== */
 static uint64_t div64_u32(uint64_t n, uint32_t d, uint32_t *rem_out)
 {
     uint32_t hi  = (uint32_t)(n >> 32u);
     uint32_t lo  = (uint32_t)(n);
     uint32_t rh  = 0u;
     uint32_t qh  = udiv32(hi, d, &rh);
 
     /* Combine remainder of high half with lo:
      *   value = rh * 2^32 + lo
      * Split lo into two 16-bit halves to avoid overflow.         */
     uint32_t tmp, qm, rm, ql, rl;
 
     /* Process upper 16 bits of lo */
     tmp = (rh << 16u) | (lo >> 16u);
     qm  = udiv32(tmp, d, &rm);
 
     /* Process lower 16 bits of lo */
     tmp = (rm << 16u) | (lo & 0xFFFFu);
     ql  = udiv32(tmp, d, &rl);
 
     /* Combine the two 16-bit quotient pieces */
     uint32_t qlo = (qm << 16u) | ql;
 
     if (rem_out) *rem_out = rl;
     return ((uint64_t)qh << 32u) | (uint64_t)qlo;
 }
 
 /* =========================================================================
  * Console init / putc / puts
  * ====================================================================== */
 void uiox_boot_console_init(void)
 {
     /* UART already up from hw_register()->init(). Reserved hook. */
 }
 
 void uiox_boot_putc(char c)
 {
     if (c == '\n')
         uiox_boot_hw_uart_putc('\r');
     uiox_boot_hw_uart_putc(c);
 }
 
 void uiox_boot_puts(const char *s)
 {
     if (!s) return;
     while (*s) uiox_boot_putc(*s++);
 }
 
 /* =========================================================================
  * Hex formatter — shift/mask only, zero division
  * ====================================================================== */
 static const char s_hex_lo[] = "0123456789abcdef";
 static const char s_hex_up[] = "0123456789ABCDEF";
 
 static void fmt_hex(uint64_t v, int width, int upper)
 {
     const char *digits = upper ? s_hex_up : s_hex_lo;
     char buf[16];
     int  n = 0;
 
     do {
         buf[n++] = digits[(uint8_t)(v & 0xFu)];
         v >>= 4u;
     } while (v != 0u);
 
     while (n < width)
         buf[n++] = '0';
 
     for (int i = n - 1; i >= 0; i--)
         uiox_boot_putc(buf[i]);
 }
 
 /* =========================================================================
  * Decimal formatter — uses udiv32 / div64_u32, no / or % operators
  *
  * Splits a uint64 into three groups of 9 digits (max 27 digits,
  * but uint64 only needs 20) using 10^9 as the group divisor.
  * ====================================================================== */
 #define DEC_GROUP  1000000000u   /* 10^9 — fits in uint32 */
 
 static void fmt_dec(uint64_t v)
 {
     if (v == 0u) { uiox_boot_putc('0'); return; }
 
     uint32_t rem  = 0u;
     uint32_t bot, mid, top_v;
     uint64_t q;
 
     /* bottom 9 digits */
     q   = div64_u32(v, DEC_GROUP, &rem);
     bot = rem;
 
     /* middle 9 digits */
     q   = div64_u32(q, DEC_GROUP, &rem);
     mid = rem;
 
     /* top (at most 2 digits for uint64 max ≈ 1.8×10^19) */
     top_v = (uint32_t)q;
 
     char buf[20];
     int  n = 0;
 
     /* ── bottom group ── */
     {
         uint32_t tmp = bot;
         uint32_t r2  = 0u;
         do {
             udiv32(tmp, 10u, &r2);
             buf[n++] = s_hex_lo[r2];
             tmp = udiv32(tmp, 10u, NULL);
         } while (tmp != 0u);
     }
 
     if (top_v != 0u || mid != 0u) {
         /* pad bottom group to 9 digits */
         while (n < 9) buf[n++] = '0';
 
         /* ── middle group ── */
         uint32_t tmp = mid;
         uint32_t r2  = 0u;
         do {
             udiv32(tmp, 10u, &r2);
             buf[n++] = s_hex_lo[r2];
             tmp = udiv32(tmp, 10u, NULL);
         } while (tmp != 0u);
     }
 
     if (top_v != 0u) {
         /* pad to 18 digits total */
         while (n < 18) buf[n++] = '0';
 
         /* ── top group ── */
         uint32_t tmp = top_v;
         uint32_t r2  = 0u;
         do {
             udiv32(tmp, 10u, &r2);
             buf[n++] = s_hex_lo[r2];
             tmp = udiv32(tmp, 10u, NULL);
         } while (tmp != 0u);
     }
 
     for (int i = n - 1; i >= 0; i--)
         uiox_boot_putc(buf[i]);
 }
 
 /* =========================================================================
  * Minimal printf
  *
  * Supported: %c %s %d %u %x %X %p %% %lld %llu %llx %llX %ld %lu
  * Width field supported for %x/%X (zero-pad).
  * NO floating point.  NO __aeabi_* calls.
  * ====================================================================== */
 void uiox_boot_printf(const char *fmt, ...)
 {
     va_list ap;
     va_start(ap, fmt);
 
     while (*fmt) {
         if (*fmt != '%') {
             uiox_boot_putc(*fmt++);
             continue;
         }
         fmt++;  /* consume '%' */
 
         int is_upper    = 0;
         int is_longlong = 0;
         int is_long     = 0;
         int is_signed   = 0;
         int width       = 0;
 
         /* zero-pad flag */
         if (*fmt == '0') fmt++;
 
         /* width digits */
         while (*fmt >= '1' && *fmt <= '9') {
             width = width * 10 + (*fmt - '0');
             fmt++;
         }
 
         /* length modifier */
         if (fmt[0] == 'l' && fmt[1] == 'l') {
             is_longlong = 1; fmt += 2;
         } else if (*fmt == 'l') {
             is_long = 1; fmt++;
         }
 
         char spec = *fmt++;
         switch (spec) {
 
         case 'c':
             uiox_boot_putc((char)va_arg(ap, int));
             break;
 
         case 's': {
             const char *s = va_arg(ap, const char *);
             uiox_boot_puts(s ? s : "(null)");
             break;
         }
 
         case 'd':
             is_signed = 1;
             /* fall-through */
         case 'u': {
             uint64_t uval;
             if (is_longlong)
                 uval = (uint64_t)va_arg(ap, unsigned long long);
             else if (is_long)
                 uval = (uint64_t)va_arg(ap, unsigned long);
             else
                 uval = (uint64_t)va_arg(ap, unsigned int);
 
             if (is_signed) {
                 int64_t sval;
                 if      (is_longlong) sval = (int64_t)uval;
                 else if (is_long)     sval = (int64_t)(long)uval;
                 else                  sval = (int64_t)(int)uval;
                 if (sval < 0) {
                     uiox_boot_putc('-');
                     uval = (uint64_t)(-sval);
                 } else {
                     uval = (uint64_t)sval;
                 }
             }
             fmt_dec(uval);
             break;
         }
 
         case 'X':
             is_upper = 1;
             /* fall-through */
         case 'x': {
             uint64_t uval;
             if (is_longlong)
                 uval = (uint64_t)va_arg(ap, unsigned long long);
             else if (is_long)
                 uval = (uint64_t)va_arg(ap, unsigned long);
             else
                 uval = (uint64_t)va_arg(ap, unsigned int);
             fmt_hex(uval, width, is_upper);
             break;
         }
 
         case 'p': {
            __UINTPTR_TYPE__ pval = (__UINTPTR_TYPE__)va_arg(ap, void *);
             uiox_boot_puts("0x");
             fmt_hex((uint64_t)pval,
                     (int)(sizeof(uintptr_t) * 2u), 0);
             break;
         }
 
         case '%':
             uiox_boot_putc('%');
             break;
 
         default:
             uiox_boot_putc('%');
             uiox_boot_putc(spec);
             break;
         }
     }
 
     va_end(ap);
 }
 
 void uiox_boot_banner(void)
 {
     uiox_boot_puts("\r\n");
     uiox_boot_puts("============================================\r\n");
     uiox_boot_puts("  UIOX Bootloader v1.0\r\n");
     uiox_boot_puts("  ARM64 / ARM32 / x86-64\r\n");
     uiox_boot_puts("  [github.com](https://github.com/Pramod645/UIOX\r\n)");
     uiox_boot_puts("============================================\r\n");
 }
 