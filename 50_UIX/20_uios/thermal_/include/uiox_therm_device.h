/* uiox_therm_device.h */
#ifndef UIOX_THERM_DEVICE_H
#define UIOX_THERM_DEVICE_H
#include "uiox_therm_subsys.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
    uiox_therm_hw_t              *hw;
    const uiox_therm_hw_ops_t   *hw_ops;
    int16_t                       t_high_dc;
    int16_t                       t_hyst_dc;
    int16_t                       t_crit_dc;
    uint32_t                      meas_interval_ms;
    uiox_therm_evt_cb_t           evt_cb;
    void                         *evt_ctx;
} uiox_therm_open_params_t;
typedef struct {
    uiox_therm_subsys_t  subsys;
    uiox_therm_hw_t     *hw;
    bool                 open;
} uiox_therm_device_t;

int      uiox_therm_open          (uiox_therm_device_t           *dev,
                                    const uiox_therm_open_params_t *p);
int      uiox_therm_start         (uiox_therm_device_t *dev);
void     uiox_therm_stop          (uiox_therm_device_t *dev);
void     uiox_therm_close         (uiox_therm_device_t *dev);
void     uiox_therm_tick          (uiox_therm_device_t *dev, uint32_t now_ms);
int      uiox_therm_add_sensor    (uiox_therm_device_t        *dev,
                                    const uiox_therm_sensor_t  *s);
int      uiox_therm_add_zone      (uiox_therm_device_t       *dev,
                                    const uiox_therm_zone_t   *z);
int      uiox_therm_set_alert     (uiox_therm_device_t *dev,
                                    int16_t t_high_dc, int16_t t_hyst_dc);
int16_t  uiox_therm_read          (uiox_therm_device_t *dev,
                                    const char *sensor_name);
int16_t  uiox_therm_read_ch       (const uiox_therm_device_t *dev,
                                    uint8_t ch);
bool     uiox_therm_alert_active  (const uiox_therm_device_t *dev,
                                    uint8_t ch);
bool     uiox_therm_throttled     (const uiox_therm_device_t *dev);
bool     uiox_therm_emergency     (const uiox_therm_device_t *dev);
int      uiox_therm_get_telemetry (uiox_therm_device_t *dev,
                                    uiox_therm_telem_t  *out,
                                    uint32_t now_ms);
void     uiox_therm_print_info    (const uiox_therm_device_t *dev);
void     uiox_therm_print_stats   (uiox_therm_device_t *dev);
void     uiox_therm_print_events  (void);
const char *uiox_therm_state_name (uiox_therm_subsys_state_t s);
const char *uiox_therm_ev_name    (uiox_therm_ev_t ev);
const char *uiox_therm_type_name  (uiox_therm_type_t t);
#ifdef __cplusplus
}
#endif
#endif /* UIOX_THERM_DEVICE_H */
