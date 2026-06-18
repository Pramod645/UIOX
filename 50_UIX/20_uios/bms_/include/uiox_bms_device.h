/* ---- uiox_bms_device.h (combined for brevity) ---- */
#ifndef UIOX_BMS_DEVICE_H
#define UIOX_BMS_DEVICE_H
#include "uiox_bms_subsys.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
    uiox_bms_hw_t            *hw;
    const uiox_bms_hw_ops_t  *hw_ops;
    uiox_bms_batt_t           batt;
    uiox_bms_evt_cb_t         evt_cb;
    void                     *evt_ctx;
} uiox_bms_open_params_t;
typedef struct {
    uiox_bms_subsys_t  subsys;
    uiox_bms_hw_t     *hw;
    bool               open;
} uiox_bms_device_t;

int  uiox_bms_open        (uiox_bms_device_t           *dev,
                            const uiox_bms_open_params_t *p);
int  uiox_bms_start       (uiox_bms_device_t *dev);
void uiox_bms_stop        (uiox_bms_device_t *dev);
void uiox_bms_close       (uiox_bms_device_t *dev);
void uiox_bms_tick        (uiox_bms_device_t *dev, uint32_t now_ms);
uint8_t  uiox_bms_soc     (const uiox_bms_device_t *dev);
uint8_t  uiox_bms_soh     (const uiox_bms_device_t *dev);
int32_t  uiox_bms_current (const uiox_bms_device_t *dev);
uint32_t uiox_bms_pack_mv (const uiox_bms_device_t *dev);
int32_t  uiox_bms_tte_min (const uiox_bms_device_t *dev);
int32_t  uiox_bms_ttf_min (const uiox_bms_device_t *dev);
int32_t  uiox_bms_remain_mah(const uiox_bms_device_t *dev);
bool     uiox_bms_charging (const uiox_bms_device_t *dev);
bool     uiox_bms_present  (const uiox_bms_device_t *dev);
int  uiox_bms_set_chg_fet  (uiox_bms_device_t *dev, bool on);
int  uiox_bms_set_dsg_fet  (uiox_bms_device_t *dev, bool on);
int  uiox_bms_get_telemetry(uiox_bms_device_t  *dev,
                             uiox_bms_telem_t   *out, uint32_t now_ms);
void uiox_bms_print_info   (const uiox_bms_device_t *dev);
void uiox_bms_print_stats  (uiox_bms_device_t *dev);
void uiox_bms_print_events (void);
const char *uiox_bms_state_name(uiox_bms_subsys_state_t s);
const char *uiox_bms_ev_name   (uiox_bms_ev_t ev);
#ifdef __cplusplus
}
#endif
#endif /* UIOX_BMS_DEVICE_H */
