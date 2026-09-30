/* uiox_fan_subsys.h */
#ifndef UIOX_FAN_SUBSYS_H
#define UIOX_FAN_SUBSYS_H
#include "uiox_fan_thermal.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef void (*uiox_fan_evt_cb_t)(uiox_fan_ev_t evt,
                                   uint8_t fan_id, void *ctx);
typedef enum {
    UIOX_FAN_SUBSYS_STOPPED = 0,
    UIOX_FAN_SUBSYS_RUNNING,
    UIOX_FAN_SUBSYS_EMERGENCY,
    UIOX_FAN_SUBSYS_FAULT,
} uiox_fan_subsys_state_t;
typedef struct {
    uiox_fan_if_t           fif;
    uiox_fan_drv_t           drv;
    uiox_fan_thermal_t       thermal;
    uiox_fan_subsys_state_t  state;
    uiox_fan_evt_cb_t        evt_cb;
    void                    *evt_ctx;
    uint32_t                 tick_count;
    uint64_t                 uptime_ms;
    uint32_t                 meas_interval_ms;
    uint32_t                 last_meas_ms;
    uint32_t                 wdt_interval_ms;
    uint32_t                 last_wdt_ms;
} uiox_fan_subsys_t;
int  uiox_fan_subsys_init   (uiox_fan_subsys_t *sys, uiox_fan_hw_t *hw,
                              int16_t critical_temp_dc);
int  uiox_fan_subsys_start  (uiox_fan_subsys_t *sys);
void uiox_fan_subsys_stop   (uiox_fan_subsys_t *sys);
void uiox_fan_subsys_tick   (uiox_fan_subsys_t *sys, uint32_t now_ms);
void uiox_fan_subsys_set_cb (uiox_fan_subsys_t *sys,
                              uiox_fan_evt_cb_t cb, void *ctx);
#ifdef __cplusplus
}
#endif
#endif /* UIOX_FAN_SUBSYS_H */
