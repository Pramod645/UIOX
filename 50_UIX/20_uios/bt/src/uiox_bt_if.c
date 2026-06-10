/**
 * @file    uiox_bt_if.c
 * @brief   UIOX Bluetooth HCI transport interface driver implementation.
 * @date    2026-06-09
 */

 #include "uiox_bt_if.h"
 #include <string.h>
 #include <errno.h>
 
 int uiox_bt_if_config(uiox_bt_if_t *bif, uiox_bt_hw_t *hw)
 {
     if (!bif || !hw) return -EINVAL;
     memset(bif, 0, sizeof(*bif));
     bif->hw     = hw;
     bif->primed = true;
     uiox_bt_buf_init();
     uiox_bt_evt_ring_init(&bif->evt_ring);
     return 0;
 }
 
 int uiox_bt_if_start(uiox_bt_if_t *bif)
 {
     if (!bif || !bif->primed) return -EINVAL;
     int rc = uiox_bt_hw_power(bif->hw, true);
     if (rc < 0) return rc;
     /* Small delay for controller startup */
     const uiox_bt_hw_ops_t *ops =
         (const uiox_bt_hw_ops_t *)bif->hw->priv;
     if (ops && ops->delay_ms) ops->delay_ms(bif->hw, 200u);
     bif->hw->initialised = true;
     return 0;
 }
 
 void uiox_bt_if_stop(uiox_bt_if_t *bif)
 {
     if (!bif) return;
     uiox_bt_hw_power(bif->hw, false);
 }
 
 int uiox_bt_if_hci_raw(uiox_bt_if_t *bif,
                         const uint8_t *buf, uint16_t len)
 {
     if (!bif || !buf || !len) return -EINVAL;
     int rc = uiox_bt_hw_hci_write(bif->hw, buf, len);
     if (rc >= 0) {
         bif->stats.bytes_tx += len;
         bif->stats.hci_cmds_sent++;
     } else {
         bif->stats.comm_errors++;
     }
     return rc;
 }
 
 int uiox_bt_if_hci_cmd(uiox_bt_if_t *bif,
                         uint16_t opcode,
                         const uint8_t *params, uint8_t param_len,
                         uint8_t *resp, uint8_t resp_max,
                         uint32_t timeout_ms)
 {
     if (!bif) return -EINVAL;
 
     /* Build HCI command packet */
     uint8_t pkt[UIOX_BT_PKT_MAX_LEN];
     pkt[0] = HCI_CMD_PKT;
     pkt[1] = (uint8_t)(opcode & 0xFFu);
     pkt[2] = (uint8_t)(opcode >> 8u);
     pkt[3] = param_len;
     if (params && param_len)
         memcpy(&pkt[4], params, param_len);
     uint16_t pkt_len = (uint16_t)(4u + param_len);
 
     int rc = uiox_bt_hw_hci_write(bif->hw, pkt, pkt_len);
     if (rc < 0) { bif->stats.cmd_errors++; return rc; }
     bif->stats.bytes_tx += pkt_len;
     bif->stats.hci_cmds_sent++;
 
     /* Wait for HCI_EV_CMD_COMPLETE */
     uint32_t elapsed = 0;
     while (elapsed < timeout_ms) {
         uint8_t rx[UIOX_BT_PKT_MAX_LEN];
         int n = uiox_bt_hw_hci_read(bif->hw, rx, sizeof(rx));
         if (n > 3 && rx[0] == HCI_EVENT_PKT) {
             bif->stats.hci_events_recv++;
             bif->stats.bytes_rx += (uint64_t)n;
             if (rx[1] == HCI_EV_CMD_COMPLETE) {
                 /* rx[4..5] = opcode, rx[6..] = return params */
                 uint16_t comp_op = (uint16_t)(rx[4] | (rx[5] << 8u));
                 if (comp_op == opcode) {
                     if (resp && resp_max > 0 && n > 6) {
                         uint8_t copy = (uint8_t)(n - 6u);
                         if (copy > resp_max) copy = resp_max;
                         memcpy(resp, &rx[6], copy);
                     }
                     /* Status byte is rx[6] */
                     return (n > 6) ? (rx[6] == 0u ? 0 : -EIO) : 0;
                 }
             }
             /* Other events go to ring */
             uiox_bt_evt_push(&bif->evt_ring, rx, (uint16_t)n, rx[0]);
         }
         const uiox_bt_hw_ops_t *ops =
             (const uiox_bt_hw_ops_t *)bif->hw->priv;
         if (ops && ops->delay_ms) ops->delay_ms(bif->hw, 10u);
         elapsed += 10u;
     }
     bif->stats.cmd_errors++;
     return -ETIMEDOUT;
 }
 
 int uiox_bt_if_acl_tx(uiox_bt_if_t *bif,
                        uint16_t handle,
                        const uint8_t *data, uint16_t len)
 {
     if (!bif || !data || !len) return -EINVAL;
     uint8_t pkt[UIOX_BT_PKT_MAX_LEN];
     pkt[0] = HCI_ACL_PKT;
     pkt[1] = (uint8_t)(handle & 0xFFu);
     pkt[2] = (uint8_t)((handle >> 8u) & 0x0Fu);
     pkt[3] = (uint8_t)(len & 0xFFu);
     pkt[4] = (uint8_t)(len >> 8u);
     uint16_t copy = len < (UIOX_BT_PKT_MAX_LEN - 5u) ?
                     len : (UIOX_BT_PKT_MAX_LEN - 5u);
     memcpy(&pkt[5], data, copy);
     int rc = uiox_bt_hw_hci_write(bif->hw, pkt, (uint16_t)(5u + copy));
     if (rc >= 0) {
         bif->stats.acl_tx_pkts++;
         bif->stats.bytes_tx += (5u + copy);
     }
     return rc;
 }
 
 int uiox_bt_if_rx_poll(uiox_bt_if_t *bif)
 {
     if (!bif) return -EINVAL;
     uint8_t rx[UIOX_BT_PKT_MAX_LEN];
     int n = uiox_bt_hw_hci_read(bif->hw, rx, sizeof(rx));
     if (n <= 0) return 0;
     bif->stats.bytes_rx += (uint64_t)n;
     if (rx[0] == HCI_EVENT_PKT) bif->stats.hci_events_recv++;
     if (rx[0] == HCI_ACL_PKT)   bif->stats.acl_rx_pkts++;
     uiox_bt_evt_push(&bif->evt_ring, rx, (uint16_t)n, rx[0]);
     return 1;
 }
 
 void uiox_bt_if_stats_get(const uiox_bt_if_t *bif,
                            uiox_bt_if_stats_t *out)
 { if (!bif || !out) return; memcpy(out, &bif->stats, sizeof(*out)); }
 
 void uiox_bt_if_stats_reset(uiox_bt_if_t *bif)
 { if (!bif) return; memset(&bif->stats, 0, sizeof(bif->stats)); }
 