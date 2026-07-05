/**
 * @file  uiox_uart_proto.c
 * @brief UIOX UART Protocol — line discipline, ANSI escape, echo.
 * @date  2026-07-05
 */

 #include "uiox_uart_device.h"
 #include <string.h>
 #include <stdarg.h>
 #include <stdio.h>
 #include <errno.h>
 
 /* =========================================================================
  * ANSI colour codes
  * ====================================================================== */
 
 /* fg: 30–37, bg: 40–47 */
 #define ANSI_RESET      "\033[0m"
 #define ANSI_BOLD       "\033[1m"
 #define ANSI_FG(n)      "\033[3" #n "m"
 #define ANSI_BG(n)      "\033[4" #n "m"
 
 int uiox_uart_proto_init(uiox_uart_proto_t *proto, uiox_uart_if_t *uif)
 {
     if (!proto || !uif) return -EINVAL;
     memset(proto, 0, sizeof(*proto));
     proto->uif      = uif;
     proto->ldisc    = UIOX_UART_LDISC_COOKED;
     proto->echo     = true;
     proto->c_erase  = 0x7Fu;  /* DEL */
     proto->c_kill   = 0x15u;  /* ^U  */
     proto->c_intr   = 0x03u;  /* ^C  */
     proto->c_eof    = 0x04u;  /* ^D  */
     proto->c_eol    = '\n';
     proto->ansi.state = UIOX_ANSI_GROUND;
     return 0;
 }
 
 int uiox_uart_proto_set_ldisc(uiox_uart_proto_t *proto,
                                 uiox_uart_ldisc_t mode)
 {
     if (!proto) return -EINVAL;
     proto->ldisc = mode;
     return 0;
 }
 
 void uiox_uart_proto_set_echo(uiox_uart_proto_t *proto, bool echo)
 { if (proto) proto->echo = echo; }
 
 /* ── ANSI escape sequence parser ────────────────────────────────── */
 
 static void ansi_reset(uiox_ansi_parser_t *p)
 {
     p->state       = UIOX_ANSI_GROUND;
     p->buf_len     = 0u;
     p->param_count = 0u;
     p->final_byte  = 0;
     memset(p->params, 0, sizeof(p->params));
 }
 
 static bool ansi_feed(uiox_ansi_parser_t *p, char c)
 {
     switch (p->state) {
     case UIOX_ANSI_GROUND:
         if (c == 0x1Bu) { p->state = UIOX_ANSI_ESC; return false; }
         return true;   /* Normal character */
 
     case UIOX_ANSI_ESC:
         if (c == '[') { p->state = UIOX_ANSI_CSI; return false; }
         if (c == ']') { p->state = UIOX_ANSI_OSC; return false; }
         ansi_reset(p);
         return true;
 
     case UIOX_ANSI_CSI:
         if (c >= '0' && c <= '9') {
             if (p->param_count == 0u) p->param_count = 1u;
             p->params[p->param_count - 1u] =
                 p->params[p->param_count - 1u] * 10 + (c - '0');
         } else if (c == ';') {
             if (p->param_count < UIOX_ANSI_PARAM_MAX) p->param_count++;
         } else if (c >= 0x40u && c <= 0x7Eu) {
             /* Final byte: sequence complete */
             p->final_byte = c;
             p->state      = UIOX_ANSI_COMPLETE;
         } else {
             ansi_reset(p);
         }
         return false;
 
     case UIOX_ANSI_OSC:
         /* Consume until BEL (0x07) or ST (ESC \) */
         if (c == 0x07u || c == 0x1Bu) ansi_reset(p);
         return false;
 
     default:
         ansi_reset(p);
         return true;
     }
 }
 
 /* ── Transmit helpers ──────────────────────────────────────────── */
 
 int uiox_uart_proto_putc(uiox_uart_proto_t *proto, char c)
 {
     if (!proto) return -EINVAL;
     if (c == '\n')
         uiox_uart_if_putc(proto->uif, '\r');
     return uiox_uart_if_putc(proto->uif, c);
 }
 
 int uiox_uart_proto_puts(uiox_uart_proto_t *proto, const char *s)
 {
     if (!proto || !s) return -EINVAL;
     int n = 0;
     while (*s) { uiox_uart_proto_putc(proto, *s++); n++; }
     return n;
 }
 
 int uiox_uart_proto_printf(uiox_uart_proto_t *proto, const char *fmt, ...)
 {
     if (!proto || !fmt) return -EINVAL;
     char buf[256];
     va_list ap;
     va_start(ap, fmt);
     int n = vsnprintf(buf, sizeof(buf), fmt, ap);
     va_end(ap);
     uiox_uart_proto_puts(proto, buf);
     return n;
 }
 
 /* ── ANSI cursor / colour helpers ───────────────────────────────── */
 
 void uiox_uart_proto_cursor_up(uiox_uart_proto_t *proto, uint32_t n)
 { uiox_uart_proto_printf(proto, "\033[%uA", n); }
 
 void uiox_uart_proto_cursor_dn(uiox_uart_proto_t *proto, uint32_t n)
 { uiox_uart_proto_printf(proto, "\033[%uB", n); }
 
 void uiox_uart_proto_cursor_col(uiox_uart_proto_t *proto, uint32_t col)
 { uiox_uart_proto_printf(proto, "\033[%uG", col); }
 
 void uiox_uart_proto_clear_line(uiox_uart_proto_t *proto)
 { uiox_uart_proto_puts(proto, "\033[2K"); }
 
 void uiox_uart_proto_clear_scr(uiox_uart_proto_t *proto)
 { uiox_uart_proto_puts(proto, "\033[2J\033[H"); }
 
 void uiox_uart_proto_set_colour(uiox_uart_proto_t *proto,
                                   uint8_t fg, uint8_t bg)
 { uiox_uart_proto_printf(proto, "\033[%u;%um", 30u + fg, 40u + bg); }
 
 void uiox_uart_proto_reset_attr(uiox_uart_proto_t *proto)
 { uiox_uart_proto_puts(proto, "\033[0m"); }
 
 /* ── Receive: line discipline processing ────────────────────────── */
 
 int uiox_uart_proto_process_rx(uiox_uart_proto_t *proto)
 {
     if (!proto) return -EINVAL;
     int processed = 0;
 
     while (uiox_uart_if_rx_avail(proto->uif) > 0u) {
         int raw = uiox_uart_if_getc(proto->uif);
         if (raw < 0) break;
         char c = (char)raw;
         processed++;
 
         if (proto->ldisc == UIOX_UART_LDISC_RAW) {
             /* Raw: just buffer into line (no processing) */
             if (proto->line_len < UIOX_UART_LINE_BUF_SIZE - 1u)
                 proto->line_buf[proto->line_len++] = c;
             proto->line_ready = true;
             continue;
         }
 
         /* Feed into ANSI parser */
         bool is_printable = ansi_feed(&proto->ansi, c);
         if (proto->ansi.state == UIOX_ANSI_COMPLETE) {
             proto->ansi_sequences++;
             ansi_reset(&proto->ansi);
             is_printable = false;
         }
         if (!is_printable) continue;
 
         /* Cooked / cbreak processing */
         if (c == proto->c_intr) {
             /* ^C: signal — clear line buffer */
             proto->line_len = 0u;
             if (proto->echo)
                 uiox_uart_proto_puts(proto, "^C\r\n");
             continue;
         }
 
         if (c == proto->c_eof && proto->line_len == 0u) {
             /* ^D on empty line: EOF */
             proto->line_ready = true;
             continue;
         }
 
         if ((c == proto->c_erase || c == '\b') &&
             proto->line_len > 0u) {
             /* Backspace / DEL: remove last char */
             proto->line_len--;
             if (proto->echo)
                 uiox_uart_proto_puts(proto, "\b \b");
             continue;
         }
 
         if (c == proto->c_kill) {
             /* ^U: kill entire line */
             while (proto->line_len > 0u) {
                 proto->line_len--;
                 if (proto->echo)
                     uiox_uart_proto_puts(proto, "\b \b");
             }
             continue;
         }
 
         /* Normal character: add to line buffer */
         if (proto->line_len < UIOX_UART_LINE_BUF_SIZE - 1u) {
             proto->line_buf[proto->line_len++] = c;
             if (proto->echo)
                 uiox_uart_proto_putc(proto, c);
         }
 
         /* EOL: mark line ready */
         if (c == proto->c_eol || c == '\r') {
             proto->line_buf[proto->line_len] = '\0';
             if (proto->echo)
                 uiox_uart_proto_puts(proto, "\r\n");
             proto->line_ready = true;
             proto->lines_received++;
         }
     }
     return processed;
 }
 
 bool uiox_uart_proto_line_ready(const uiox_uart_proto_t *proto)
 { return proto && proto->line_ready; }
 
 int uiox_uart_proto_read_line(uiox_uart_proto_t *proto,
                                char *buf, uint32_t max)
 {
     if (!proto || !buf || !proto->line_ready) return 0;
     uint32_t n = (proto->line_len < max - 1u) ? proto->line_len : max - 1u;
     memcpy(buf, proto->line_buf, n);
     buf[n]              = '\0';
     proto->line_len     = 0u;
     proto->line_ready   = false;
     return (int)n;
 }
 
 void uiox_uart_proto_send_break(uiox_uart_proto_t *proto, uint32_t ms)
 { if (proto) uiox_uart_if_send_break(proto->uif, ms); }
 