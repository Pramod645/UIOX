/**
 * @file  uiox_fw_net.h
 * @brief UIOX Firmware — Network MAC/PHY HAL (VirtIO-net / SMSC LAN9118).
 * @version 1.0.0
 * @date    2026-06-21
 */

 #ifndef UIOX_FW_NET_H
 #define UIOX_FW_NET_H
 
 #include "uiox_fw_hw.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 #define UIOX_FW_NET_MAC_LEN       6u
 #define UIOX_FW_NET_MTU           1514u
 #define UIOX_FW_NET_NAME_LEN      16u
 
 typedef enum {
     UIOX_FW_NET_VIRTIO = 0,
     UIOX_FW_NET_SMSC,
     UIOX_FW_NET_E1000,
     UIOX_FW_NET_LOOPBACK,
 } uiox_fw_net_type_t;
 
 typedef void (*uiox_fw_net_rx_cb_t)(const uint8_t *frame,
                                       uint32_t len, void *priv);
 
 typedef struct {
     uiox_fw_net_type_t type;
     char               name[UIOX_FW_NET_NAME_LEN];
     uintptr_t          base;
     uint32_t           irq;
     uint8_t            mac[UIOX_FW_NET_MAC_LEN];
     bool               link_up;
     uint32_t           speed_mbps;
     uiox_fw_net_rx_cb_t rx_cb;
     void              *rx_priv;
     void              *priv;
     /* Ops */
     uiox_fw_err_t (*init)   (void *priv);
     uiox_fw_err_t (*send)   (void *priv, const uint8_t *frame,
                               uint32_t len);
     /* Stats */
     uint64_t           tx_frames;
     uint64_t           rx_frames;
     uint64_t           tx_bytes;
     uint64_t           rx_bytes;
     uint32_t           errors;
 } uiox_fw_net_dev_t;
 
 /* API */
 uiox_fw_err_t  uiox_fw_net_init     (uiox_fw_net_dev_t *dev);
 uiox_fw_err_t  uiox_fw_net_send     (uiox_fw_net_dev_t *dev,
                                        const uint8_t *frame, uint32_t len);
 void           uiox_fw_net_set_rx_cb(uiox_fw_net_dev_t *dev,
                                        uiox_fw_net_rx_cb_t cb, void *priv);
 void           uiox_fw_net_irq      (uiox_fw_net_dev_t *dev);
 void           uiox_fw_net_print    (const uiox_fw_net_dev_t *dev);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_FW_NET_H */
 