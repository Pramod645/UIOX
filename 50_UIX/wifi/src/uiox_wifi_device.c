/**
 * @file    uiox_wifi_device.c
 * @brief   UIOX WiFi device API implementation.
 * @date    2026-05-28
 */

 #include "uiox_wifi_device.h"
 #include <string.h>
 #include <errno.h>
 #include <stdio.h>
 
 int uiox_wifi_open(uiox_wifi_device_t           *dev,
                     const uiox_wifi_open_params_t *p)
 {
     if (!dev || !p || !p->hw || !p->hw_ops) return -EINVAL;
     memset(dev, 0, sizeof(*dev));
     dev->hw = p->hw;
 
     int rc = uiox_wifi_hw_init(p->hw, p->hw_ops);
     if (rc < 0) return rc;
 
     rc = uiox_wifi_subsys_init(&dev->subsys, p->hw, &p->subsys_cfg);
     if (rc < 0) return rc;
 
     if (p->evt_cb)
         uiox_wifi_subsys_set_evt_cb(&dev->subsys, p->evt_cb, p->evt_ctx);
 
     dev->open = true;
     return 0;
 }
 
 int uiox_wifi_start(uiox_wifi_device_t *dev)
 {
     if (!dev || !dev->open) return -EINVAL;
     return uiox_wifi_subsys_start(&dev->subsys);
 }
 
 void uiox_wifi_stop(uiox_wifi_device_t *dev)
 {
     if (!dev || !dev->open) return;
     uiox_wifi_subsys_stop(&dev->subsys);
 }
 
 void uiox_wifi_close(uiox_wifi_device_t *dev)
 {
     if (!dev || !dev->open) return;
     uiox_wifi_stop(dev);
     uiox_wifi_hw_deinit(dev->hw);
     dev->open = false;
 }
 
 int uiox_wifi_scan(uiox_wifi_device_t *dev, uint32_t timeout_ms)
 {
     if (!dev || !dev->open) return -EINVAL;
     return uiox_wifi_subsys_scan(&dev->subsys, timeout_ms);
 }
 
 int uiox_wifi_connect(uiox_wifi_device_t *dev,
                        const char *ssid, const char *passphrase,
                        uint32_t timeout_ms)
 {
     if (!dev || !dev->open) return -EINVAL;
     return uiox_wifi_subsys_connect(&dev->subsys, ssid, passphrase, timeout_ms);
 }
 
 int uiox_wifi_disconnect(uiox_wifi_device_t *dev)
 {
     if (!dev || !dev->open) return -EINVAL;
     return uiox_wifi_subsys_disconnect(&dev->subsys);
 }
 
 void uiox_wifi_tick(uiox_wifi_device_t *dev, uint32_t now_ms)
 {
     if (!dev || !dev->open) return;
     uiox_wifi_subsys_tick(&dev->subsys, now_ms);
 }
 
 int uiox_wifi_tx(uiox_wifi_device_t    *dev,
                   const uiox_wifi_mac_t  dst,
                   uint16_t               ethertype,
                   const uint8_t         *payload,
                   uint16_t               len)
 {
     if (!dev || !dev->open) return -EINVAL;
     return uiox_wifi_proto_tx_data(&dev->subsys.proto, dst,
                                     ethertype, payload, len);
 }
 
 bool uiox_wifi_connected(const uiox_wifi_device_t *dev)
 {
     if (!dev || !dev->open) return false;
     return dev->subsys.state == UIOX_WIFI_SUBSYS_CONNECTED;
 }
 
 void uiox_wifi_get_quality(uiox_wifi_device_t  *dev,
                             uiox_wifi_quality_t *out)
 {
     if (!dev || !out) return;
     uiox_wifi_subsys_quality(&dev->subsys, out);
 }
 
 void uiox_wifi_get_mac(const uiox_wifi_device_t *dev,
                         uiox_wifi_mac_t mac_out)
 {
     if (!dev || !mac_out) return;
     memcpy(mac_out, dev->hw->mac_addr, UIOX_WIFI_MAC_LEN);
 }
 
 const uiox_wifi_bss_t *uiox_wifi_bss_list(const uiox_wifi_device_t *dev,
                                             uint8_t *count_out)
 {
     if (!dev) return NULL;
     return uiox_wifi_subsys_bss_list(&dev->subsys, count_out);
 }
 
 void uiox_wifi_print_stats(const uiox_wifi_device_t *dev)
 {
     if (!dev) return;
     const uiox_wifi_quality_t *q = &dev->subsys.quality;
     uiox_wifi_if_stats_t s;
     uiox_wifi_if_stats_get(&dev->subsys.wif, &s);
     printf("  State          : %s\n",
            uiox_wifi_state_name(dev->subsys.state));
     printf("  RSSI           : %d dBm\n",  q->rssi_dbm);
     printf("  Link quality   : %u %%\n",   q->link_quality);
     printf("  TX rate idx    : %u\n",       q->tx_rate_idx);
     printf("  TX frames      : %llu\n",
            (unsigned long long)s.tx_frames);
     printf("  RX frames      : %llu\n",
            (unsigned long long)s.rx_frames);
     printf("  TX retries     : %llu\n",
            (unsigned long long)s.tx_retries);
     printf("  TX dropped     : %llu\n",
            (unsigned long long)s.tx_dropped);
     printf("  Reconnects     : %u\n",       q->reconnect_count);
     printf("  TX buf free    : %u / %u\n",
            uiox_wifi_buf_tx_free(), UIOX_WIFI_TX_POOL_SIZE);
     printf("  RX buf free    : %u / %u\n",
            uiox_wifi_buf_rx_free(), UIOX_WIFI_RX_POOL_SIZE);
 }
 
 const char *uiox_wifi_state_name(uiox_wifi_subsys_state_t s)
 {
     switch (s) {
     case UIOX_WIFI_SUBSYS_STOPPED:      return "STOPPED";
     case UIOX_WIFI_SUBSYS_SCANNING:     return "SCANNING";
     case UIOX_WIFI_SUBSYS_CONNECTING:   return "CONNECTING";
     case UIOX_WIFI_SUBSYS_CONNECTED:    return "CONNECTED";
     case UIOX_WIFI_SUBSYS_RECONNECTING: return "RECONNECTING";
     default:                             return "UNKNOWN";
     }
 }
 
 const char *uiox_wifi_evt_name(uiox_wifi_evt_t evt)
 {
     switch (evt) {
     case UIOX_WIFI_EVT_SCAN_DONE:    return "SCAN_DONE";
     case UIOX_WIFI_EVT_CONNECTED:    return "CONNECTED";
     case UIOX_WIFI_EVT_DISCONNECTED: return "DISCONNECTED";
     case UIOX_WIFI_EVT_AUTH_FAILED:  return "AUTH_FAILED";
     case UIOX_WIFI_EVT_ASSOC_FAILED: return "ASSOC_FAILED";
     case UIOX_WIFI_EVT_IP_ACQUIRED:  return "IP_ACQUIRED";
     case UIOX_WIFI_EVT_RSSI_LOW:     return "RSSI_LOW";
     default:                          return "UNKNOWN";
     }
 }
 