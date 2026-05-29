/**
 * @file    uiox_can_if.c
 * @brief   UIOX CAN interface driver implementation.
 * @date    2026-05-26
 */

 #include "uiox_can_if.h"
 #include <string.h>
 #include <errno.h>
 
 /* =========================================================================
  * Bit-rate to bittiming helper
  * Assumes 80% sample point, controller clock from hw->clk_hz
  * ====================================================================== */
 
 static int bitrate_to_timing(uint32_t clk_hz, uint32_t bitrate,
                               uiox_can_bittiming_t *bt)
 {
     if (!bitrate) return -EINVAL;
 
     /* Try prescalers 1..512 to find best fit */
     for (uint32_t brp = 1; brp <= 512; brp++) {
         uint32_t tq_total = clk_hz / (brp * bitrate);
         if (tq_total < 4 || tq_total > 25) continue;
 
         /* 80% sample point: tseg1 = 0.8×total - 1, tseg2 = rest */
         uint32_t tseg1 = (tq_total * 8u / 10u) - 1u;
         uint32_t tseg2 = tq_total - 1u - tseg1;
         if (tseg1 < 1 || tseg1 > 16) continue;
         if (tseg2 < 1 || tseg2 > 8)  continue;
 
         bt->brp   = (uint32_t)(brp - 1u);
         bt->tseg1 = (uint8_t)tseg1;
         bt->tseg2 = (uint8_t)tseg2;
         bt->sjw   = (bt->tseg2 < 4u) ? (uint8_t)bt->tseg2 : 4u;
         return 0;
     }
     return -EINVAL;
 }
 
 /* =========================================================================
  * TX software queue helpers
  * ====================================================================== */
 
 static int txq_push(uiox_can_if_t *cif, uiox_can_msg_t *msg)
 {
     if (cif->tx_q_count >= UIOX_CAN_IF_TX_DEPTH) return -ENOSPC;
     cif->tx_queue[cif->tx_q_tail % UIOX_CAN_IF_TX_DEPTH] = msg;
     cif->tx_q_tail++;
     cif->tx_q_count++;
     return 0;
 }
 
 static uiox_can_msg_t *txq_pop(uiox_can_if_t *cif)
 {
     if (!cif->tx_q_count) return NULL;
     uiox_can_msg_t *m =
         cif->tx_queue[cif->tx_q_head % UIOX_CAN_IF_TX_DEPTH];
     cif->tx_q_head++;
     cif->tx_q_count--;
     return m;
 }
 
 /* =========================================================================
  * RX software queue helpers
  * ====================================================================== */
 
 static int rxq_push(uiox_can_if_t *cif, uiox_can_msg_t *msg)
 {
     if (cif->rx_q_count >= UIOX_CAN_IF_RX_DEPTH) {
         cif->stats.rx_overflows++;
         return -ENOSPC;
     }
     cif->rx_queue[cif->rx_q_tail % UIOX_CAN_IF_RX_DEPTH] = msg;
     cif->rx_q_tail++;
     cif->rx_q_count++;
     return 0;
 }
 
 static uiox_can_msg_t *rxq_pop(uiox_can_if_t *cif)
 {
     if (!cif->rx_q_count) return NULL;
     uiox_can_msg_t *m =
         cif->rx_queue[cif->rx_q_head % UIOX_CAN_IF_RX_DEPTH];
     cif->rx_q_head++;
     cif->rx_q_count--;
     return m;
 }
 
 /* =========================================================================
  * Public API
  * ====================================================================== */
 
 int uiox_can_if_config(uiox_can_if_t *cif,
                         uiox_can_hw_t *hw,
                         uint8_t        channel,
                         bool           fd_enabled,
                         uint32_t       nom_bitrate,
                         uint32_t       data_bitrate)
 {
     if (!cif || !hw) return -EINVAL;
     memset(cif, 0, sizeof(*cif));
 
     cif->hw         = hw;
     cif->channel    = channel;
     cif->fd_enabled = fd_enabled;
 
     /* Compute bit timing */
     uiox_can_bittiming_t nom_bt, data_bt;
     int rc = bitrate_to_timing(hw->clk_hz, nom_bitrate, &nom_bt);
     if (rc < 0) return rc;
 
     if (fd_enabled) {
         rc = bitrate_to_timing(hw->clk_hz, data_bitrate, &data_bt);
         if (rc < 0) return rc;
     } else {
         data_bt = nom_bt;
     }
 
     /* Programme bittiming into HAL */
     const uiox_can_hw_ops_t *ops =
         (const uiox_can_hw_ops_t *)hw->priv;
     if (ops && ops->set_bittiming) {
         rc = ops->set_bittiming(hw, &nom_bt, &data_bt);
         if (rc < 0) return rc;
     }
 
     memcpy(&hw->nom_bt,  &nom_bt,  sizeof(nom_bt));
     memcpy(&hw->data_bt, &data_bt, sizeof(data_bt));
     hw->fd_enabled = fd_enabled;
 
     uiox_can_buf_init();
     return 0;
 }
 
 int uiox_can_if_add_filter(uiox_can_if_t *cif,
                             uint32_t id, uint32_t mask, bool ext)
 {
     if (!cif || cif->filter_count >= UIOX_CAN_MAX_FILTERS)
         return -ENOSPC;
 
     uint8_t idx = cif->filter_count;
     cif->filters[idx].id      = id;
     cif->filters[idx].mask    = mask;
     cif->filters[idx].ext     = ext;
     cif->filters[idx].enabled = true;
     cif->filter_count++;
 
     return uiox_can_hw_set_filter(cif->hw, idx, id, mask, ext);
 }
 
 void uiox_can_if_clr_filters(uiox_can_if_t *cif)
 {
     if (!cif) return;
     memset(cif->filters, 0, sizeof(cif->filters));
     cif->filter_count = 0;
     const uiox_can_hw_ops_t *ops =
         (const uiox_can_hw_ops_t *)cif->hw->priv;
     if (ops && ops->clear_filters) ops->clear_filters(cif->hw);
 }
 
 int uiox_can_if_tx(uiox_can_if_t *cif, uiox_can_msg_t *msg)
 {
     if (!cif || !msg) return -EINVAL;
 
     const uiox_can_hw_ops_t *ops =
         (const uiox_can_hw_ops_t *)cif->hw->priv;
 
     /* Try to submit directly to hardware */
     if (ops && ops->tx_submit) {
         int rc = ops->tx_submit(cif->hw,
                                 (uintptr_t)msg,
                                 sizeof(uiox_can_msg_t));
         if (rc == 0) {
             cif->stats.tx_frames++;
             cif->stats.tx_bytes += uiox_can_dlc2len(msg->dlc);
             return 0;
         }
     }
 
     /* HW FIFO full — queue in software */
     int rc = txq_push(cif, msg);
     if (rc < 0) {
         cif->stats.tx_dropped++;
         return rc;
     }
     return 0;
 }
 
 uiox_can_msg_t *uiox_can_if_rx(uiox_can_if_t *cif)
 {
     if (!cif) return NULL;
 
     const uiox_can_hw_ops_t *ops =
         (const uiox_can_hw_ops_t *)cif->hw->priv;
 
     /* Drain hardware RX into software queue */
     if (ops && ops->rx_poll) {
         uintptr_t phys  = 0;
         uint32_t  len   = 0;
         while (ops->rx_poll(cif->hw, &phys, &len) > 0) {
             uiox_can_msg_t *msg = uiox_can_buf_alloc_rx();
             if (!msg) { cif->stats.rx_overflows++; break; }
             memcpy(msg, (void *)phys, sizeof(uiox_can_msg_t));
             msg->channel = cif->channel;
             cif->stats.rx_frames++;
             cif->stats.rx_bytes += uiox_can_dlc2len(msg->dlc);
             if (rxq_push(cif, msg) < 0) {
                 uiox_can_buf_free(msg);
                 break;
             }
         }
     }
 
     return rxq_pop(cif);
 }
 
 void uiox_can_if_tx_flush(uiox_can_if_t *cif)
 {
     if (!cif) return;
     const uiox_can_hw_ops_t *ops =
         (const uiox_can_hw_ops_t *)cif->hw->priv;
     uiox_can_msg_t *msg;
     while ((msg = txq_pop(cif)) != NULL) {
         if (ops && ops->tx_submit)
             ops->tx_submit(cif->hw, (uintptr_t)msg,
                            sizeof(uiox_can_msg_t));
         cif->stats.tx_frames++;
     }
 }
 
 void uiox_can_if_stats_get(const uiox_can_if_t *cif,
                             uiox_can_if_stats_t *out)
 {
     if (!cif || !out) return;
     memcpy(out, &cif->stats, sizeof(*out));
 }
 
 void uiox_can_if_stats_reset(uiox_can_if_t *cif)
 {
     if (!cif) return;
     memset(&cif->stats, 0, sizeof(cif->stats));
 }
 