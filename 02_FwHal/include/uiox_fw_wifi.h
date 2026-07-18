/* ─────────────────────────────────────────────────────────────────────────
 * uiox_fw_wifi.h — Wi-Fi 6 HAL (SDIO / PCIe — Qualcomm / Broadcom)
 * ───────────────────────────────────────────────────────────────────────── */
#ifndef UIOX_FW_WIFI_H
#define UIOX_FW_WIFI_H
#include "uiox_fw_types.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef enum { UIOX_WIFI_SDIO=0, UIOX_WIFI_PCIE=1,
               UIOX_WIFI_USB=2  } uiox_wifi_bus_t;
typedef enum { UIOX_WIFI_802_11N=0, UIOX_WIFI_802_11AC=1,
               UIOX_WIFI_802_11AX=2 } uiox_wifi_std_t;

#define UIOX_WIFI_SSID_MAX   33u
#define UIOX_WIFI_KEY_MAX    64u
#define UIOX_WIFI_MAC_LEN    6u
#define UIOX_WIFI_MAX_SCAN   16u

typedef struct {
    char    ssid[UIOX_WIFI_SSID_MAX];
    uint8_t bssid[UIOX_WIFI_MAC_LEN];
    int8_t  rssi_dbm;
    uint8_t channel;
    bool    wpa2;
} uiox_wifi_ap_t;

typedef void (*uiox_wifi_rx_cb_t)(const uint8_t *frame, uint32_t len, void *p);

typedef struct {
    uintptr_t       base;
    uint32_t        irq;
    uiox_wifi_bus_t bus;
    uiox_wifi_std_t standard;
    uint8_t         mac[UIOX_WIFI_MAC_LEN];
    char            ssid[UIOX_WIFI_SSID_MAX];
    bool            associated;
    int8_t          rssi_dbm;
    uiox_wifi_rx_cb_t rx_cb;
    void           *rx_priv;
    uint64_t        tx_bytes, rx_bytes;
    uiox_wifi_ap_t  scan_results[UIOX_WIFI_MAX_SCAN];
    uint8_t         num_scan;
    bool            fw_loaded;
    bool            initialized;
    void           *priv;
} uiox_wifi_dev_t;

typedef struct {
    uiox_fw_err_t (*init)      (uiox_wifi_dev_t *dev);
    void          (*deinit)    (uiox_wifi_dev_t *dev);
    uiox_fw_err_t (*load_fw)   (uiox_wifi_dev_t *dev,
                                  const uint8_t *fw, uint32_t len);
    uiox_fw_err_t (*scan)      (uiox_wifi_dev_t *dev);
    uiox_fw_err_t (*connect)   (uiox_wifi_dev_t *dev,
                                  const char *ssid, const char *key);
    uiox_fw_err_t (*disconnect)(uiox_wifi_dev_t *dev);
    uiox_fw_err_t (*send)      (uiox_wifi_dev_t *dev,
                                  const uint8_t *frame, uint32_t len);
    void          (*isr)       (uiox_wifi_dev_t *dev);
} uiox_wifi_ops_t;

uiox_fw_err_t uiox_fw_wifi_init      (uiox_wifi_dev_t *dev,
                                        const uiox_wifi_ops_t *ops);
void          uiox_fw_wifi_deinit    (uiox_wifi_dev_t *dev);
uiox_fw_err_t uiox_fw_wifi_scan      (uiox_wifi_dev_t *dev);
uiox_fw_err_t uiox_fw_wifi_connect   (uiox_wifi_dev_t *dev,
                                        const char *ssid, const char *key);
uiox_fw_err_t uiox_fw_wifi_disconnect(uiox_wifi_dev_t *dev);
uiox_fw_err_t uiox_fw_wifi_send      (uiox_wifi_dev_t *dev,
                                        const uint8_t *frame, uint32_t len);
void          uiox_fw_wifi_set_rx_cb (uiox_wifi_dev_t *dev,
                                        uiox_wifi_rx_cb_t cb, void *p);
uiox_fw_err_t uiox_fw_wifi_init_sdio (uiox_wifi_dev_t *dev, uintptr_t base,
                                        uint32_t irq);
#ifdef __cplusplus
}
#endif
#endif /* UIOX_FW_WIFI_H */
