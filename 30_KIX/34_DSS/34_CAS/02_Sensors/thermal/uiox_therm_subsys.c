/* uiox_therm_subsys.c */
#include "uiox_therm_subsys.h"
#include <string.h>
#include <errno.h>

static void fire(uiox_therm_subsys_t *s, uiox_therm_ev_t ev, uint8_t id)
{ if (s->evt_cb) s->evt_cb(ev, id, s->evt_ctx); }

int uiox_therm_subsys_init(uiox_therm_subsys_t *sys, uiox_therm_hw_t *hw)
{
    if (!sys || !hw) return -EINVAL;
    memset(sys, 0, sizeof(*sys));
    int rc = uiox_therm_if_config(&sys->tif, hw);
    if (rc < 0) return rc;
    rc = uiox_therm_sensor_init(&sys->smgr, &sys->tif);
    if (rc < 0) return rc;
    rc = uiox_therm_policy_init(&sys->policy, &sys->smgr);
    if (rc < 0) return rc;
    sys->meas_interval_ms = 1000u;
    sys->state            = UIOX_THERM_SUBSYS_STOPPED;
    return 0;
}

int uiox_therm_subsys_start(uiox_therm_subsys_t *sys)
{
    if (!sys) return -EINVAL;
    int rc = uiox_therm_if_start(&sys->tif);
    if (rc < 0) return rc;
    sys->state = UIOX_THERM_SUBSYS_RUNNING;
    return 0;
}

void uiox_therm_subsys_stop(uiox_therm_subsys_t *sys)
{
    if (!sys) return;
    uiox_therm_if_stop(&sys->tif);
    sys->state = UIOX_THERM_SUBSYS_STOPPED;
}

void uiox_therm_subsys_tick(uiox_therm_subsys_t *sys, uint32_t now_ms)
{
    if (!sys || sys->state == UIOX_THERM_SUBSYS_STOPPED) return;
    sys->tick_count++;
    sys->uptime_ms += 10u;

    /* IRQ / alert check */
    if (sys->tif.hw->alert_pending) {
        uiox_therm_if_irq_handle(&sys->tif, now_ms);
        sys->state = UIOX_THERM_SUBSYS_ALERT;
    }

    /* Periodic measurement */
    if ((now_ms - sys->last_meas_ms) >= sys->meas_interval_ms) {
        sys->last_meas_ms = now_ms;
        uiox_therm_if_measure(&sys->tif, now_ms);
        uiox_therm_sensor_update(&sys->smgr, now_ms);
    }

    /* Policy tick */
    uiox_therm_policy_tick(&sys->policy, now_ms);

    /* Update state */
    if (sys->policy.emergency)
        sys->state = UIOX_THERM_SUBSYS_EMERGENCY;
    else if (sys->policy.throttled)
        sys->state = UIOX_THERM_SUBSYS_ALERT;
    else
        sys->state = UIOX_THERM_SUBSYS_RUNNING;

    /* Dispatch events */
    uiox_therm_event_t ev;
    while (!uiox_therm_event_empty())
        if (uiox_therm_event_pop(&ev))
            fire(sys, ev.type, ev.sensor_id);
}

void uiox_therm_subsys_set_cb(uiox_therm_subsys_t *sys,
                               uiox_therm_evt_cb_t cb, void *ctx)
{ if (!sys) return; sys->evt_cb = cb; sys->evt_ctx = ctx; }
