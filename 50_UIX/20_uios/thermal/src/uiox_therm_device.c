/* uiox_therm_device.c */
#include "uiox_therm_device.h"
#include <string.h>
#include <errno.h>
#include <stdio.h>

int uiox_therm_open(uiox_therm_device_t *dev,
                     const uiox_therm_open_params_t *p)
{
    if (!dev||!p||!p->hw||!p->hw_ops) return -EINVAL;
    memset(dev, 0, sizeof(*dev));
    dev->hw = p->hw;
    p->hw->t_high_dc = p->t_high_dc;
    p->hw->t_hyst_dc = p->t_hyst_dc;
    p->hw->t_crit_dc = p->t_crit_dc;
    int rc = uiox_therm_hw_init(p->hw, p->hw_ops);
    if (rc<0) return rc;
    rc = uiox_therm_subsys_init(&dev->subsys, p->hw);
    if (rc<0) return rc;
    dev->subsys.meas_interval_ms =
        p->meas_interval_ms ? p->meas_interval_ms : 1000u;
    if (p->evt_cb)
        uiox_therm_subsys_set_cb(&dev->subsys, p->evt_cb, p->evt_ctx);
    dev->open = true;
    return 0;
}
int  uiox_therm_start(uiox_therm_device_t *d)
{ if(!d||!d->open) return -EINVAL;
  return uiox_therm_subsys_start(&d->subsys); }
void uiox_therm_stop(uiox_therm_device_t *d)
{ if(!d||!d->open) return; uiox_therm_subsys_stop(&d->subsys); }
void uiox_therm_close(uiox_therm_device_t *d)
{ if(!d||!d->open) return; uiox_therm_stop(d);
  uiox_therm_hw_deinit(d->hw); d->open=false; }
void uiox_therm_tick(uiox_therm_device_t *d, uint32_t ms)
{ if(!d||!d->open) return; uiox_therm_subsys_tick(&d->subsys, ms); }
int  uiox_therm_add_sensor(uiox_therm_device_t *d,
                             const uiox_therm_sensor_t *s)
{ if(!d||!d->open) return -EINVAL;
  return uiox_therm_sensor_register(&d->subsys.smgr, s); }
int  uiox_therm_add_zone(uiox_therm_device_t *d,
                          const uiox_therm_zone_t *z)
{ if(!d||!d->open) return -EINVAL;
  return uiox_therm_policy_add_zone(&d->subsys.policy, z); }
int  uiox_therm_set_alert(uiox_therm_device_t *d,
                           int16_t hi, int16_t hyst)
{ if(!d||!d->open) return -EINVAL;
  return uiox_therm_if_set_alert(&d->subsys.tif, hi, hyst); }
int16_t uiox_therm_read(uiox_therm_device_t *d, const char *name)
{ if(!d||!d->open) return INT16_MIN;
  return uiox_therm_sensor_get(&d->subsys.smgr, name); }
int16_t uiox_therm_read_ch(const uiox_therm_device_t *d, uint8_t ch)
{ return d&&d->open && ch<UIOX_THERM_MAX_CHANNELS ?
         d->hw->meas[ch].temp_dc : INT16_MIN; }
bool uiox_therm_alert_active(const uiox_therm_device_t *d, uint8_t ch)
{ return d&&d->open && ch<UIOX_THERM_MAX_CHANNELS ?
         d->hw->meas[ch].alert_active : false; }
bool uiox_therm_throttled(const uiox_therm_device_t *d)
{ return d&&d->open ? d->subsys.policy.throttled : false; }
bool uiox_therm_emergency(const uiox_therm_device_t *d)
{ return d&&d->open ? d->subsys.policy.emergency : false; }
int  uiox_therm_get_telemetry(uiox_therm_device_t *d,
                               uiox_therm_telem_t *out, uint32_t ms)
{ if(!d||!d->open) return -EINVAL;
  return uiox_therm_if_telemetry(&d->subsys.tif, out, ms); }

void uiox_therm_print_info(const uiox_therm_device_t *d)
{
    if(!d) return;
    const uiox_therm_hw_t *hw = d->hw;
    printf("  Model          : %s\n", hw->model);
    printf("  Type           : %s\n", uiox_therm_type_name(hw->type));
    printf("  Channels       : %u\n", hw->num_channels);
    printf("  Resolution     : %s\n",
           hw->resolution==UIOX_THERM_RES_16BIT?"16-bit":
           hw->resolution==UIOX_THERM_RES_12BIT?"12-bit":"9-bit");
    printf("  T_high         : %.1f°C\n", (float)hw->t_high_dc/10.0f);
    printf("  T_hyst         : %.1f°C\n", (float)hw->t_hyst_dc/10.0f);
    printf("  T_crit         : %.1f°C\n", (float)hw->t_crit_dc/10.0f);
    printf("  Capabilities   : 0x%08X\n", hw->caps);
    printf("  Sensors        : %u\n", d->subsys.smgr.num_sensors);
    printf("  Zones          : %u\n", d->subsys.policy.num_zones);
    printf("  State          : %s\n",
           uiox_therm_state_name(d->subsys.state));
}

void uiox_therm_print_stats(uiox_therm_device_t *d)
{
    if(!d) return;
    const uiox_therm_subsys_t *s = &d->subsys;
    printf("  Uptime         : %llu ms\n",(unsigned long long)s->uptime_ms);
    printf("  Tick count     : %u\n", s->tick_count);
    printf("  Throttled      : %s\n", s->policy.throttled?"YES":"NO");
    printf("  Emergency      : %s\n", s->policy.emergency?"YES":"NO");
    printf("  Throttle events: %u\n", s->policy.throttle_count);
    printf("  Emergency evts : %u\n", s->policy.emergency_count);
    for (uint8_t i=0; i<d->subsys.smgr.num_sensors; i++) {
        const uiox_therm_sensor_t *sen = &d->subsys.smgr.sensors[i];
        printf("  %-12s   cur=%.1f°C  avg=%.1f°C  %s\n",
               sen->name,
               (float)sen->cur_dc/10.0f,
               (float)sen->avg_dc/10.0f,
               sen->error?"ERROR":"ok");
    }
    uiox_therm_if_stats_t ist;
    uiox_therm_if_stats_get(&d->subsys.tif, &ist);
    printf("  Measurements   : %llu\n",(unsigned long long)ist.measurements);
    printf("  IRQ count      : %u\n",  ist.irq_count);
    printf("  Alert count    : %u\n",  ist.alert_count);
    printf("  Error count    : %u\n",  ist.error_count);
    printf("  Events in log  : %u\n",  uiox_therm_event_count());
}

void uiox_therm_print_events(void)
{
    printf("  Event log (%u entries):\n", uiox_therm_event_count());
    uiox_therm_event_t ev;
    uint8_t n = uiox_therm_event_count();
    for (uint8_t i=0; i<n; i++) {
        if (!uiox_therm_event_pop(&ev)) break;
        printf("    [%2u] %-22s  sensor=%u zone=%u  "
               "temp=%.1f°C  thresh=%.1f°C\n",
               i, uiox_therm_ev_name(ev.type),
               ev.sensor_id, ev.zone_id,
               (float)ev.temp_dc/10.0f,
               (float)ev.threshold_dc/10.0f);
    }
}

const char *uiox_therm_state_name(uiox_therm_subsys_state_t s)
{
    switch(s){
    case UIOX_THERM_SUBSYS_STOPPED:   return "STOPPED";
    case UIOX_THERM_SUBSYS_RUNNING:   return "RUNNING";
    case UIOX_THERM_SUBSYS_ALERT:     return "ALERT";
    case UIOX_THERM_SUBSYS_EMERGENCY: return "EMERGENCY";
    default:                           return "UNKNOWN";
    }
}

const char *uiox_therm_ev_name(uiox_therm_ev_t ev)
{
    switch(ev){
    case UIOX_THERM_EV_ALERT_HIGH:     return "ALERT_HIGH";
    case UIOX_THERM_EV_ALERT_CLEAR:    return "ALERT_CLEAR";
    case UIOX_THERM_EV_CRITICAL:       return "CRITICAL";
    case UIOX_THERM_EV_TRIP_CROSSED:   return "TRIP_CROSSED";
    case UIOX_THERM_EV_TRIP_CLEARED:   return "TRIP_CLEARED";
    case UIOX_THERM_EV_THROTTLE_ON:    return "THROTTLE_ON";
    case UIOX_THERM_EV_THROTTLE_OFF:   return "THROTTLE_OFF";
    case UIOX_THERM_EV_SENSOR_ERROR:   return "SENSOR_ERROR";
    case UIOX_THERM_EV_SENSOR_RECOVER: return "SENSOR_RECOVER";
    case UIOX_THERM_EV_ZONE_HOT:       return "ZONE_HOT";
    case UIOX_THERM_EV_ZONE_COOL:      return "ZONE_COOL";
    default:                            return "UNKNOWN";
    }
}

const char *uiox_therm_type_name(uiox_therm_type_t t)
{
    switch(t){
    case UIOX_THERM_TYPE_PCT2075:  return "PCT2075";
    case UIOX_THERM_TYPE_LM75:     return "LM75";
    case UIOX_THERM_TYPE_TMP117:   return "TMP117";
    case UIOX_THERM_TYPE_TMP112:   return "TMP112";
    case UIOX_THERM_TYPE_MAX31875: return "MAX31875";
    case UIOX_THERM_TYPE_ADT7461:  return "ADT7461";
    case UIOX_THERM_TYPE_NTC:      return "NTC";
    case UIOX_THERM_TYPE_INTERNAL: return "INTERNAL";
    default:                        return "CUSTOM";
    }
}
