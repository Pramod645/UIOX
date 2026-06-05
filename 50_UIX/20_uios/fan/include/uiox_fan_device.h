/* uiox_fan_device.h */
#ifndef UIOX_FAN_DEVICE_H
#define UIOX_FAN_DEVICE_H
#include "uiox_fan_subsys.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
    uiox_fan_hw_t            *hw;
    const uiox_fan_hw_ops_t  *hw_ops;
    int16_t                   critical_temp_dc;
    uiox_fan_evt_cb_t         evt_cb;
    void                     *evt_ctx;
} uiox_fan_open_params_t;
typedef struct {
    uiox_fan_subsys_t  subsys;
    uiox_fan_hw_t     *hw;
    bool               open;
} uiox_fan_device_t;

int  uiox_fan_open        (uiox_fan_device_t           *dev,
                            const uiox_fan_open_params_t *p);
int  uiox_fan_start       (uiox_fan_device_t *dev);
void uiox_fan_stop        (uiox_fan_device_t *dev);
void uiox_fan_close       (uiox_fan_device_t *dev);
void uiox_fan_tick        (uiox_fan_device_t *dev, uint32_t now_ms);

int  uiox_fan_add_fan     (uiox_fan_device_t *dev, const uiox_fan_ch_t *ch);
int  uiox_fan_add_zone    (uiox_fan_device_t *dev,
                            const uiox_fan_zone_t *zone);
int  uiox_fan_set_duty    (uiox_fan_device_t *dev, uint8_t fan_id,
                            uint8_t duty, uint32_t now_ms);
int  uiox_fan_set_pct     (uiox_fan_device_t *dev, uint8_t fan_id,
                            uint8_t pct, uint32_t now_ms);
void uiox_fan_set_manual  (uiox_fan_device_t *dev, uint8_t fan_id,
                            bool manual, uint8_t duty, uint32_t now_ms);
uint16_t uiox_fan_get_rpm (const uiox_fan_device_t *dev, uint8_t fan_id);
uint8_t  uiox_fan_get_pct (const uiox_fan_device_t *dev, uint8_t fan_id);
int16_t  uiox_fan_get_temp(const uiox_fan_device_t *dev, uint8_t sensor_id);
bool     uiox_fan_stalled (const uiox_fan_device_t *dev, uint8_t fan_id);
int  uiox_fan_get_telemetry(uiox_fan_device_t *dev,
                             uiox_fan_telem_t *out, uint32_t now_ms);
void uiox_fan_print_info  (const uiox_fan_device_t *dev);
void uiox_fan_print_stats (uiox_fan_device_t *dev);
void uiox_fan_print_events(void);
const char *uiox_fan_state_name(uiox_fan_subsys_state_t s);
const char *uiox_fan_ev_name   (uiox_fan_ev_t ev);
#ifdef __cplusplus
}
#endif
#endif /* UIOX_FAN_DEVICE_H */
