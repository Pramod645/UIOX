/**
 * @file    uiox_bt_buf.h
 * @brief   UIOX Bluetooth HCI packet buffer pool.
 * @date    2026-06-09
 */

 #ifndef UIOX_BT_BUF_H
 #define UIOX_BT_BUF_H
 
 #include "uiox_bt_hw.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 #define UIOX_BT_CMD_POOL_SIZE   16
 #define UIOX_BT_EVT_POOL_SIZE   32
 #define UIOX_BT_ACL_POOL_SIZE   32
 #define UIOX_BT_PKT_MAX_LEN     256
 #define UIOX_BT_EVT_RING_SIZE   64
 #define UIOX_BT_EVT_RING_MASK   (UIOX_BT_EVT_RING_SIZE - 1)
 
 /* =========================================================================
  * HCI packet buffer
  * ====================================================================== */
 
 typedef struct uiox_bt_pkt {
     uint8_t   data[UIOX_BT_PKT_MAX_LEN];
     uint16_t  len;
     uint8_t   pkt_type;    /**< HCI_*_PKT                                 */
     uint8_t   in_use;
     struct uiox_bt_pkt *next;
 } uiox_bt_pkt_t;
 
 /* =========================================================================
  * HCI event ring buffer (for async events)
  * ====================================================================== */
 
 typedef struct {
     uiox_bt_pkt_t    buf[UIOX_BT_EVT_RING_SIZE];
     volatile uint32_t head;
     volatile uint32_t tail;
     uint32_t          overflow;
 } uiox_bt_evt_ring_t;
 
 /* =========================================================================
  * Pool API
  * ====================================================================== */
 
 void           uiox_bt_buf_init     (void);
 uiox_bt_pkt_t *uiox_bt_cmd_alloc   (void);
 uiox_bt_pkt_t *uiox_bt_acl_alloc   (void);
 void           uiox_bt_pkt_free    (uiox_bt_pkt_t *p);
 
 void  uiox_bt_evt_ring_init        (uiox_bt_evt_ring_t *r);
 bool  uiox_bt_evt_push             (uiox_bt_evt_ring_t *r,
                                      const uint8_t *data, uint16_t len,
                                      uint8_t pkt_type);
 bool  uiox_bt_evt_pop              (uiox_bt_evt_ring_t *r,
                                      uiox_bt_pkt_t *out);
 bool  uiox_bt_evt_empty            (const uiox_bt_evt_ring_t *r);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_BT_BUF_H */
 