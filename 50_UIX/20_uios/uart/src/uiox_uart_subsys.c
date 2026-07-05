/**
 * @file  uiox_uart_subsys.c
 * @brief UIOX UART Subsystem — console, events, tick.
 * @date  2026-07-05
 */

 #include "uiox_uart_device.h"
 #include <string.h>
 #include <stdarg.h>
 #include <stdio.h>
 #include <errno.h>
 
 static void fire(uiox_uart_subsys_t *sys, uiox_uart_ev_t ev,
                  uiox_uart_evt_t *data)
 { if (sys->evt_cb) sys->evt_cb(ev, data, sys->evt_ctx); }
 
 static uiox_uart_ev_t evt_type_to_ev(uiox_uart_evt_type_t t)
 {
     switch (t) {
     case UIOX_UART_EVT_RX_DATA:      return UIOX_UART_EV_RX_DATA;
     case UIOX_UART_EVT_TX_DONE:      return UIOX_UART_EV_TX_DONE;
     case UIOX_UART_EVT_LINE_ERR:     return UIOX_UART_EV_LINE_ERR;
     case UIOX_UART_EVT_BREAK_DETECT: return UIOX_UART_EV_BREAK;
     case UIOX_UART_EVT_MODEM_CHANGE: return UIOX_UART_EV_MODEM;
     case UIOX_UART_EVT_OVERRUN:      return UIOX_UART_EV_OVERRUN;
     case UIOX_UART_EVT_RX_TIMEOUT:   return UIOX_UART_EV_RX_DATA;
     default:                          return UIOX_UART_EV_ERROR;
     }
 }
 
 int uiox_uart_subsys_init(uiox_uart_subsys_t *sys,
                            uiox_uart_hw_t *hw,
                            bool is_console)
 {
     if (!sys || !hw) return -EINVAL;
     memset(sys, 0, sizeof(*sys));
     sys->is_console = is_console;
     int rc = uiox_uart_if_config(&sys->uif, hw);
     if (rc < 0) return rc;
     return uiox_uart_proto_init(&sys->proto, &sys->uif);
 }
 
 int uiox_uart_subsys_start(uiox_uart_subsys_t *sys)
 {
     if (!sys) return -EINVAL;
     sys->state = UIOX_UART_STATE_INIT;
     int rc = uiox_uart_if_start(&sys->uif);
     if (rc < 0) { sys->state = UIOX_UART_STATE_ERROR; return rc; }
     sys->state = UIOX_UART_STATE_READY;
     return 0;
 }
 
 void uiox_uart_subsys_stop(uiox_uart_subsys_t *sys)
 {
     if (!sys) return;
     uiox_uart_if_stop(&sys->uif);
     sys->state = UIOX_UART_STATE_OFF;
 }
 
 void uiox_uart_subsys_tick(uiox_uart_subsys_t *sys, uint32_t now_ms)
 {
     if (!sys || sys->state != UIOX_UART_STATE_READY) return;
     sys->tick_count++;
     sys->uptime_ms += 10u;
 
     /* Process any pending hardware IRQ events */
     uiox_uart_evt_t *e = uiox_uart_if_irq_handle(&sys->uif, now_ms);
     if (e) {
         if (e->type == UIOX_UART_EV_ERROR ||
             e->type == UIOX_UART_EVT_LINE_ERR) {
             sys->error_count++;
             fire(sys, UIOX_UART_EV_LINE_ERR, e);
         } else if (e->type == UIOX_UART_EVT_RX_DATA ||
                    e->type == UIOX_UART_EVT_RX_TIMEOUT) {
             sys->rx_event_count++;
             /* Feed new bytes through line discipline */
             uiox_uart_proto_process_rx(&sys->proto);
             if (uiox_uart_proto_line_ready(&sys->proto))
                 fire(sys, UIOX_UART_EV_LINE_READY, e);
             else
                 fire(sys, UIOX_UART_EV_RX_DATA, e);
         } else {
             fire(sys, evt_type_to_ev(e->type), e);
         }
         uiox_uart_evt_free(e);
     }
 }
 
 void uiox_uart_subsys_set_cb(uiox_uart_subsys_t *sys,
                                uiox_uart_evt_cb_t cb, void *ctx)
 { if (sys) { sys->evt_cb = cb; sys->evt_ctx = ctx; } }
 
 /* ── I/O wrappers ──────────────────────────────────────────────── */
 
 int uiox_uart_subsys_putc(uiox_uart_subsys_t *sys, char c)
 { if (!sys || sys->state != UIOX_UART_STATE_READY) return -EINVAL;
   return uiox_uart_proto_putc(&sys->proto, c); }
 
 int uiox_uart_subsys_puts(uiox_uart_subsys_t *sys, const char *s)
 { if (!sys || sys->state != UIOX_UART_STATE_READY) return -EINVAL;
   return uiox_uart_proto_puts(&sys->proto, s); }
 
 int uiox_uart_subsys_write(uiox_uart_subsys_t *sys,
                              const uint8_t *buf, uint32_t len)
 { if (!sys || sys->state != UIOX_UART_STATE_READY) return -EINVAL;
   return uiox_uart_if_write(&sys->uif, buf, len); }
 
 int uiox_uart_subsys_getc(uiox_uart_subsys_t *sys)
 { if (!sys || sys->state != UIOX_UART_STATE_READY) return -1;
   return uiox_uart_if_getc(&sys->uif); }
 
 int uiox_uart_subsys_read(uiox_uart_subsys_t *sys,
                            uint8_t *buf, uint32_t len)
 { if (!sys || sys->state != UIOX_UART_STATE_READY) return -EINVAL;
   return uiox_uart_if_read(&sys->uif, buf, len); }
 
 int uiox_uart_subsys_printf(uiox_uart_subsys_t *sys,
                               const char *fmt, ...)
 {
     if (!sys || sys->state != UIOX_UART_STATE_READY) return -EINVAL;
     char buf[512];
     va_list ap;
     va_start(ap, fmt);
     int n = vsnprintf(buf, sizeof(buf), fmt, ap);
     va_end(ap);
     uiox_uart_proto_puts(&sys->proto, buf);
     return n;
 }
 
 /* ── Name helpers ──────────────────────────────────────────────── */
 
 const char *uiox_uart_state_name(uiox_uart_state_t s)
 {
     switch (s) {
     case UIOX_UART_STATE_OFF:   return "OFF";
     case UIOX_UART_STATE_INIT:  return "INIT";
     case UIOX_UART_STATE_READY: return "READY";
     case UIOX_UART_STATE_ERROR: return "ERROR";
     default:                     return "?";
     }
 }
 
 const char *uiox_uart_ev_name(uiox_uart_ev_t ev)
 {
     switch (ev) {
     case UIOX_UART_EV_RX_DATA:    return "RX_DATA";
     case UIOX_UART_EV_TX_DONE:    return "TX_DONE";
     case UIOX_UART_EV_LINE_READY: return "LINE_READY";
     case UIOX_UART_EV_BREAK:      return "BREAK";
     case UIOX_UART_EV_LINE_ERR:   return "LINE_ERR";
     case UIOX_UART_EV_OVERRUN:    return "OVERRUN";
     case UIOX_UART_EV_MODEM:      return "MODEM";
     case UIOX_UART_EV_ERROR:      return "ERROR";
     default:                       return "?";
     }
 }
 
 const char *uiox_uart_variant_name(uiox_uart_variant_t v)
 {
     switch (v) {
     case UIOX_UART_PL011:  return "PL011";
     case UIOX_UART_16550:  return "NS16550";
     case UIOX_UART_SIFIVE: return "SiFive";
     default:                return "?";
     }
 }
 