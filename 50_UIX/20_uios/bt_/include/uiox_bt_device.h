/* uiox_bt_device.h */
#ifndef UIOX_BT_DEVICE_H
#define UIOX_BT_DEVICE_H
#include "uiox_bt_subsys.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
    uiox_bt_hw_t            *hw;
    const uiox_bt_hw_ops_t  *hw_ops;
    uiox_bt_evt_cb_t         evt_cb;
    void                    *evt_ctx;
} uiox_bt_open_params_t;
typedef struct {
    uiox_bt_subsys_t  subsys;
    uiox_bt_hw_t     *hw;
    bool              open;
} uiox_bt_device_t;

int  uiox_bt_open          (uiox_bt_device_t           *dev,
                             const uiox_bt_open_params_t *p);
int  uiox_bt_start         (uiox_bt_device_t *dev);
void uiox_bt_stop          (uiox_bt_device_t *dev);
void uiox_bt_close         (uiox_bt_device_t *dev);
void uiox_bt_tick          (uiox_bt_device_t *dev, uint32_t now_ms);
int  uiox_bt_set_name      (uiox_bt_device_t *dev, const char *name);
int  uiox_bt_scan_start    (uiox_bt_device_t *dev, uint8_t dur_s);
int  uiox_bt_scan_stop     (uiox_bt_device_t *dev);
int  uiox_bt_le_scan_start (uiox_bt_device_t *dev,
                             uint16_t interval, uint16_t window, bool active);
int  uiox_bt_le_scan_stop  (uiox_bt_device_t *dev);
int  uiox_bt_adv_start     (uiox_bt_device_t *dev,
                             const uint8_t *adv_data, uint8_t adv_len);
int  uiox_bt_adv_stop      (uiox_bt_device_t *dev);
int  uiox_bt_connect       (uiox_bt_device_t *dev,
                             const uiox_bt_addr_t addr, bool is_ble);
int  uiox_bt_disconnect    (uiox_bt_device_t *dev, uint16_t handle);
int  uiox_bt_gatt_write    (uiox_bt_device_t *dev, uint16_t conn_handle,
                             uint16_t attr_handle,
                             const uint8_t *data, uint16_t len);
int  uiox_bt_acl_send      (uiox_bt_device_t *dev, uint16_t handle,
                             const uint8_t *data, uint16_t len);
int  uiox_bt_hci_cmd       (uiox_bt_device_t *dev, uint16_t opcode,
                             const uint8_t *params, uint8_t param_len,
                             uint8_t *resp, uint8_t resp_max,
                             uint32_t timeout_ms);
uiox_bt_remote_dev_t *uiox_bt_find_device(uiox_bt_device_t *dev,
                                           const uiox_bt_addr_t addr);
void uiox_bt_print_info    (const uiox_bt_device_t *dev);
void uiox_bt_print_stats   (uiox_bt_device_t *dev);
void uiox_bt_print_devices (const uiox_bt_device_t *dev);
const char *uiox_bt_state_name(uiox_bt_subsys_state_t s);
const char *uiox_bt_ev_name   (uiox_bt_ev_t ev);
const char *uiox_bt_ver_name  (uiox_bt_ver_t v);
#ifdef __cplusplus
}
#endif
#endif /* UIOX_BT_DEVICE_H */
