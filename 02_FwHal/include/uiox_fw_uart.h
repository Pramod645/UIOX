/**
 * @file  uiox_fw_uart.h
 * @brief UIOX Firmware — UART driver (PL011 for ARM, 16550 for x86).
 * @version 1.0.0
 * @date    2026-06-21
 */

 #ifndef UIOX_FW_UART_H
 #define UIOX_FW_UART_H
 
 #include "uiox_fw_hw.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* PL011 register offsets (ARM) */
 #define PL011_DR         0x000u
 #define PL011_RSR        0x004u
 #define PL011_FR         0x018u
 #define PL011_FR_TXFF    UIOX_FW_BIT(5)
 #define PL011_FR_RXFE    UIOX_FW_BIT(4)
 #define PL011_FR_BUSY    UIOX_FW_BIT(3)
 #define PL011_IBRD       0x024u
 #define PL011_FBRD       0x028u
 #define PL011_LCR_H      0x02Cu
 #define PL011_LCR_WLEN8  (0x3u << 5)
 #define PL011_LCR_FEN    UIOX_FW_BIT(4)
 #define PL011_CR         0x030u
 #define PL011_CR_UARTEN  UIOX_FW_BIT(0)
 #define PL011_CR_TXE     UIOX_FW_BIT(8)
 #define PL011_CR_RXE     UIOX_FW_BIT(9)
 #define PL011_IMSC       0x038u
 #define PL011_MIS        0x040u
 #define PL011_ICR        0x044u
 
 /* 16550 register offsets (x86 I/O port relative) */
 #define UART16550_THR    0u
 #define UART16550_RBR    0u
 #define UART16550_IER    1u
 #define UART16550_FCR    2u
 #define UART16550_LCR    3u
 #define UART16550_MCR    4u
 #define UART16550_LSR    5u
 #define UART16550_MSR    6u
 #define UART16550_DLL    0u
 #define UART16550_DLM    1u
 #define UART16550_LSR_DR    UIOX_FW_BIT(0)
 #define UART16550_LSR_THRE  UIOX_FW_BIT(5)
 
 /* UART parity */
 typedef enum {
     UIOX_FW_UART_PARITY_NONE = 0,
     UIOX_FW_UART_PARITY_ODD,
     UIOX_FW_UART_PARITY_EVEN,
 } uiox_fw_uart_parity_t;
 
 /* UART configuration */
 typedef struct {
     uint32_t              baud;
     uint8_t               data_bits;  /**< 5–8                            */
     uint8_t               stop_bits;  /**< 1–2                            */
     uiox_fw_uart_parity_t parity;
     bool                  flow_ctrl;  /**< HW RTS/CTS                     */
     bool                  fifo_en;
 } uiox_fw_uart_cfg_t;
 
 #define UIOX_FW_UART_CFG_DEFAULT \
     { .baud=115200, .data_bits=8, .stop_bits=1, \
       .parity=UIOX_FW_UART_PARITY_NONE, \
       .flow_ctrl=false, .fifo_en=true }
 
 /* RX callback */
 typedef void (*uiox_fw_uart_rx_cb_t)(char c, void *priv);
 
 /* UART device */
 typedef struct {
     uintptr_t           base;
     uint32_t            clock_hz;    /**< Input clock (default 24 MHz)   */
     uint32_t            irq;
     uiox_fw_uart_cfg_t  cfg;
     uiox_fw_uart_rx_cb_t rx_cb;
     void               *rx_priv;
     bool                is_pl011;    /**< true=PL011, false=16550        */
     /* Stats */
     uint64_t            tx_bytes;
     uint64_t            rx_bytes;
     uint32_t            errors;
 } uiox_fw_uart_t;
 
 /* API */
 uiox_fw_err_t uiox_fw_uart_init   (uiox_fw_uart_t *uart,
                                      uintptr_t base, bool is_pl011,
                                      uint32_t irq,
                                      const uiox_fw_uart_cfg_t *cfg);
 void          uiox_fw_uart_putc   (uiox_fw_uart_t *uart, char c);
 void          uiox_fw_uart_puts   (uiox_fw_uart_t *uart, const char *s);
 int           uiox_fw_uart_getc   (uiox_fw_uart_t *uart);
 bool          uiox_fw_uart_rx_rdy (uiox_fw_uart_t *uart);
 void          uiox_fw_uart_irq    (uiox_fw_uart_t *uart);  /* ISR entry */
 void          uiox_fw_uart_set_rx_cb(uiox_fw_uart_t *uart,
                                       uiox_fw_uart_rx_cb_t cb, void *priv);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_FW_UART_H */
 