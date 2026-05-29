/**
 * @file    uiox_can_buf.h
 * @brief   UIOX CAN message buffer pool (TX and RX).
 *
 * Provides statically allocated message buffers supporting both
 * Classic CAN (8 bytes) and CAN-FD (up to 64 bytes) payloads.
 * Two pools: TX and RX, each with configurable depth.
 *
 * @date    2026-05-26
 */
//Layer 1.5 — Buffer Manager
 #ifndef UIOX_CAN_BUF_H
 #define UIOX_CAN_BUF_H
 
 #include "uiox_can_hw.h"
 #include <stdint.h>
 #include <stdbool.h>
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * Pool configuration
  * ====================================================================== */
 
 #define UIOX_CAN_TX_POOL_SIZE    32   /**< TX buffer pool depth             */
 #define UIOX_CAN_RX_POOL_SIZE    64   /**< RX buffer pool depth             */
 #define UIOX_CAN_BUF_ALIGN       16
 
 /* =========================================================================
  * CAN message (TX and RX unified)
  * ====================================================================== */
 
 typedef struct uiox_can_msg {
     uint32_t  id;               /**< CAN ID with flags (EXT/RTR/ERR/BRS)   */
     uint8_t   dlc;              /**< Data length code (0..15 for FD)       */
     uint8_t   data[UIOX_CANFD_MAX_DLC]; /**< Payload (up to 64 bytes)      */
     uint64_t  ts_ns;            /**< Hardware timestamp (ns)               */
     uint8_t   channel;          /**< CAN channel / bus index               */
     uint8_t   in_use;           /**< Reference count                       */
     struct uiox_can_msg *next;  /**< Free-list linkage                     */
 } uiox_can_msg_t;
 
 /* DLC to actual byte count (CAN-FD) */
 static inline uint8_t uiox_can_dlc2len(uint8_t dlc)
 {
     static const uint8_t dlc2len[] =
         { 0,1,2,3,4,5,6,7,8,12,16,20,24,32,48,64 };
     return dlc < 16u ? dlc2len[dlc] : 64u;
 }
 
 /* Byte count to DLC (CAN-FD) */
 static inline uint8_t uiox_can_len2dlc(uint8_t len)
 {
     if (len <= 8u)  return len;
     if (len <= 12u) return 9u;
     if (len <= 16u) return 10u;
     if (len <= 20u) return 11u;
     if (len <= 24u) return 12u;
     if (len <= 32u) return 13u;
     if (len <= 48u) return 14u;
     return 15u;
 }
 
 /* =========================================================================
  * Pool API
  * ====================================================================== */
 
 void           uiox_can_buf_init    (void);
 uiox_can_msg_t *uiox_can_buf_alloc_tx(void);
 uiox_can_msg_t *uiox_can_buf_alloc_rx(void);
 void           uiox_can_buf_ref     (uiox_can_msg_t *msg);
 void           uiox_can_buf_free    (uiox_can_msg_t *msg);
 uint16_t       uiox_can_buf_tx_free (void);
 uint16_t       uiox_can_buf_rx_free (void);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_CAN_BUF_H */
 