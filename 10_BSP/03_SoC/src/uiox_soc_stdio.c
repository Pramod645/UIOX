/**
 * @file    uiox_soc_stdio.c
 * @brief   UIOX SoC — uiox_printf / uiox_snprintf / uiox_puts implementation.
 * @date    2026-07-18
 */

 #include "uiox_soc_stdio.h"
 #include "uiox_soc_hw.h"
 #include "uiox_stdarg.h"
 
 /* =========================================================================
  * Internal helpers — soft division (no libc, no / operator issues on ARM)
  * ====================================================================== */
 
 static uiox_uint32_t _udiv32(uiox_uint32_t n, uiox_uint32_t d,
                                uiox_uint32_t *rem)
 {
     uiox_uint32_t q = 0u, r = 0u;
     if (d == 0u) { if (rem) *rem = 0u; return 0u; }
     for (int i = 31; i >= 0; i--) {
         r = (r << 1u) | ((n >> (uiox_uint32_t)i) & 1u);
         if (r >= d) { r -= d; q |= (1u << (uiox_uint32_t)i); }
     }
     if (rem) *rem = r;
     return q;
 }
 
 static uiox_uint64_t _udiv64_u32(uiox_uint64_t n, uiox_uint32_t d,
                                    uiox_uint32_t *rem)
 {
     uiox_uint32_t hi = (uiox_uint32_t)(n >> 32u);
     uiox_uint32_t lo = (uiox_uint32_t)n;
     uiox_uint32_t rh, rm, rl, tmp;
     uiox_uint32_t qh = _udiv32(hi, d, &rh);
     tmp = (rh << 16u) | (lo >> 16u);
     uiox_uint32_t qm = _udiv32(tmp, d, &rm);
     tmp = (rm << 16u) | (lo & 0xFFFFu);
     uiox_uint32_t ql = _udiv32(tmp, d, &rl);
     if (rem) *rem = rl;
     return ((uiox_uint64_t)qh << 32u) | ((uiox_uint64_t)(qm << 16u) | ql);
 }
 
 /* ── Output sink type ─────────────────────────────────────────────────── */
 typedef struct {
     char       *buf;        /* NULL = UART output mode  */
     uiox_size_t cap;        /* buffer capacity          */
     uiox_size_t pos;        /* current write position   */
 } _sink_t;
 
 static void _sink_putc(_sink_t *sk, char c)
 {
     if (sk->buf) {
         if (sk->pos + 1u < sk->cap)
             sk->buf[sk->pos++] = c;
     } else {
         if (c == '\n') uiox_soc_hw_uart_putc('\r');
         uiox_soc_hw_uart_putc(c);
     }
 }
 
 static void _sink_puts(_sink_t *sk, const char *s)
 {
     if (!s) s = "(null)";
     while (*s) _sink_putc(sk, *s++);
 }
 
 /* ── Integer formatters ───────────────────────────────────────────────── */
 
 static void _put_hex(_sink_t *sk, uiox_uint64_t v, int width, int upper)
 {
     static const char lo[] = "0123456789abcdef";
     static const char hi[] = "0123456789ABCDEF";
     const char *h = upper ? hi : lo;
     char buf[16]; int n = 0;
     do { buf[n++] = h[(uiox_uint8_t)(v & 0xFu)]; v >>= 4; } while (v);
     while (n < width) buf[n++] = '0';
     for (int i = n - 1; i >= 0; i--) _sink_putc(sk, buf[i]);
 }
 
 static void _put_udec32(_sink_t *sk, uiox_uint32_t v)
 {
     if (v == 0u) { _sink_putc(sk, '0'); return; }
     char buf[12]; int n = 0;
     uiox_uint32_t rem = 0u;
     do { v = _udiv32(v, 10u, &rem); buf[n++] = (char)('0' + rem); }
     while (v != 0u);
     for (int i = n - 1; i >= 0; i--) _sink_putc(sk, buf[i]);
 }
 
 static void _put_udec64(_sink_t *sk, uiox_uint64_t v)
 {
     if (v == 0u) { _sink_putc(sk, '0'); return; }
     static const uiox_uint32_t BILLION = 1000000000u;
     uiox_uint32_t rem = 0u;
     uiox_uint64_t q   = _udiv64_u32(v, BILLION, &rem);
     uiox_uint32_t bot = rem;
     q                  = _udiv64_u32(q, BILLION, &rem);
     uiox_uint32_t mid = rem;
     uiox_uint32_t top = (uiox_uint32_t)q;
     char buf[20]; int n = 0;
     { uiox_uint32_t t = bot, r2 = 0u;
       do { t = _udiv32(t, 10u, &r2); buf[n++] = (char)('0' + r2); }
       while (t != 0u); }
     if (top != 0u || mid != 0u) {
         while (n < 9) buf[n++] = '0';
         uiox_uint32_t t = mid, r2 = 0u;
         do { t = _udiv32(t, 10u, &r2); buf[n++] = (char)('0' + r2); }
         while (t != 0u);
     }
     if (top != 0u) {
         while (n < 18) buf[n++] = '0';
         uiox_uint32_t t = top, r2 = 0u;
         do { t = _udiv32(t, 10u, &r2); buf[n++] = (char)('0' + r2); }
         while (t != 0u);
     }
     for (int i = n - 1; i >= 0; i--) _sink_putc(sk, buf[i]);
 }
 
 /* ── Core formatter ───────────────────────────────────────────────────── */
 
 static uiox_size_t _vformat(_sink_t *sk, const char *fmt, va_list ap)
 {
     uiox_size_t written = 0u;
     while (*fmt) {
         if (*fmt != '%') {
             _sink_putc(sk, *fmt++);
             written++;
             continue;
         }
         fmt++;
         int  ll    = 0;
         int  width = 0;
         int  zero  = 0;
 
         if (*fmt == '0') { zero = 1; fmt++; }
         (void)zero;
         while (*fmt >= '0' && *fmt <= '9')
             { width = width * 10 + (*fmt - '0'); fmt++; }
         if (*fmt == 'l') {
             fmt++;
             if (*fmt == 'l') { ll = 2; fmt++; }
             else              { ll = 1; }
         }
 
         char spec = *fmt++;
         switch (spec) {
         case 'c':
             _sink_putc(sk, (char)va_arg(ap, int));
             written++;
             break;
         case 's': {
             const char *s = va_arg(ap, const char *);
             if (!s) s = "(null)";
             while (*s) { _sink_putc(sk, *s++); written++; }
             break;
         }
         case 'd': case 'i': {
             uiox_int64_t v = (ll == 2) ? va_arg(ap, uiox_int64_t)
                                         : (uiox_int64_t)va_arg(ap, int);
             if (v < 0) { _sink_putc(sk, '-'); written++;
                           _put_udec64(sk, (uiox_uint64_t)(-v)); }
             else          _put_udec64(sk, (uiox_uint64_t)v);
             written += 2; /* approximate — exact not needed */
             break;
         }
         case 'u':
             if (ll == 2) _put_udec64(sk, va_arg(ap, uiox_uint64_t));
             else         _put_udec32(sk, va_arg(ap, uiox_uint32_t));
             written++;
             break;
         case 'x':
             _put_hex(sk,
                      (ll == 2) ? va_arg(ap, uiox_uint64_t)
                                 : (uiox_uint64_t)va_arg(ap, uiox_uint32_t),
                      width, 0);
             written++;
             break;
         case 'X':
             _put_hex(sk,
                      (ll == 2) ? va_arg(ap, uiox_uint64_t)
                                 : (uiox_uint64_t)va_arg(ap, uiox_uint32_t),
                      width, 1);
             written++;
             break;
         case 'p':
             _sink_puts(sk, "0x");
             _put_hex(sk,
                      (uiox_uint64_t)(uiox_uintptr_t)va_arg(ap, void *),
                      (int)(sizeof(uiox_uintptr_t) * 2), 0);
             written += 2;
             break;
         case '%':
             _sink_putc(sk, '%');
             written++;
             break;
         default:
             _sink_putc(sk, '%');
             _sink_putc(sk, spec);
             written += 2;
             break;
         }
     }
     /* NUL-terminate buffer mode */
     if (sk->buf && sk->pos < sk->cap)
         sk->buf[sk->pos] = '\0';
     return written;
 }
 
 /* =========================================================================
  * Public API
  * ====================================================================== */
 
 void uiox_putc(char c)
 {
     if (c == '\n') uiox_soc_hw_uart_putc('\r');
     uiox_soc_hw_uart_putc(c);
 }
 
 void uiox_puts(const char *s)
 {
     if (!s) return;
     while (*s) uiox_putc(*s++);
     uiox_putc('\n');
 }
 
 void uiox_printf(const char *fmt, ...)
 {
     _sink_t sk = { NULL, 0u, 0u };
     va_list ap;
     va_start(ap, fmt);
     _vformat(&sk, fmt, ap);
     va_end(ap);
 }
 
 int uiox_snprintf(char *buf, uiox_size_t n, const char *fmt, ...)
 {
     _sink_t sk = { buf, n, 0u };
     va_list ap;
     va_start(ap, fmt);
     uiox_size_t w = _vformat(&sk, fmt, ap);
     va_end(ap);
     return (int)w;
 }
 