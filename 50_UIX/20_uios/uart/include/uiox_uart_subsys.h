/**
 * @file  uiox_uart_subsys.h
 * @brief UIOX UART Subsystem — console, TTY, events, hotplug.
 * @version 1.0.0
 * @date    2026-07-05
 */

 #ifndef UIOX_UART_SUBSYS_H
 #define UIOX_UART_SUBSYS_H
 
 #include "uiox_uart_proto.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 typedef enum {
     UIOX_UART_EV_RX_DATA     = 0,
     UIOX_UART_EV_TX_DONE,
     UIOX_UART_EV_LINE_READY,
     UIOX_UART_EV_BREAK,
     UIOX_UART_EV_LINE_ERR,
     UIOX_UART_EV_OVERRUN,
     UIOX_UART_EV_MODEM,
     UIOX_UART_EV_ERROR,
 } uiox_uart_ev_t;
 
 typedef void (*uiox_uart_evt_cb_t)(uiox_uart_ev_t ev,
                                     uiox_uart_evt_t *data, void *ctx);
 
 typedef enum {
     UIOX_UART_STATE_OFF   = 0,
     UIOX_UART_STATE_INIT,
     UIOX_UART_STATE_READY,
     UIOX_UART_STATE_ERROR,
 } uiox_uart_state_t;
 
 typedef struct {
     uiox_uart_if_t       uif;
     uiox_uart_proto_t    proto;
     uiox_uart_state_t    state;
     uiox_uart_evt_cb_t   evt_cb;
     void                *evt_ctx;
     bool                 is_console;
     uint32_t             tick_count;
     uint64_t             uptime_ms;
     uint32_t             rx_event_count;
     uint32_t             error_count;
 } uiox_uart_subsys_t;
 
 int  uiox_uart_subsys_init    (uiox_uart_subsys_t *sys,
                                 uiox_uart_hw_t *hw,
                                 bool is_console);
 int  uiox_uart_subsys_start   (uiox_uart_subsys_t *sys);
 void uiox_uart_subsys_stop    (uiox_uart_subsys_t *sys);
 void uiox_uart_subsys_tick    (uiox_uart_subsys_t *sys, uint32_t now_ms);
 void uiox_uart_subsys_set_cb  (uiox_uart_subsys_t *sys,
                                 uiox_uart_evt_cb_t cb, void *ctx);
 
 int  uiox_uart_subsys_putc    (uiox_uart_subsys_t *sys, char c);
 int  uiox_uart_subsys_puts    (uiox_uart_subsys_t *sys, const char *s);
 int  uiox_uart_subsys_write   (uiox_uart_subsys_t *sys,
                                 const uint8_t *buf, uint32_t len);
 int  uiox_uart_subsys_getc    (uiox_uart_subsys_t *sys);
 int  uiox_uart_subsys_read    (uiox_uart_subsys_t *sys,
                                 uint8_t *buf, uint32_t len);
 int  uiox_uart_subsys_printf  (uiox_uart_subsys_t *sys,
                                 const char *fmt, ...);
 
 const char *uiox_uart_state_name  (uiox_uart_state_t s);
 const char *uiox_uart_ev_name     (uiox_uart_ev_t ev);
 const char *uiox_uart_variant_name(uiox_uart_variant_t v);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_UART_SUBSYS_H */
 