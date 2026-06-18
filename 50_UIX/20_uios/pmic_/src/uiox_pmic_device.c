/* uiox_pmic_device.c */
#include "uiox_pmic_device.h"
#include <string.h>
#include <errno.h>
#include <stdio.h>

int uiox_pmic_open(uiox_pmic_device_t *dev, const uiox_pmic_open_params_t *p)
{
    if (!dev||!p||!p->hw||!p->hw_ops) return -EINVAL;
    memset(dev, 0, sizeof(*dev));
    dev->hw = p->hw;
    int rc = uiox_pmic_hw_init(p->hw, p->hw_ops);
    if (rc<0) return rc;
    rc = uiox_pmic_subsys_init(&dev->subsys, p->hw);
    if (rc<0) return rc;
    if (p->evt_cb)
        uiox_pmic_subsys_set_cb(&dev->subsys, p->evt_cb, p->evt_ctx);
    dev->open = true;
    return 0;
}
int  uiox_pmic_start(uiox_pmic_device_t *dev)
{ if(!dev||!dev->open) return -EINVAL;
  return uiox_pmic_subsys_start(&dev->subsys); }
void uiox_pmic_stop (uiox_pmic_device_t *dev)
{ if(!dev||!dev->open) return; uiox_pmic_subsys_stop(&dev->subsys); }
void uiox_pmic_close(uiox_pmic_device_t *dev)
{ if(!dev||!dev->open) return; uiox_pmic_stop(dev);
  uiox_pmic_hw_deinit(dev->hw); dev->open=false; }
void uiox_pmic_tick (uiox_pmic_device_t *dev, uint32_t now_ms)
{ if(!dev||!dev->open) return; uiox_pmic_subsys_tick(&dev->subsys,now_ms); }

int uiox_pmic_rail_add(uiox_pmic_device_t *dev, const uiox_pmic_rail_t *r)
{ if(!dev||!dev->open) return -EINVAL;
  return uiox_pmic_rail_register(&dev->subsys.mgr, r); }
int uiox_pmic_rail_on(uiox_pmic_device_t *dev, const char *name)
{ if(!dev||!dev->open) return -EINVAL;
  return uiox_pmic_rail_enable(&dev->subsys.mgr, name); }
int uiox_pmic_rail_off(uiox_pmic_device_t *dev, const char *name)
{ if(!dev||!dev->open) return -EINVAL;
  return uiox_pmic_rail_disable(&dev->subsys.mgr, name); }
int uiox_pmic_rail_voltage(uiox_pmic_device_t *dev,
                            const char *name, uint32_t mv)
{ if(!dev||!dev->open) return -EINVAL;
  return uiox_pmic_rail_set_mv(&dev->subsys.mgr, name, mv); }
int uiox_pmic_rail_read_mv(uiox_pmic_device_t *dev,
                            const char *name, uint32_t *mv_out)
{ if(!dev||!dev->open) return -EINVAL;
  return uiox_pmic_rail_get_mv(&dev->subsys.mgr, name, mv_out); }
int uiox_pmic_set_ps(uiox_pmic_device_t *dev, uiox_pmic_ps_t ps)
{ if(!dev||!dev->open) return -EINVAL;
  return uiox_pmic_policy_set_ps(&dev->subsys.policy, ps); }
int uiox_pmic_add_opp(uiox_pmic_device_t *dev,
                       uint32_t cpu_mhz, uint32_t vcore_mv, uint32_t pw)
{ if(!dev||!dev->open) return -EINVAL;
  return uiox_pmic_policy_add_opp(&dev->subsys.policy,cpu_mhz,vcore_mv,pw); }
void uiox_pmic_update_load(uiox_pmic_device_t *dev,
                            uint32_t load_pct, uint32_t now_ms)
{ if(!dev||!dev->open) return;
  uiox_pmic_policy_update_load(&dev->subsys.policy,load_pct,now_ms); }
int uiox_pmic_adc_read(uiox_pmic_device_t *dev,
                        uiox_pmic_adc_ch_t ch, uint32_t *result)
{ if(!dev||!dev->open) return -EINVAL;
  return uiox_pmic_hw_adc_read(dev->hw, ch, result); }
int uiox_pmic_get_telemetry(uiox_pmic_device_t *dev,
                             uiox_pmic_telem_t *out, uint32_t now_ms)
{ if(!dev||!dev->open) return -EINVAL;
  return uiox_pmic_if_telemetry(&dev->subsys.pif, out, now_ms); }
int uiox_pmic_wdt_kick(uiox_pmic_device_t *dev)
{ if(!dev||!dev->open) return -EINVAL;
  return uiox_pmic_hw_wdt_kick(dev->hw); }

void uiox_pmic_print_info(const uiox_pmic_device_t *dev)
{
    if (!dev) return;
    const uiox_pmic_hw_t *hw = dev->hw;
    printf("  PMIC model     : %s\n",  hw->model);
    printf("  Bus            : %s\n",
           hw->bus==UIOX_PMIC_BUS_I2C?"I2C":
           hw->bus==UIOX_PMIC_BUS_SPI?"SPI":"SPMI");
    printf("  I2C address    : 0x%02X\n", hw->i2c_addr);
    printf("  Capabilities   : 0x%08X\n", hw->caps);
    printf("  Bucks          : %u\n", hw->num_bucks);
    printf("  LDOs           : %u\n", hw->num_ldos);
    printf("  Die temp       : %d°C\n", hw->die_temp_c);
    printf("  Fault flags    : 0x%08X\n", hw->fault_flags);
    printf("  State          : %s\n",
           uiox_pmic_state_name(dev->subsys.state));
}

void uiox_pmic_print_stats(uiox_pmic_device_t *dev)
{
    if (!dev) return;
    const uiox_pmic_subsys_t *s = &dev->subsys;
    printf("  Uptime         : %llu ms\n",(unsigned long long)s->uptime_ms);
    printf("  Tick count     : %u\n", s->tick_count);
    printf("  Power state    : %s\n",
           uiox_pmic_ps_name(s->policy.current_ps));
    printf("  Current OPP    : [%u] %u MHz  %u mV  %u mW\n",
           s->policy.cur_opp,
           s->policy.num_opps ?
               s->policy.opps[s->policy.cur_opp].cpu_freq_mhz : 0u,
           s->policy.num_opps ?
               s->policy.opps[s->policy.cur_opp].vcore_mv : 0u,
           s->policy.num_opps ?
               s->policy.opps[s->policy.cur_opp].power_mw : 0u);
    printf("  CPU load       : %u %%\n", s->policy.cpu_load_pct);
    printf("  Throttled      : %s\n", s->policy.throttled?"YES":"NO");
    printf("  Events in log  : %u\n", uiox_pmic_event_count());
    uiox_pmic_if_stats_t ist;
    uiox_pmic_if_stats_get(&dev->subsys.pif, &ist);
    printf("  Reg reads      : %llu\n",(unsigned long long)ist.reg_reads);
    printf("  Reg writes     : %llu\n",(unsigned long long)ist.reg_writes);
    printf("  IRQ count      : %u\n", ist.irq_count);
    printf("  Fault count    : %u\n", ist.fault_count);
    printf("  WDT kicks      : %u\n", ist.wdt_kicks);
    /* Print rails */
    printf("  Rails:\n");
    for (uint8_t i = 0; i < dev->subsys.mgr.num_rails; i++) {
        const uiox_pmic_rail_t *r = &dev->subsys.mgr.rails[i];
        printf("    %-12s  %4u mV  %s  consumers=%u\n",
               r->name, r->cur_mv, r->enabled?"ON ":"OFF", r->consumers);
    }
}

void uiox_pmic_print_events(void)
{
    printf("  Event log (%u entries):\n", uiox_pmic_event_count());
    uiox_pmic_event_t ev;
    uint8_t n = uiox_pmic_event_count();
    for (uint8_t i = 0; i < n; i++) {
        if (!uiox_pmic_event_pop(&ev)) break;
        printf("    [%2u] %-16s  rail=0x%02X  mv=%4u  ts=%u ms\n",
               i, uiox_pmic_ev_name(ev.type),
               ev.rail_id, ev.mv, ev.ts_ms);
    }
}

const char *uiox_pmic_state_name(uiox_pmic_subsys_state_t s)
{
    switch(s){
    case UIOX_PMIC_SUBSYS_STOPPED: return "STOPPED";
    case UIOX_PMIC_SUBSYS_RUNNING: return "RUNNING";
    case UIOX_PMIC_SUBSYS_FAULT:   return "FAULT";
    case UIOX_PMIC_SUBSYS_SLEEP:   return "SLEEP";
    default:                        return "UNKNOWN";
    }
}

const char *uiox_pmic_ps_name(uiox_pmic_ps_t ps)
{
    switch(ps){
    case UIOX_PMIC_PS_ACTIVE:    return "ACTIVE";
    case UIOX_PMIC_PS_BALANCED:  return "BALANCED";
    case UIOX_PMIC_PS_POWERSAVE: return "POWERSAVE";
    case UIOX_PMIC_PS_SLEEP:     return "SLEEP";
    case UIOX_PMIC_PS_HIBERNATE: return "HIBERNATE";
    case UIOX_PMIC_PS_SHUTDOWN:  return "SHUTDOWN";
    default:                      return "UNKNOWN";
    }
}

const char *uiox_pmic_ev_name(uiox_pmic_ev_t ev)
{
    switch(ev){
    case UIOX_PMIC_EV_POWER_ON:   return "POWER_ON";
    case UIOX_PMIC_EV_POWER_OFF:  return "POWER_OFF";
    case UIOX_PMIC_EV_RAIL_ON:    return "RAIL_ON";
    case UIOX_PMIC_EV_RAIL_OFF:   return "RAIL_OFF";
    case UIOX_PMIC_EV_OTP:        return "OTP";
    case UIOX_PMIC_EV_OCP:        return "OCP";
    case UIOX_PMIC_EV_OVP:        return "OVP";
    case UIOX_PMIC_EV_UVP:        return "UVP";
    case UIOX_PMIC_EV_WDT:        return "WDT";
    case UIOX_PMIC_EV_PGOOD_LOST: return "PGOOD_LOST";
    case UIOX_PMIC_EV_DVFS_UP:    return "DVFS_UP";
    case UIOX_PMIC_EV_DVFS_DOWN:  return "DVFS_DOWN";
    case UIOX_PMIC_EV_SLEEP:      return "SLEEP";
    case UIOX_PMIC_EV_WAKE:       return "WAKE";
    case UIOX_PMIC_EV_FAULT:      return "FAULT";
    default:                       return "UNKNOWN";
    }
}
