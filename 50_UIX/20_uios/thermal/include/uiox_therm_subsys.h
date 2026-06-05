/* uiox_therm_subsys.h */
#ifndef UIOX_THERM_SUBSYS_H
#define UIOX_THERM_SUBSYS_H
#include "uiox_therm_policy.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef void (*uiox_therm_evt_cb_t)(uiox_therm_ev_t ev,
                                     uint8_t sensor_id, void *ctx);
typedef enum {
    UIOX_THERM_SUBSYS_STOPPED = 0,
    UIOX_THERM_SUBSYS_RUNNING,
    UIOX_THERM_SUBSYS_ALERT,
    UIOX_THERM_SUBSYS_EMERGENCY,
} uiox_therm_subsys_state_t;
typedef struct {
    uiox_therm_if_t          tif;
    uiox_therm_sensor_mgr_t  smgr;
    uiox_therm_policy_t      policy;
    uiox_therm_subsys_state_t state;
    uiox_therm_evt_cb_t       evt_cb;
    void                     *evt_ctx;
    uint32_t                  tick_count;
    uint64_t                  uptime_ms;
    uint32_t                  meas_interval_ms;
    uint32_t                  last_meas_ms;
} uiox_therm_subsys_t;
int  uiox_therm_subsys_init  (uiox_therm_subsys_t *sys, uiox_therm_hw_t *hw);
int  uiox_therm_subsys_start (uiox_therm_subsys_t *sys);
void uiox_therm_subsys_stop  (uiox_therm_subsys_t *sys);
void uiox_therm_subsys_tick  (uiox_therm_subsys_t *sys, uint32_t now_ms);
void uiox_therm_subsys_set_cb(uiox_therm_subsys_t *sys,
                               uiox_therm_evt_cb_t cb, void *ctx);
#ifdef __cplusplus
}
#endif
#endif /* UIOX_THERM_SUBSYS_H */
