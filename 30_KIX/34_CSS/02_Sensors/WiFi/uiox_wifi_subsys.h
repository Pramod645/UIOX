/**
 * @file    uiox_wifi_subsys.h
 * @brief   UIOX WiFi subsystem — connect, scan, DHCP, power management.
 *
 * Top layer. Manages:
 *   - Connection lifecycle with auto-reconnect
 *   - Background periodic scan
 *   - DHCP client integration (via uiox_connectivity)
 *   - Power save mode (PSM / U-APSD)
 *   - WiFi event callbacks (connected, disconnected, scan done)
 *   - Connection statistics and quality metrics
 *
 * @date    2026-05-28
 */
//Layer 4 — Subsystem
 #ifndef UIOX_WIFI_SUBSYS_H
 #define UIOX_WIFI_SUBSYS_H
 
 #include "uiox_wifi_proto.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * WiFi events
  * ====================================================================== */
 
 typedef enum {
     UIOX_WIFI_EVT_SCAN_DONE = 0,
     UIOX_WIFI_EVT_CONNECTED,
     UIOX_WIFI_EVT_DISCONNECTED,
     UIOX_WIFI_EVT_AUTH_FAILED,
     UIOX_WIFI_EVT_ASSOC_FAILED,
     UIOX_WIFI_EVT_IP_ACQUIRED,
     UIOX_WIFI_EVT_RSSI_LOW,
 } uiox_wifi_evt_t;
 
 typedef void (*uiox_wifi_evt_cb_t)(uiox_wifi_evt_t evt, void *ctx);
 
 /* =========================================================================
  * Power save modes
  * ====================================================================== */
 
 typedef enum {
     UIOX_WIFI_PS_NONE = 0,    /**< Always active                          */
     UIOX_WIFI_PS_MIN,          /**< Legacy PSM (listen every DTIM)        */
     UIOX_WIFI_PS_MAX,          /**< Deep sleep, wake on beacon            */
 } uiox_wifi_ps_mode_t;
 
 /* =========================================================================
  * Subsystem configuration
  * ====================================================================== */
 
 typedef struct {
     char     ssid[UIOX_WIFI_SSID_MAX + 1];
     char     passphrase[64];
     bool     auto_reconnect;
     uint32_t reconnect_delay_ms;
     uint32_t scan_interval_ms;    /**< 0 = no background scan             */
     uiox_wifi_ps_mode_t ps_mode;
 } uiox_wifi_subsys_cfg_t;
 
 /* =========================================================================
  * Subsystem state
  * ====================================================================== */
 
 typedef enum {
     UIOX_WIFI_SUBSYS_STOPPED = 0,
     UIOX_WIFI_SUBSYS_SCANNING,
     UIOX_WIFI_SUBSYS_CONNECTING,
     UIOX_WIFI_SUBSYS_CONNECTED,
     UIOX_WIFI_SUBSYS_RECONNECTING,
 } uiox_wifi_subsys_state_t;
 
 /* =========================================================================
  * Quality metrics
  * ====================================================================== */
 
 typedef struct {
     int8_t   rssi_dbm;
     int8_t   noise_dbm;
     uint8_t  link_quality;     /**< 0..100 %                              */
     uint8_t  tx_rate_idx;
     uint32_t tx_frames;
     uint32_t rx_frames;
     uint32_t tx_retries;
     uint32_t reconnect_count;
 } uiox_wifi_quality_t;
 
 /* =========================================================================
  * Subsystem descriptor
  * ====================================================================== */
 
 typedef struct {
     uiox_wifi_if_t           wif;
     uiox_wifi_proto_t        proto;
     uiox_wifi_subsys_cfg_t   cfg;
     uiox_wifi_subsys_state_t state;
     uiox_wifi_quality_t      quality;
     uiox_wifi_evt_cb_t       evt_cb;
     void                    *evt_ctx;
     uint32_t                 last_scan_ms;
     uint32_t                 last_reconnect_ms;
     uint32_t                 connect_timeout_ms;
 } uiox_wifi_subsys_t;
 
 /* =========================================================================
  * Subsystem API
  * ====================================================================== */
 
 int  uiox_wifi_subsys_init    (uiox_wifi_subsys_t           *sys,
                                 uiox_wifi_hw_t               *hw,
                                 const uiox_wifi_subsys_cfg_t *cfg);
 
 int  uiox_wifi_subsys_start   (uiox_wifi_subsys_t *sys);
 void uiox_wifi_subsys_stop    (uiox_wifi_subsys_t *sys);
 
 int  uiox_wifi_subsys_scan    (uiox_wifi_subsys_t *sys,
                                 uint32_t timeout_ms);
 
 int  uiox_wifi_subsys_connect (uiox_wifi_subsys_t *sys,
                                 const char *ssid,
                                 const char *passphrase,
                                 uint32_t timeout_ms);
 
 int  uiox_wifi_subsys_disconnect(uiox_wifi_subsys_t *sys);
 
 void uiox_wifi_subsys_tick    (uiox_wifi_subsys_t *sys, uint32_t now_ms);
 
 void uiox_wifi_subsys_set_evt_cb(uiox_wifi_subsys_t *sys,
                                   uiox_wifi_evt_cb_t cb, void *ctx);
 
 void uiox_wifi_subsys_quality (uiox_wifi_subsys_t  *sys,
                                 uiox_wifi_quality_t *out);
 
 const uiox_wifi_bss_t *uiox_wifi_subsys_bss_list(
     const uiox_wifi_subsys_t *sys, uint8_t *count_out);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_WIFI_SUBSYS_H */
 