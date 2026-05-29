/**
 * @file    uiox_can_node.c
 * @brief   UIOX CAN node abstraction implementation.
 * @date    2026-05-26
 */

 #include "uiox_can_node.h"
 #include <string.h>
 #include <errno.h>
 
 int uiox_can_node_init(uiox_can_node_t *node,
                         uiox_can_if_t   *cif,
                         uint8_t          node_id,
                         const char      *name,
                         uint32_t         heartbeat_ms)
 {
     if (!node || !cif || node_id > UIOX_CAN_NODE_ID_MAX)
         return -EINVAL;
 
     memset(node, 0, sizeof(*node));
     node->node_id      = node_id;
     node->name         = name;
     node->cif          = cif;
     node->heartbeat_ms = heartbeat_ms;
     node->nmt_state    = UIOX_CAN_NMT_INIT;
     return 0;
 }
 
 int uiox_can_node_add_mb(uiox_can_node_t          *node,
                           const uiox_can_mailbox_t *mb)
 {
     if (!node || !mb) return -EINVAL;
     if (node->mb_count >= UIOX_CAN_MAX_MAILBOXES) return -ENOSPC;
 
     memcpy(&node->mailboxes[node->mb_count++], mb, sizeof(*mb));
 
     /* Register RX filter for this mailbox */
     if (mb->dir == UIOX_CAN_MB_RX)
         uiox_can_if_add_filter(node->cif, mb->cob_id, mb->mask, mb->ext);
 
     return 0;
 }
 
 int uiox_can_node_tx(uiox_can_node_t *node,
                       uint32_t         cob_id,
                       const uint8_t   *data,
                       uint8_t          len,
                       bool             ext)
 {
     if (!node || !data || len > UIOX_CANFD_MAX_DLC) return -EINVAL;
     if (node->nmt_state == UIOX_CAN_NMT_STOPPED)   return -ENETDOWN;
 
     uiox_can_msg_t *msg = uiox_can_buf_alloc_tx();
     if (!msg) return -ENOMEM;
 
     msg->id  = cob_id;
     if (ext) msg->id |= UIOX_CAN_ID_EXT_FLAG;
     msg->dlc = uiox_can_len2dlc(len);
     memcpy(msg->data, data, len);
 
     int rc = uiox_can_if_tx(node->cif, msg);
     if (rc < 0) uiox_can_buf_free(msg);
     return rc;
 }
 
 void uiox_can_node_dispatch(uiox_can_node_t *node, uiox_can_msg_t *msg)
 {
     if (!node || !msg) return;
     uint32_t id = msg->id & UIOX_CAN_ID_EXT_MASK;
 
     for (uint8_t i = 0; i < node->mb_count; i++) {
         uiox_can_mailbox_t *mb = &node->mailboxes[i];
         if (!mb->enabled || mb->dir != UIOX_CAN_MB_RX) continue;
         if ((id & mb->mask) == (mb->cob_id & mb->mask)) {
             if (mb->rx_cb)
                 mb->rx_cb(id, msg->data,
                           uiox_can_dlc2len(msg->dlc), mb->cb_ctx);
         }
     }
     uiox_can_buf_free(msg);
 }
 
 void uiox_can_node_tick(uiox_can_node_t *node, uint32_t now_ms)
 {
     if (!node) return;
     if (node->nmt_state != UIOX_CAN_NMT_OPERATIONAL) return;
 
     /* Heartbeat */
     if (node->heartbeat_ms &&
         (now_ms - node->last_hb_ms) >= node->heartbeat_ms) {
         uint8_t hb = (uint8_t)node->nmt_state;
         uiox_can_node_tx(node,
                           UIOX_CAN_HEARTBEAT_COB + node->node_id,
                           &hb, 1, false);
         node->last_hb_ms = now_ms;
     }
 
     /* Periodic TX mailboxes */
     for (uint8_t i = 0; i < node->mb_count; i++) {
         uiox_can_mailbox_t *mb = &node->mailboxes[i];
         if (!mb->enabled || mb->dir != UIOX_CAN_MB_TX) continue;
         if (!mb->period_ms) continue;
         if ((now_ms - mb->last_tx_ms) >= mb->period_ms) {
             /* Trigger periodic TX — data filled by application layer */
             mb->last_tx_ms = now_ms;
         }
     }
 }
 
 int uiox_can_node_nmt(uiox_can_node_t      *node,
                        uiox_can_nmt_state_t  state)
 {
     if (!node) return -EINVAL;
     node->nmt_state = state;
 
     /* Broadcast NMT state via heartbeat immediately */
     uint8_t hb = (uint8_t)state;
     return uiox_can_node_tx(node,
                              UIOX_CAN_HEARTBEAT_COB + node->node_id,
                              &hb, 1, false);
 }
 