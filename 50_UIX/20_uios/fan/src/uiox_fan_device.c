#include "uiox_fan_device.h"
#include <string.h>
#include <errno.h>
#include <stdio.h>

int uiox_fan_open(uiox_fan_device_t *dev, const uiox_fan_open_params_t *p)
{
    if (!dev||!p||!p->hw||!p->hw_ops) return -EINVAL;
    memset(dev, 0, sizeof(*dev));
    dev->hw = p->hw;
    int rc = uiox_fan_hw_init(p->hw, p->hw_ops);
    if (rc<0) return rc;
    rc = uiox_fan_subsys_init(&dev->subsys, p->hw, p->critical_temp_dc);
    if (rc<0) return rc;
    if (p->evt_cb)
        uiox_fan_subsys_set_cb(&dev->subsys, p->evt_cb, p->evt_ctx);
    dev->open = true;
    return 0;
}

int  uiox_fan_start(uiox_fan_device_t *d)
{ if(!d||!d->open) return -EINVAL;
  return uiox_fan_subsys_start(&d->subsys); }
void uiox_fan_stop(uiox_fan_device_t *d)
{ if(!d||!d->open) return; uiox_fan_subsys_stop(&d->subsys); }
void uiox_fan_close(uiox_fan_device_t *d)
{ if(!d||!d->open) return; uiox_fan_stop(d);
  uiox_fan_hw_deinit(d->hw); d->open=false; }
void uiox_fan_tick(uiox_fan_device_t *d, uint32_t ms)
{ if(!d||!d->open) return; uiox_fan_subsys_tick(&d->subsys, ms); }

int  uiox_fan_add_fan(uiox_fan_device_t *d, const uiox_fan_ch_t *ch)
{ if(!d||!d->open) return -EINVAL;
  return uiox_fan_drv_register(&d->subsys.drv, ch); }
int  uiox_fan_add_zone(uiox_fan_device_t *d, const uiox_fan_zone_t *z)
{ if(!d||!d->open) return -EINVAL;
  return uiox_fan_thermal_add_zone(&d->subsys.thermal, z); }
int  uiox_fan_set_duty(uiox_fan_device_t *d, uint8_t id,
                        uint8_t duty, uint32_t ms)
{ if(!d||!d->open) return -EINVAL;
  return uiox_fan_drv_set_duty(&d->subsys.drv, id, duty, ms); }
int  uiox_fan_set_pct(uiox_fan_device_t *d, uint8_t id,
                       uint8_t pct, uint32_t ms)
{ if(!d||!d->open) return -EINVAL;
  return uiox_fan_drv_set_pct(&d->subsys.drv, id, pct, ms); }
void uiox_fan_set_manual(uiox_fan_device_t *d, uint8_t id,
                          bool manual, uint8_t duty, uint32_t ms)
{ if(!d||!d->open) return;
  uiox_fan_drv_set_manual(&d->subsys.drv, id, manual, duty, ms); }

uint16_t uiox_fan_get_rpm(const uiox_fan_device_t *d, uint8_t id)
{ return d&&d->open ? d->hw->chan[id<UIOX_FAN_MAX_CHANNELS?id:0].rpm_measured:0u;}
uint8_t uiox_fan_get_pct(const uiox_fan_device_t *d, uint8_t id)
{ if(!d||!d->open) return 0u;
  return (uint8_t)(d->hw->chan[id<UIOX_FAN_MAX_CHANNELS?id:0].pwm_duty * 100u
                   / UIOX_FAN_PWM_MAX); }
int16_t uiox_fan_get_temp(const uiox_fan_device_t *d, uint8_t sid)
{ return d&&d->open ? d->hw->temp_dc[sid<UIOX_FAN_MAX_TEMP_SENSORS?sid:0]:0;}
bool uiox_fan_stalled(const uiox_fan_device_t *d, uint8_t id)
{ if(!d||!d->open) return false;
  const uiox_fan_ch_t *f = NULL;
  for(uint8_t i=0;i<d->subsys.drv.num_fans;i++)
      if(d->subsys.drv.fans[i].fan_id==id){f=&d->subsys.drv.fans[i];break;}
  return f ? f->stalled : false; }
int uiox_fan_get_telemetry(uiox_fan_device_t *d,
                            uiox_fan_telem_t *out, uint32_t ms)
{ if(!d||!d->open) return -EINVAL;
  return uiox_fan_if_telemetry(&d->subsys.fif, out, ms); }

void uiox_fan_print_info(const uiox_fan_device_t *d)
{
    if(!d) return;
    const uiox_fan_hw_t *hw = d->hw;
    printf("  Fan IC model   : %s\n",  hw->model);
    printf("  Bus            : %s\n",
           hw->bus==UIOX_FAN_BUS_I2C?"I2C":
           hw->bus==UIOX_FAN_BUS_SPI?"SPI":"GPIO");
    printf("  Fans           : %u\n",  hw->num_fans);
    printf("  Temp sensors   : %u\n",  hw->num_temps);
    printf("  PWM freq       : %u Hz\n",hw->pwm_freq_hz);
    printf("  Capabilities   : 0x%08X\n",hw->caps);
    printf("  Critical temp  : %.1f°C\n",
           (float)d->subsys.thermal.critical_temp_dc / 10.0f);
    printf("  State          : %s\n",
           uiox_fan_state_name(d->subsys.state));
}

void uiox_fan_print_stats(uiox_fan_device_t *d)
{
    if(!d) return;
    const uiox_fan_subsys_t *s = &d->subsys;
    printf("  Uptime         : %llu ms\n",(unsigned long long)s->uptime_ms);
    printf("  Tick count     : %u\n",     s->tick_count);
    printf("  Emergency      : %s\n",     s->thermal.emergency?"YES":"NO");
    for(uint8_t i=0;i<d->hw->num_fans;i++)
        printf("  Fan[%u]         : PWM=%3u%%  RPM=%5u  %s%s\n",
               i, uiox_fan_get_pct(d,i),
               uiox_fan_get_rpm(d,i),
               d->hw->chan[i].enabled?"ON ":"OFF",
               uiox_fan_stalled(d,i)?"  STALLED":"");
    for(uint8_t i=0;i<d->hw->num_temps;i++)
        printf("  Temp[%u]        : %.1f°C\n",
               i,(float)d->hw->temp_dc[i]/10.0f);
    uiox_fan_if_stats_t is;
    uiox_fan_if_stats_get(&d->subsys.fif, &is);
    printf("  Measurements   : %llu\n",(unsigned long long)is.measurements);
    printf("  IRQ count      : %u\n",  is.irq_count);
    printf("  Fault count    : %u\n",  is.fault_count);
    printf("  Stall count    : %u\n",  is.stall_count);
    printf("  PWM changes    : %u\n",  is.pwm_changes);
    printf("  Events in log  : %u\n",  uiox_fan_event_count());
}

void uiox_fan_print_events(void)
{
    printf("  Event log (%u entries):\n", uiox_fan_event_count());
    uiox_fan_event_t ev;
    uint8_t n = uiox_fan_event_count();
    for(uint8_t i=0;i<n;i++){
        if(!uiox_fan_event_pop(&ev)) break;
        printf("    [%2u] %-20s  fan=%u  pwm=%3u%%  rpm=%5u  "
               "temp=%.1f°C\n",
               i, uiox_fan_ev_name(ev.type), ev.fan_id,
               ev.pwm_duty*100u/255u, ev.rpm,
               (float)ev.temp_dc/10.0f);
    }
}

const char *uiox_fan_state_name(uiox_fan_subsys_state_t s)
{
    switch(s){
    case UIOX_FAN_SUBSYS_STOPPED:   return "STOPPED";
    case UIOX_FAN_SUBSYS_RUNNING:   return "RUNNING";
    case UIOX_FAN_SUBSYS_EMERGENCY: return "EMERGENCY";
    case UIOX_FAN_SUBSYS_FAULT:     return "FAULT";
    default:                         return "UNKNOWN";
    }
}

const char *uiox_fan_ev_name(uiox_fan_ev_t ev)
{
    switch(ev){
    case UIOX_FAN_EV_START:           return "START";
    case UIOX_FAN_EV_STOP:            return "STOP";
    case UIOX_FAN_EV_STALL:           return "STALL";
    case UIOX_FAN_EV_STALL_CLEAR:     return "STALL_CLEAR";
    case UIOX_FAN_EV_SPIN_UP_FAIL:    return "SPIN_UP_FAIL";
    case UIOX_FAN_EV_OVERHEAT:        return "OVERHEAT";
    case UIOX_FAN_EV_TEMP_OK:         return "TEMP_OK";
    case UIOX_FAN_EV_PWM_CHANGE:      return "PWM_CHANGE";
    case UIOX_FAN_EV_FAULT:           return "FAULT";
    case UIOX_FAN_EV_WATCHDOG:        return "WATCHDOG";
    case UIOX_FAN_EV_MANUAL_OVERRIDE: return "MANUAL_OVERRIDE";
    case UIOX_FAN_EV_AUTO_RESTORE:    return "AUTO_RESTORE";
    default:                           return "UNKNOWN";
    }
}
