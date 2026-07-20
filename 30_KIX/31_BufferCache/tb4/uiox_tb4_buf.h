/**
 * @file    uiox_tb4_buf.h
 * @brief   UIOX Thunderbolt 4 TX/RX frame buffer pool.
 * @date    2026-06-08
 */

 #ifndef UIOX_TB4_BUF_H
 #define UIOX_TB4_BUF_H
 
 #include "uiox_tb4_hw.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 #define UIOX_TB4_TX_POOL_SIZE    32
 #define UIOX_TB4_RX_POOL_SIZE    64
 #define UIOX_TB4_FRAME_MAX       4096u   /**< Max TB4 frame bytes          */
 #define UIOX_TB4_BUF_ALIGN       64
 
 /* =========================================================================
  * Frame buffer
  * ====================================================================== */
 
 typedef enum {
     UIOX_TB4_BUF_FREE = 0,
     UIOX_TB4_BUF_TX_PENDING,
     UIOX_TB4_BUF_RX_READY,
 } uiox_tb4_buf_state_t;
 
 typedef struct uiox_tb4_frame {
     uint8_t    *data;
     uintptr_t   paddr;
     uint32_t    capacity;
     uint32_t    len;
     uiox_tb4_buf_state_t state;
     uint8_t     in_use;
     struct uiox_tb4_frame *next;
 } uiox_tb4_frame_t;
 
 /* =========================================================================
  * Pool API
  * ====================================================================== */
 
 void              uiox_tb4_buf_init    (void);
 uiox_tb4_frame_t *uiox_tb4_buf_alloc_tx(void);
 uiox_tb4_frame_t *uiox_tb4_buf_alloc_rx(void);
 void              uiox_tb4_buf_free    (uiox_tb4_frame_t *f);
 uint8_t           uiox_tb4_buf_tx_free (void);
 uint8_t           uiox_tb4_buf_rx_free (void);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_TB4_BUF_H */
 