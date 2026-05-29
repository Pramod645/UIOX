/**
 * @file    uiox_wifi_device.h
 * @brief   UIOX WiFi top-level application-facing device API.
 * @date    2026-05-28
 */
//Layer 5 — Device API
 #ifndef UIOX_WIFI_DEVICE_H
 #define UIOX_WIFI_DEVICE_H
 
 #include "uiox_wifi_subsys.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 typedef struct {
     uiox_wifi_hw_t             *hw;
     const uiox_wifi_hw_ops_t   *hw_ops;
     uiox_wifi_subsys_cfg_t      subsys_cfg;
     uiox_wifi_evt_cb_t          evt_cb;
     void                       *evt_ctx;
 } uiox_wifi_open_params_t;
 
 typedef struct {
     uiox_wifi_subsys_t  subsys;
     uiox_wifi_hw_t     *hw;
     bool                open;
 } uiox_wifi_device_t;
 
 int  uiox_wifi_open        (uiox_wifi_device_t           *dev,
                              const uiox_wifi_open_params_t *p);
 int  uiox_wifi_start       (uiox_wifi_device_t *dev);
 void uiox_wifi_stop        (uiox_wifi_device_t *dev);
 void uiox_wifi_close       (uiox_wifi_device_t *dev);
 
 int  uiox_wifi_scan        (uiox_wifi_device_t *dev, uint32_t timeout_ms);
 int  uiox_wifi_connect     (uiox_wifi_device_t *dev,
                              const char *ssid, const char *passphrase,
                              uint32_t timeout_ms);
 int  uiox_wifi_disconnect  (uiox_wifi_device_t *dev);
 void uiox_wifi_tick        (uiox_wifi_device_t *dev, uint32_t now_ms);
 
 int  uiox_wifi_tx          (uiox_wifi_device_t    *dev,
                              const uiox_wifi_mac_t  dst,
                              uint16_t               ethertype,
                              const uint8_t         *payload,
                              uint16_t               len);
 
 bool uiox_wifi_connected   (const uiox_wifi_device_t *dev);
 void uiox_wifi_get_quality (uiox_wifi_device_t  *dev,
                              uiox_wifi_quality_t *out);
 void uiox_wifi_get_mac     (const uiox_wifi_device_t *dev,
                              uiox_wifi_mac_t mac_out);
 
 const uiox_wifi_bss_t *uiox_wifi_bss_list(const uiox_wifi_device_t *dev,
                                             uint8_t *count_out);
 void uiox_wifi_print_stats (const uiox_wifi_device_t *dev);
 
 const char *uiox_wifi_state_name(uiox_wifi_subsys_state_t state);
 const char *uiox_wifi_evt_name  (uiox_wifi_evt_t evt);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_WIFI_DEVICE_H */
 