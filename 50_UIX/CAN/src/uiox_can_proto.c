/**
 * @file    uiox_can_proto.c
 * @brief   UIOX CAN protocol layer implementation.
 * @date    2026-05-26
 */

 #include "uiox_can_proto.h"
 #include <string.h>
 #include <errno.h>
 
 /* =========================================================================
  * NMT command codes
  * ====================================================================== */
 
 #define NMT_CMD_START       0x01u
 #define NMT_CMD_STOP        0x02u
 #define NMT_CMD_PRE_OP      0x80u
 #define NMT_CMD_RESET_NODE  0x81u
 #define NMT_CMD_RESET_COMM  0x82u
 
 /* =========================================================================
  * Init
  * ====================================================================== */
 
 int uiox_can_proto_init(uiox_can_proto_t         *proto,
                          uiox_can_node_t          *node,
                          const uiox_can_busoff_cfg_t *bo_cfg,
                          uint32_t                  sync_period_ms)
 {
     if (!proto || !node || !bo_cfg) return -EINVAL;
     memset(proto, 0, sizeof(*proto));
     proto->node           = node;
     proto->sync_period_ms = sync_period_ms;
     memcpy(&proto->busoff_cfg, bo_cfg, sizeof(*bo_cfg));
 
     /* Register SDO RX filter (RSDO COB-ID for this node) */
     uint32_t rsdo_cob = UIOX_CAN_COB_RSDO_BASE + node->node_id;
     uiox_can_if_add_filter(node->cif, rsdo_cob, 0x7FFu, false);
 
     /* Register NMT filter */
     uiox_can_if_add_filter(node->cif, UIOX_CAN_COB_NMT, 0x7FFu, false);
 
     /* Register SYNC filter */
     uiox_can_if_add_filter(node->cif, UIOX_CAN_COB_SYNC, 0x7FFu, false);
 
     return 0;
 }
 
 /* =========================================================================
  * NMT command
  * ====================================================================== */
 
 int uiox_can_proto_nmt_cmd(uiox_can_proto_t *proto,
                             uint8_t           node_id,
                             uint8_t           cmd)
 {
     if (!proto) return -EINVAL;
     uint8_t payload[2] = { cmd, node_id };
     return uiox_can_node_tx(proto->node,
                              UIOX_CAN_COB_NMT,
                              payload, 2, false);
 }
 
 /* =========================================================================
  * SDO expedited write
  * Frame layout (8 bytes):
  *   [0]  cs | size_indicator | expedited | transfer_type
  *   [1]  index lo
  *   [2]  index hi
  *   [3]  subindex
  *   [4..7] data (little-endian, padded)
  * ====================================================================== */
 
 int uiox_can_proto_sdo_write(uiox_can_proto_t *proto,
                               uint8_t  node_id,
                               uint16_t index,
                               uint8_t  subindex,
                               const uint8_t *data,
                               uint8_t  len)
 {
     if (!proto || !data || len > 4) return -EINVAL;
 
     uint8_t frame[8] = {0};
     uint8_t size_ind  = (uint8_t)(4u - len);
     frame[0] = (uint8_t)(UIOX_SDO_CS_WR_EXP | (size_ind << 2) | 0x03u);
     frame[1] = (uint8_t)(index & 0xFFu);
     frame[2] = (uint8_t)(index >> 8u);
     frame[3] = subindex;
     memcpy(&frame[4], data, len);
 
     uint32_t cob = UIOX_CAN_COB_RSDO_BASE + node_id;
     return uiox_can_node_tx(proto->node, cob, frame, 8u, false);
 }
 
 /* =========================================================================
  * SDO expedited read (upload request — sends request, reply via RX path)
  * ====================================================================== */
 
 int uiox_can_proto_sdo_read(uiox_can_proto_t *proto,
                              uint8_t  node_id,
                              uint16_t index,
                              uint8_t  subindex,
                              uint8_t *data_out,
                              uint8_t *len_out)
 {
     if (!proto || !data_out || !len_out) return -EINVAL;
 
     /* Send upload request */
     uint8_t frame[8] = {0};
     frame[0] = UIOX_SDO_CS_RD_REQ;
     frame[1] = (uint8_t)(index & 0xFFu);
     frame[2] = (uint8_t)(index >> 8u);
     frame[3] = subindex;
 
     uint32_t cob = UIOX_CAN_COB_RSDO_BASE + node_id;
     int rc = uiox_can_node_tx(proto->node, cob, frame, 8u, false);
     if (rc < 0) return rc;
 
     /* Poll for TSDO response (blocking spin — replace with semaphore) */
     uint32_t tsdo_cob = UIOX_CAN_COB_TSDO_BASE + node_id;
     uint32_t waited   = 0;
     while (waited++ < 100u) {
         uiox_can_msg_t *msg = uiox_can_if_rx(proto->node->cif);
         if (!msg) continue;
         uint32_t rx_id = msg->id & UIOX_CAN_ID_EXT_MASK;
         if (rx_id == tsdo_cob) {
             uint8_t n = (msg->data[0] >> 2u) & 0x03u;
             *len_out  = (uint8_t)(4u - n);
             memcpy(data_out, &msg->data[4], *len_out);
             uiox_can_buf_free(msg);
             return 0;
         }
         uiox_can_node_dispatch(proto->node, msg);
     }
     return -ETIMEDOUT;
 }
 
 /* =========================================================================
  * EMCY
  * ====================================================================== */
 
 int uiox_can_proto_emcy(uiox_can_proto_t *proto,
                          uint16_t err_code,
                          uint8_t  err_reg,
                          const uint8_t *mspec,
                          uint8_t  mspec_len)
 {
     if (!proto) return -EINVAL;
     uint8_t frame[8] = {0};
     frame[0] = (uint8_t)(err_code & 0xFFu);
     frame[1] = (uint8_t)(err_code >> 8u);
     frame[2] = err_reg;
     if (mspec && mspec_len > 0) {
         uint8_t cp = mspec_len < 5u ? mspec_len : 5u;
         memcpy(&frame[3], mspec, cp);
     }
     uint32_t cob = UIOX_CAN_COB_EMCY_BASE + proto->node->node_id;
     return uiox_can_node_tx(proto->node, cob, frame, 8u, false);
 }
 
 /* =========================================================================
  * SYNC
  * ====================================================================== */
 
 int uiox_can_proto_sync(uiox_can_proto_t *proto)
 {
     if (!proto) return -EINVAL;
     return uiox_can_node_tx(proto->node,
                              UIOX_CAN_COB_SYNC, NULL, 0, false);
 }
 
 /* =========================================================================
  * PDO TX
  * ====================================================================== */
 
 int uiox_can_proto_pdo_tx(uiox_can_proto_t     *proto,
                            const uiox_can_pdo_t *pdo,
                            const uint8_t        *data,
                            uint8_t               len)
 {
     if (!proto || !pdo || !data) return -EINVAL;
     return uiox_can_node_tx(proto->node,
                              pdo->cob_id, data, len, pdo->ext);
 }
 
 /* =========================================================================
  * Incoming message dispatcher
  * ====================================================================== */
 
 void uiox_can_proto_rx(uiox_can_proto_t *proto,
                         uiox_can_msg_t   *msg)
 {
     if (!proto || !msg) return;
     uint32_t id = msg->id & UIOX_CAN_ID_EXT_MASK;
 
     /* NMT command */
     if (id == UIOX_CAN_COB_NMT && msg->dlc >= 2u) {
         uint8_t cmd     = msg->data[0];
         uint8_t dest_id = msg->data[1];
         if (dest_id == 0u || dest_id == proto->node->node_id) {
             uiox_can_nmt_state_t new_state = proto->node->nmt_state;
             switch (cmd) {
             case NMT_CMD_START:      new_state = UIOX_CAN_NMT_OPERATIONAL; break;
             case NMT_CMD_STOP:       new_state = UIOX_CAN_NMT_STOPPED;     break;
             case NMT_CMD_PRE_OP:     new_state = UIOX_CAN_NMT_PRE_OP;      break;
             case NMT_CMD_RESET_NODE: new_state = UIOX_CAN_NMT_INIT;        break;
             default: break;
             }
             uiox_can_node_nmt(proto->node, new_state);
         }
         uiox_can_buf_free(msg);
         return;
     }
 
     /* SYNC */
     if (id == UIOX_CAN_COB_SYNC) {
         /* Application can register SYNC callback — stub here */
         uiox_can_buf_free(msg);
         return;
     }
 
     /* Dispatch to node mailboxes */
     uiox_can_node_dispatch(proto->node, msg);
 }
 
 /* =========================================================================
  * Periodic tick
  * ====================================================================== */
 
 void uiox_can_proto_tick(uiox_can_proto_t *proto, uint32_t now_ms)
 {
     if (!proto) return;
 
     /* Bus-off recovery */
     if (proto->busoff_pending) {
         uint32_t elapsed = now_ms - proto->busoff_ts_ms;
         if (elapsed >= proto->busoff_cfg.retry_delay_ms) {
             uint8_t max = proto->busoff_cfg.max_retries;
             if (max == 0u || proto->busoff_retries < max) {
                 uiox_can_hw_recover(proto->node->cif->hw);
                 proto->busoff_retries++;
                 proto->busoff_ts_ms   = now_ms;
                 if (proto->node->cif->hw->err_state != UIOX_CAN_ERR_BUS_OFF)
                     proto->busoff_pending = false;
             }
         }
     }
 
     /* SYNC transmission */
     if (proto->sync_period_ms &&
         (now_ms - proto->last_sync_ms) >= proto->sync_period_ms) {
         uiox_can_proto_sync(proto);
         proto->last_sync_ms = now_ms;
     }
 
     /* Node tick (heartbeat + periodic PDOs) */
     uiox_can_node_tick(proto->node, now_ms);
 }
 