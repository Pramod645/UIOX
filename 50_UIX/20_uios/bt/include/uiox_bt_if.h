/**
 * @file    uiox_bt_if.h
 * @brief   UIOX Bluetooth HCI transport interface driver.
 * @date    2026-06-09
 */

 #ifndef UIOX_BT_IF_H
 #define UIOX_BT_IF_H
 
 #include "uiox_bt_hw.h"
 #include "uiox_bt_buf.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 typedef struct {
     uint64_t  hci_cmds_sent;
     uint64_t  hci_events_recv;
     uint64_t  acl_tx_pkts;
     uint64_t  acl_rx_pkts;
     uint64_t  bytes_tx;
     uint64_t  bytes_rx;
     uint32_t  cmd_errors;
     uint32_t  comm_errors;
 } uiox_bt_if_stats_t;
 
 typedef struct {
     uiox_bt_hw_t        *hw;
     uiox_bt_evt_ring_t   evt_ring;
     uiox_bt_if_stats_t   stats;
     bool                 primed;
 } uiox_bt_if_t;
 
 int  uiox_bt_if_config     (uiox_bt_if_t *bif, uiox_bt_hw_t *hw);
 int  uiox_bt_if_start      (uiox_bt_if_t *bif);
 void uiox_bt_if_stop       (uiox_bt_if_t *bif);
 
 /**
  * @brief  Send an HCI command and wait for cmd_complete event.
  * @param  opcode      HCI opcode.
  * @param  params      Command parameters.
  * @param  param_len   Length of params.
  * @param  resp        Buffer for return parameters.
  * @param  resp_max    Size of resp.
  * @param  timeout_ms  Timeout.
  * @return 0 on success, negative errno on error.
  */
 int  uiox_bt_if_hci_cmd    (uiox_bt_if_t *bif,
                              uint16_t opcode,
                              const uint8_t *params, uint8_t param_len,
                              uint8_t *resp, uint8_t resp_max,
                              uint32_t timeout_ms);
 
 /** Send raw HCI packet. */
 int  uiox_bt_if_hci_raw    (uiox_bt_if_t *bif,
                              const uint8_t *buf, uint16_t len);
 
 /** Send ACL data. */
 int  uiox_bt_if_acl_tx     (uiox_bt_if_t *bif,
                              uint16_t handle,
                              const uint8_t *data, uint16_t len);
 
 /** Poll for received events and data, push to ring. */
 int  uiox_bt_if_rx_poll    (uiox_bt_if_t *bif);
 
 void uiox_bt_if_stats_get  (const uiox_bt_if_t *bif,
                              uiox_bt_if_stats_t *out);
 void uiox_bt_if_stats_reset(uiox_bt_if_t *bif);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_BT_IF_H */
 