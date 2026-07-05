/**
 * @file  uiox_uart_buf.h
 * @brief UIOX UART — TX/RX ring buffer pool.
 * @version 1.0.0
 * @date    2026-07-05
 */

 #ifndef UIOX_UART_BUF_H
 #define UIOX_UART_BUF_H
 
 #include "uiox_uart_hw.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * Ring buffer sizes
  * ====================================================================== */
 
 #define UIOX_UART_TX_BUF_SIZE   256u  /**< TX software ring buffer        */
 #define UIOX_UART_RX_BUF_SIZE   512u  /**< RX software ring buffer        */
 
 /* =========================================================================
  * Ring buffer descriptor
  * ====================================================================== */
 
 typedef struct {
     uint8_t  data[UIOX_UART_RX_BUF_SIZE]; /**< Backing store (max size)  */
     uint32_t head;    /**< Read index                                      */
     uint32_t tail;    /**< Write index                                     */
     uint32_t size;    /**< Total capacity (bytes)                          */
     uint32_t count;   /**< Bytes currently in buffer                       */
     uint32_t overruns;/**< Overrun count                                   */
 } uiox_uart_ring_t;
 
 /* =========================================================================
  * Event record
  * ====================================================================== */
 
 #define UIOX_UART_EVT_POOL_SIZE  16u
 
 typedef enum {
     UIOX_UART_EVT_NONE        = 0,
     UIOX_UART_EVT_RX_DATA,          /**< RX byte(s) available            */
     UIOX_UART_EVT_TX_DONE,          /**< TX buffer fully drained         */
     UIOX_UART_EVT_LINE_ERR,         /**< Framing/parity/overrun error    */
     UIOX_UART_EVT_BREAK_DETECT,     /**< Break condition detected        */
     UIOX_UART_EVT_MODEM_CHANGE,     /**< CTS/DSR/DCD/RI change          */
     UIOX_UART_EVT_OVERRUN,          /**< SW ring buffer overrun          */
     UIOX_UART_EVT_RX_TIMEOUT,       /**< RX FIFO idle timeout            */
     UIOX_UART_EVT_ERROR,
 } uiox_uart_evt_type_t;
 
 typedef struct {
     uiox_uart_evt_type_t type;
     uint32_t             timestamp_ms;
     uint32_t             error_flags;
     uint32_t             rx_count;   /**< Bytes in RX buffer at event time*/
     uint8_t              in_use;
 } uiox_uart_evt_t;
 
 /* =========================================================================
  * Pool API
  * ====================================================================== */
 
 void             uiox_uart_buf_init     (void);
 
 /* Ring buffer operations */
 void             uiox_uart_ring_init    (uiox_uart_ring_t *r,
                                           uint32_t size);
 int              uiox_uart_ring_put     (uiox_uart_ring_t *r, uint8_t c);
 int              uiox_uart_ring_get     (uiox_uart_ring_t *r, uint8_t *c);
 uint32_t         uiox_uart_ring_avail   (const uiox_uart_ring_t *r);
 uint32_t         uiox_uart_ring_free    (const uiox_uart_ring_t *r);
 bool             uiox_uart_ring_empty   (const uiox_uart_ring_t *r);
 bool             uiox_uart_ring_full    (const uiox_uart_ring_t *r);
 void             uiox_uart_ring_flush   (uiox_uart_ring_t *r);
 
 /* Event pool */
 uiox_uart_evt_t *uiox_uart_evt_alloc    (void);
 void             uiox_uart_evt_free     (uiox_uart_evt_t *e);
 uint8_t          uiox_uart_evt_free_cnt (void);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_UART_BUF_H */
 