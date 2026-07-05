/**
 * @file  uiox_uart_device.h
 * @brief UIOX UART application-facing API (Layer 5).
 * @version 1.0.0
 * @date    2026-07-05
 */

 #ifndef UIOX_UART_DEVICE_H
 #define UIOX_UART_DEVICE_H
 
 #include "uiox_uart_subsys.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 typedef struct {
     uiox_uart_hw_t           *hw;
     const uiox_uart_hw_ops_t *hw_ops;
     uiox_uart_cfg_t           cfg;
     bool                      is_console;
     uiox_uart_evt_cb_t        evt_cb;
     void                     *evt_ctx;
 } uiox_uart_open_params_t;
 
 typedef struct {
     uiox_uart_subsys_t  subsys;
     uiox_uart_hw_t     *hw;
     bool                open;
 } uiox_uart_device_t;
 
 /* Lifecycle */
 int  uiox_uart_open       (uiox_uart_device_t *dev,
                             const uiox_uart_open_params_t *p);
 int  uiox_uart_start      (uiox_uart_device_t *dev);
 void uiox_uart_stop       (uiox_uart_device_t *dev);
 void uiox_uart_close      (uiox_uart_device_t *dev);
 void uiox_uart_tick       (uiox_uart_device_t *dev, uint32_t now_ms);
 
 /* I/O */
 int  uiox_uart_putc       (uiox_uart_device_t *dev, char c);
 int  uiox_uart_puts       (uiox_uart_device_t *dev, const char *s);
 int  uiox_uart_write      (uiox_uart_device_t *dev,
                             const uint8_t *buf, uint32_t len);
 int  uiox_uart_getc       (uiox_uart_device_t *dev);
 int  uiox_uart_read       (uiox_uart_device_t *dev,
                             uint8_t *buf, uint32_t len);
 int  uiox_uart_printf     (uiox_uart_device_t *dev,
                             const char *fmt, ...);
 
 /* Configuration */
 int  uiox_uart_set_baud   (uiox_uart_device_t *dev, uint32_t baud);
 int  uiox_uart_set_format (uiox_uart_device_t *dev,
                             uiox_uart_bits_t bits,
                             uiox_uart_stop_t stop,
                             uiox_uart_parity_t parity);
 int  uiox_uart_set_flow   (uiox_uart_device_t *dev, uiox_uart_flow_t flow);
 int  uiox_uart_set_ldisc  (uiox_uart_device_t *dev, uiox_uart_ldisc_t mode);
 void uiox_uart_set_echo   (uiox_uart_device_t *dev, bool echo);
 void uiox_uart_send_break (uiox_uart_device_t *dev, uint32_t ms);
 
 /* Info / stats */
 void uiox_uart_print_info (const uiox_uart_device_t *dev);
 void uiox_uart_print_stats(uiox_uart_device_t *dev);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_UART_DEVICE_H */
 