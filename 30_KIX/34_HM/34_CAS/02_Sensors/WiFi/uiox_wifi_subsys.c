/**
 * @file    uiox_wifi_subsys.c
 * @brief   UIOX WiFi subsystem implementation.
 * @date    2026-05-28
 */

 #include "uiox_wifi_subsys.h"
 #include <string.h>
 #include <errno.h>
 
 static void fire(uiox_wifi_subsys_t *sys, uiox_wifi_evt_t evt)
 {
     if (sys->evt_cb) sys->evt_cb(evt, sys->evt_ctx);
 }
 
 static void update_quality(uiox_wifi_subsys_t *sys)
 {
     int8_t rssi = 0;
     uiox_wifi_hw_get_rssi(sys->wif.hw, &rssi);
     sys->quality.rssi_dbm  = rssi;
     sys->quality.tx_rate_idx = sys->wif.rate.rate_idx;
 
     uiox_wifi_if_stats_t s;
     uiox_wifi_if_stats_get(&sys->wif, &s);
     sys->quality.tx_frames  = (uint32_t)s.tx_frames;
     sys->quality.rx_frames  = (uint32_t)s.rx_frames;
     sys->quality.tx_retries = (uint32_t)s.tx_retries;
 
     /* Link quality: linear map −90 dBm=0%, −40 dBm=100% */
     int q = (int)(rssi + 90) * 2;
     if (q < 0)   q = 0;
     if (q > 100) q = 100;
     sys->quality.link_quality = (uint8_t)q;
 }
 
 int uiox_wifi_subsys_init(uiox_wifi_subsys_t           *sys,
                            uiox_wifi_hw_t               *hw,
                            const uiox_wifi_subsys_cfg_t *cfg)
 {
     if (!sys || !hw || !cfg) return -EINVAL;
     memset(sys, 0, sizeof(*sys));
     memcpy(&sys->cfg, cfg, sizeof(*cfg));
     sys->connect_timeout_ms = 15000u;
 
     int rc = uiox_wifi_if_config(&sys->wif, hw);
     if (rc < 0) return rc;
 
     rc = uiox_wifi_proto_init(&sys->proto, &sys->wif);
     if (rc < 0) return rc;
 
     sys->state = UIOX_WIFI_SUBSYS_STOPPED;
     return 0;
 }
 
 int uiox_wifi_subsys_start(uiox_wifi_subsys_t *sys)
 {
     if (!sys) return -EINVAL;
     int rc = uiox_wifi_hw_start(sys->wif.hw);
     if (rc < 0) return rc;
     uiox_wifi_buf_init();
     sys->state = UIOX_WIFI_SUBSYS_SCANNING;
     return 0;
 }
 
 void uiox_wifi_subsys_stop(uiox_wifi_subsys_t *sys)
 {
     if (!sys) return;
     if (sys->state == UIOX_WIFI_SUBSYS_CONNECTED)
         uiox_wifi_proto_disconnect(&sys->proto);
     uiox_wifi_hw_stop(sys->wif.hw);
     sys->state = UIOX_WIFI_SUBSYS_STOPPED;
 }
 
 int uiox_wifi_subsys_scan(uiox_wifi_subsys_t *sys, uint32_t timeout_ms)
 {
     if (!sys) return -EINVAL;
     sys->state = UIOX_WIFI_SUBSYS_SCANNING;
     int n = uiox_wifi_proto_scan(&sys->proto, timeout_ms);
     sys->state = UIOX_WIFI_SUBSYS_STOPPED;
     if (n >= 0) fire(sys, UIOX_WIFI_EVT_SCAN_DONE);
     return n;
 }
 
 int uiox_wifi_subsys_connect(uiox_wifi_subsys_t *sys,
                               const char *ssid,
                               const char *passphrase,
                               uint32_t timeout_ms)
 {
     if (!sys || !ssid || !passphrase) return -EINVAL;
     sys->state = UIOX_WIFI_SUBSYS_CONNECTING;
 
     int rc = uiox_wifi_proto_connect(&sys->proto, ssid, passphrase,
                                       timeout_ms ? timeout_ms :
                                       sys->connect_timeout_ms);
     if (rc == 0) {
         sys->state = UIOX_WIFI_SUBSYS_CONNECTED;
         fire(sys, UIOX_WIFI_EVT_CONNECTED);
     } else {
         sys->state = UIOX_WIFI_SUBSYS_STOPPED;
         fire(sys, rc == -ETIMEDOUT ? UIOX_WIFI_EVT_AUTH_FAILED :
                                      UIOX_WIFI_EVT_DISCONNECTED);
     }
     return rc;
 }
 
 int uiox_wifi_subsys_disconnect(uiox_wifi_subsys_t *sys)
 {
     if (!sys) return -EINVAL;
     int rc = uiox_wifi_proto_disconnect(&sys->proto);
     sys->state = UIOX_WIFI_SUBSYS_STOPPED;
     fire(sys, UIOX_WIFI_EVT_DISCONNECTED);
     return rc;
 }
 
 void uiox_wifi_subsys_tick(uiox_wifi_subsys_t *sys, uint32_t now_ms)
 {
     if (!sys || sys->state == UIOX_WIFI_SUBSYS_STOPPED) return;
 
     /* Protocol tick (beacon watchdog) */
     uiox_wifi_proto_tick(&sys->proto, now_ms);
 
     /* Auto-reconnect */
     if (sys->cfg.auto_reconnect &&
         sys->state != UIOX_WIFI_SUBSYS_CONNECTED &&
         sys->state != UIOX_WIFI_SUBSYS_CONNECTING &&
         (now_ms - sys->last_reconnect_ms) >= sys->cfg.reconnect_delay_ms) {
         sys->last_reconnect_ms = now_ms;
         sys->quality.reconnect_count++;
         sys->state = UIOX_WIFI_SUBSYS_RECONNECTING;
         uiox_wifi_subsys_connect(sys, sys->cfg.ssid,
                                   sys->cfg.passphrase, 0u);
     }
 
     /* Background periodic scan */
     if (sys->cfg.scan_interval_ms &&
         sys->state != UIOX_WIFI_SUBSYS_CONNECTING &&
         (now_ms - sys->last_scan_ms) >= sys->cfg.scan_interval_ms) {
         sys->last_scan_ms = now_ms;
         uiox_wifi_proto_scan(&sys->proto, 200u);
         fire(sys, UIOX_WIFI_EVT_SCAN_DONE);
     }
 
     /* RSSI quality update */
     if (sys->state == UIOX_WIFI_SUBSYS_CONNECTED)
         update_quality(sys);
 
     /* Low RSSI alert */
     if (sys->quality.rssi_dbm < -80 &&
         sys->state == UIOX_WIFI_SUBSYS_CONNECTED)
         fire(sys, UIOX_WIFI_EVT_RSSI_LOW);
 
     /* Drain RX */
     uiox_wifi_frame_t *rx;
     while ((rx = uiox_wifi_if_rx(&sys->wif)) != NULL) {
         uiox_wifi_frame_t *data = NULL;
         uiox_wifi_proto_rx(&sys->proto, rx, &data);
         if (!data) uiox_wifi_buf_free(rx);
         /* data frames delivered to application via uiox_wifi_device */
     }
 }
 
 void uiox_wifi_subsys_set_evt_cb(uiox_wifi_subsys_t *sys,
                                   uiox_wifi_evt_cb_t cb, void *ctx)
 {
     if (!sys) return;
     sys->evt_cb  = cb;
     sys->evt_ctx = ctx;
 }
 
 void uiox_wifi_subsys_quality(uiox_wifi_subsys_t  *sys,
                                uiox_wifi_quality_t *out)
 {
     if (!sys || !out) return;
     memcpy(out, &sys->quality, sizeof(*out));
 }
 
 const uiox_wifi_bss_t *uiox_wifi_subsys_bss_list(
     const uiox_wifi_subsys_t *sys, uint8_t *count_out)
 {
     if (!sys) return NULL;
     if (count_out) *count_out = sys->proto.bss_count;
     return sys->proto.bss_cache;
 }
 