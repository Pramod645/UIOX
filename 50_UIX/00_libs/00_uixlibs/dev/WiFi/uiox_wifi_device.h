/*
 * 50_UIX/00_libs/00_uixlibs/dev/WiFi/uiox_wifi_device.h
 *
 * UIOX WiFi top-level application-facing device API — EL0.
 *
 *   21_apps/app_wifi.c → dev/WiFi/uiox_wifi_device.c
 *     → dev/uiox_dev_stream.c → 40_SCIX → 30_KIX
 *
 * No 30_KIX header.
 *
 * ── CHANGED from the kernel-side original ───────────────────────────
 *   · "uiox_wifi_subsys.h" removed — it is EL1.  The types it carried
 *     are MIRRORED here, the way dev/emmc/uiox_dev_emmc.h mirrors its
 *     kernel ioctls and structs, with the source named beside each.
 *   · uiox_wifi_open_params_t lost hw/hw_ops — an EL0 caller has no HAL
 *     to hand in; the kernel's device is reached by opening /dev/wlan0.
 *   · uiox_wifi_device_t holds a uiox_dev_handle_t, not a
 *     uiox_wifi_subsys_t.  The subsystem lives in kernel memory.
 *
 * ── values MUST stay identical to the kernel's ──────────────────────
 * Mirrored from 30_KIX/34_CSS/02_Sensors/WiFi/:
 *   uiox_wifi_subsys.h   state enum, evt enum, quality, cfg
 *   uiox_wifi_proto.h    bss, UIOX_WIFI_SSID_MAX
 */
#ifndef UIOX_WIFI_DEVICE_H
#define UIOX_WIFI_DEVICE_H

#include "../uiox_dev_stream.h"   /* uiox_dev_handle_t, uiox_dev_err_t */
#include "../uiox_dev.h"          /* UIOX_O_RDWR, UIOX_DEV_PATH_MAX    */

#ifdef __cplusplus
extern "C" {
#endif

#define UIOX_WIFI_SSID_MAX   32
#define UIOX_WIFI_MAC_LEN     6
#define UIOX_WIFI_DEVPATH   "wlan"

typedef uix_uint8_t uiox_wifi_mac_t[UIOX_WIFI_MAC_LEN];

/* uiox_wifi_subsys.h */
typedef enum {
    UIOX_WIFI_SUBSYS_STOPPED = 0,
    UIOX_WIFI_SUBSYS_SCANNING,
    UIOX_WIFI_SUBSYS_CONNECTING,
    UIOX_WIFI_SUBSYS_CONNECTED,
    UIOX_WIFI_SUBSYS_RECONNECTING,
} uiox_wifi_subsys_state_t;

typedef enum {
    UIOX_WIFI_EVT_SCAN_DONE = 0,
    UIOX_WIFI_EVT_CONNECTED,
    UIOX_WIFI_EVT_DISCONNECTED,
    UIOX_WIFI_EVT_AUTH_FAILED,
    UIOX_WIFI_EVT_ASSOC_FAILED,
    UIOX_WIFI_EVT_IP_ACQUIRED,
    UIOX_WIFI_EVT_RSSI_LOW,
} uiox_wifi_evt_t;

/* uiox_wifi_subsys.h — eight scalars, no pointer */
typedef struct {
    uix_int8_t   rssi_dbm;
    uix_int8_t   noise_dbm;
    uix_uint8_t  link_quality;      /**< 0..100 %                      */
    uix_uint8_t  tx_rate_idx;
    uix_uint32_t tx_frames;
    uix_uint32_t rx_frames;
    uix_uint32_t tx_retries;
    uix_uint32_t reconnect_count;
} uiox_wifi_quality_t;

/* uiox_wifi_subsys.h — the config the kernel's subsystem takes */
typedef struct {
    char         ssid[UIOX_WIFI_SSID_MAX + 1];
    char         passphrase[64];
    uix_bool_t   auto_reconnect;
    uix_uint32_t reconnect_delay_ms;
    uix_uint32_t scan_interval_ms;
    uix_uint32_t ps_mode;
} uiox_wifi_subsys_cfg_t;

/* uiox_wifi_proto.h — one scanned BSS */
typedef struct {
    uiox_wifi_mac_t bssid;
    char            ssid[UIOX_WIFI_SSID_MAX + 1];
    uix_uint8_t     ssid_len;
    uix_uint8_t     channel;
    uix_int8_t      rssi_dbm;
    uix_uint16_t    caps;
    uix_uint8_t     cipher;
    uix_uint8_t     akm;
    uix_uint32_t    beacon_int_tu;
} uiox_wifi_bss_t;

/* ── the device handle ───────────────────────────────────────────────
 * The shape's own handle.  No HAL pointer: the kernel owns the hardware
 * and /dev/wlan0 is how it is reached. */
typedef struct {
    uiox_dev_handle_t       h;
    uiox_wifi_subsys_state_t last_state;   /**< filled by quality()'s read */
    uix_bool_t              open;
} uiox_wifi_device_t;

/* ── the API ─────────────────────────────────────────────────────────
 * Same verbs as the kernel-side device API, minus the ones a trap
 * cannot carry — each named here rather than silently dropped:
 *
 *   connect(ssid, passphrase)   a credential crossing the trap is a
 *                               design decision not yet made; no kernel
 *                               row exists for it.
 *   bss_list(count_out)         would return a pointer into kernel
 *                               memory (proto->bss_cache).
 *   tx(dst, ethertype, …)       takes a payload the kernel would have
 *                               to gate; no kernel row exists.
 */
uiox_dev_err_t uiox_wifi_open      (uiox_wifi_device_t *dev, int unit, int flags);
uiox_dev_err_t uiox_wifi_close     (uiox_wifi_device_t *dev);

uiox_dev_err_t uiox_wifi_scan      (uiox_wifi_device_t *dev, uix_uint32_t timeout_ms);
uiox_dev_err_t uiox_wifi_disconnect(uiox_wifi_device_t *dev);
uiox_dev_err_t uiox_wifi_quality   (uiox_wifi_device_t *dev,
                                     uiox_wifi_quality_t *out);

uiox_wifi_subsys_state_t uiox_wifi_state(const uiox_wifi_device_t *dev);
uix_bool_t               uiox_wifi_connected(const uiox_wifi_device_t *dev);

const char *uiox_wifi_state_name(uiox_wifi_subsys_state_t s);
const char *uiox_wifi_evt_name  (uiox_wifi_evt_t evt);

#ifdef __cplusplus
}
#endif
#endif /* UIOX_WIFI_DEVICE_H */
