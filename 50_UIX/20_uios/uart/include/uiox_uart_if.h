/**
 * @file  uiox_uart_if.h
 * @brief UIOX UART Interface driver — framing, parity, flow ctrl, IRQ.
 * @version 1.0.0
 * @date    2026-07-05
 */

 #ifndef UIOX_UART_IF_H
 #define UIOX_UART_IF_H
 
 #include "uiox_uart_hw.h"
 #include "uiox_uart_buf.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 typedef struct {
     uint64_t tx_bytes;
     uint64_t rx_bytes;
     uint32_t tx_irqs;
     uint32_t rx_irqs;
     uint32_t err_irqs;
     uint32_t overruns;
     uint32_t parity_errs;
     uint32_t framing_errs;
     uint32_t break_events;
 } uiox_uart_if_stats_t;
 
 typedef struct {
     uiox_uart_hw_t      *hw;
     uiox_uart_ring_t     tx_ring;
     uiox_uart_ring_t     rx_ring;
     uint8_t              tx_buf_mem[UIOX_UART_TX_BUF_SIZE];
     uint8_t              rx_buf_mem[UIOX_UART_RX_BUF_SIZE];
     uiox_uart_if_stats_t stats;
     bool                 primed;
     bool                 hw_flow_en;
     uint8_t              xon_char;   /**< XON character (def: 0x11)       */
     uint8_t              xoff_char;  /**< XOFF character (def: 0x13)      */
     bool                 xoff_sent;  /**< True when XOFF was sent to peer */
 } uiox_uart_if_t;
 
 int  uiox_uart_if_config      (uiox_uart_if_t *uif, uiox_uart_hw_t *hw);
 int  uiox_uart_if_start       (uiox_uart_if_t *uif);
 void uiox_uart_if_stop        (uiox_uart_if_t *uif);
 
 /* Transmit */
 int  uiox_uart_if_putc        (uiox_uart_if_t *uif, char c);
 int  uiox_uart_if_puts        (uiox_uart_if_t *uif, const char *s);
 int  uiox_uart_if_write       (uiox_uart_if_t *uif,
                                 const uint8_t *buf, uint32_t len);
 
 /* Receive */
 int  uiox_uart_if_getc        (uiox_uart_if_t *uif);
 int  uiox_uart_if_read        (uiox_uart_if_t *uif,
                                 uint8_t *buf, uint32_t len);
 uint32_t uiox_uart_if_rx_avail(const uiox_uart_if_t *uif);
 
 /* Configuration */
 int  uiox_uart_if_set_baud    (uiox_uart_if_t *uif, uint32_t baud);
 int  uiox_uart_if_set_format  (uiox_uart_if_t *uif,
                                 uiox_uart_bits_t bits,
                                 uiox_uart_stop_t stop,
                                 uiox_uart_parity_t parity);
 int  uiox_uart_if_set_flow    (uiox_uart_if_t *uif, uiox_uart_flow_t flow);
 void uiox_uart_if_flush       (uiox_uart_if_t *uif);
 
 /* Break */
 void uiox_uart_if_send_break  (uiox_uart_if_t *uif, uint32_t ms);
 
 /* IRQ handler — call from platform ISR */
 uiox_uart_evt_t *uiox_uart_if_irq_handle(uiox_uart_if_t *uif,
                                           uint32_t now_ms);
 
 void uiox_uart_if_stats_get   (const uiox_uart_if_t *uif,
                                 uiox_uart_if_stats_t *out);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_UART_IF_H */
 