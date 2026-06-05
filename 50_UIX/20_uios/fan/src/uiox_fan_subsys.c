#include "uiox_fan_subsys.h"
#include <string.h>
#include <errno.h>

static void fire(uiox_fan_subsys_t *sys, uiox_fan_ev_t ev, uint8_t id)
{ if (sys->evt_cb) sys->evt_cb(ev, id, sys->evt_ctx); }

int uiox_fan_subsys_init(uiox_fan_subsys_t *sys, uiox_fan_hw_t *hw,
                          int16_t critical_temp_dc)
{
    if (!sys || !hw) return -EINVAL;
    memset(sys, 0, sizeof(*sys));
    int rc = uiox_fan_if_config(&sys->fif, hw);
    if (rc < 0) return rc;
    rc = uiox_fan_drv_init(&sys->drv, &sys->fif);
    if (rc < 0) return rc;
    rc = uiox_fan_thermal_init(&sys->thermal, &sys->drv, critical_temp_dc);
    if (rc < 0) return rc;
    sys->meas_interval_ms = 500u;
    sys->wdt_interval_ms  = 5000u;
    sys->state            = UIOX_FAN_SUBSYS_STOPPED;
    return 0;
}

int uiox_fan_subsys_start(uiox_fan_subsys_t *sys)
{
    if (!sys) return -EINVAL;
    int rc = uiox_fan_if_start(&sys->fif);
    if (rc < 0) return rc;
    sys->state = UIOX_FAN_SUBSYS_RUNNING;
    fire(sys, UIOX_FAN_EV_START, 0xFFu);
    return 0;
}

void uiox_fan_subsys_stop(uiox_fan_subsys_t *sys)
{
    if (!sys) return;
    uiox_fan_if_stop(&sys->fif);
    sys->state = UIOX_FAN_SUBSYS_STOPPED;
    fire(sys, UIOX_FAN_EV_STOP, 0xFFu);
}

void uiox_fan_subsys_tick(uiox_fan_subsys_t *sys, uint32_t now_ms)
{
    if (!sys || sys->state == UIOX_FAN_SUBSYS_STOPPED) return;
    sys->tick_count++;
    sys->uptime_ms += 10u;

    /* Fault / IRQ check */
    int faults = uiox_fan_if_irq_handle(&sys->fif, now_ms);
    if (faults > 0) {
        sys->state = (sys->thermal.emergency) ?
                     UIOX_FAN_SUBSYS_EMERGENCY :
                     UIOX_FAN_SUBSYS_FAULT;
    }

    /* Periodic measurement */
    if ((now_ms - sys->last_meas_ms) >= sys->meas_interval_ms) {
        sys->last_meas_ms = now_ms;
        uiox_fan_if_measure(&sys->fif);
    }

    /* Fan driver tick (stall detection, spin-up) */
    uiox_fan_drv_tick(&sys->drv, now_ms);

    /* Thermal control tick */
    uiox_fan_thermal_tick(&sys->thermal, now_ms);

    /* Watchdog kick */
    if ((now_ms - sys->last_wdt_ms) >= sys->wdt_interval_ms) {
        const uiox_fan_hw_ops_t *ops =
            (const uiox_fan_hw_ops_t *)sys->fif.hw->priv;
        if (ops && ops->wdt_kick) ops->wdt_kick(sys->fif.hw);
        sys->last_wdt_ms = now_ms;
    }

    /* Dispatch events to callback */
    uiox_fan_event_t ev;
    while (!uiox_fan_event_empty())
        if (uiox_fan_event_pop(&ev)) fire(sys, ev.type, ev.fan_id);
}

void uiox_fan_subsys_set_cb(uiox_fan_subsys_t *sys,
                              uiox_fan_evt_cb_t cb, void *ctx)
{ if (!sys) return; sys->evt_cb = cb; sys->evt_ctx = ctx; }
