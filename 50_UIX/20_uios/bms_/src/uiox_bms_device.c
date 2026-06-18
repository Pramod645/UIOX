#include "uiox_bms_device.h"
#include <string.h>
#include <errno.h>
#include <stdio.h>

int uiox_bms_open(uiox_bms_device_t *dev, const uiox_bms_open_params_t *p)
{
    if (!dev||!p||!p->hw||!p->hw_ops) return -EINVAL;
    memset(dev, 0, sizeof(*dev));
    dev->hw = p->hw;
    int rc = uiox_bms_hw_init(p->hw, p->hw_ops);
    if (rc<0) return rc;
    rc = uiox_bms_subsys_init(&dev->subsys, p->hw, &p->batt);
    if (rc<0) return rc;
    if (p->evt_cb)
        uiox_bms_subsys_set_cb(&dev->subsys, p->evt_cb, p->evt_ctx);
    dev->open = true;
    return 0;
}
int  uiox_bms_start(uiox_bms_device_t *d)
{ if(!d||!d->open) return -EINVAL;
  return uiox_bms_subsys_start(&d->subsys); }
void uiox_bms_stop (uiox_bms_device_t *d)
{ if(!d||!d->open) return; uiox_bms_subsys_stop(&d->subsys); }
void uiox_bms_close(uiox_bms_device_t *d)
{ if(!d||!d->open) return; uiox_bms_stop(d);
  uiox_bms_hw_deinit(d->hw); d->open=false; }
void uiox_bms_tick (uiox_bms_device_t *d, uint32_t ms)
{ if(!d||!d->open) return; uiox_bms_subsys_tick(&d->subsys, ms); }

uint8_t  uiox_bms_soc(const uiox_bms_device_t *d)
{ return d&&d->open ? d->subsys.algo.soc_pct : 0u; }
uint8_t  uiox_bms_soh(const uiox_bms_device_t *d)
{ return d&&d->open ? d->subsys.algo.soh_pct : 0u; }
int32_t  uiox_bms_current(const uiox_bms_device_t *d)
{ return d&&d->open ? d->hw->current_ma : 0; }
uint32_t uiox_bms_pack_mv(const uiox_bms_device_t *d)
{ return d&&d->open ? d->hw->pack_mv : 0u; }
int32_t  uiox_bms_tte_min(const uiox_bms_device_t *d)
{ return d&&d->open ? d->subsys.algo.tte_min : -1; }
int32_t  uiox_bms_ttf_min(const uiox_bms_device_t *d)
{ return d&&d->open ? d->subsys.algo.ttf_min : -1; }
int32_t  uiox_bms_remain_mah(const uiox_bms_device_t *d)
{ return d&&d->open ? d->subsys.algo.remain_mah : 0; }
bool     uiox_bms_charging(const uiox_bms_device_t *d)
{ return d&&d->open ? d->subsys.algo.charging : false; }
bool     uiox_bms_present(const uiox_bms_device_t *d)
{ return d&&d->open ? d->hw->present : false; }
int  uiox_bms_set_chg_fet(uiox_bms_device_t *d, bool on)
{ if(!d||!d->open) return -EINVAL;
  return uiox_bms_hw_set_chg_fet(d->hw, on); }
int  uiox_bms_set_dsg_fet(uiox_bms_device_t *d, bool on)
{ if(!d||!d->open) return -EINVAL;
  return uiox_bms_hw_set_dsg_fet(d->hw, on); }
int  uiox_bms_get_telemetry(uiox_bms_device_t *d,
                             uiox_bms_telem_t *out, uint32_t ms)
{ if(!d||!d->open) return -EINVAL;
  return uiox_bms_if_telemetry(&d->subsys.bif, out, ms,
                                 d->subsys.algo.soc_pct,
                                 d->subsys.algo.soh_pct,
                                 d->subsys.algo.remain_mah,
                                 (int32_t)d->subsys.algo.batt.full_mah); }

void uiox_bms_print_info(const uiox_bms_device_t *d)
{
    if (!d) return;
    const uiox_bms_hw_t *hw = d->hw;
    printf("  BMS model      : %s\n",  hw->model);
    printf("  Cells          : %u series\n", hw->num_cells);
    printf("  Shunt          : %u µΩ\n", hw->shunt_uohm);
    printf("  Caps           : 0x%08X\n", hw->caps);
    printf("  Design cap     : %u mAh\n",
           d->subsys.algo.batt.design_mah);
    printf("  Full cap       : %u mAh\n",
           d->subsys.algo.batt.full_mah);
    printf("  State          : %s\n",
           uiox_bms_state_name(d->subsys.state));
}

void uiox_bms_print_stats(uiox_bms_device_t *d)
{
    if (!d) return;
    const uiox_bms_subsys_t *s = &d->subsys;
    printf("  SoC            : %u %%\n",  s->algo.soc_pct);
    printf("  SoH            : %u %%\n",  s->algo.soh_pct);
    printf("  Remaining      : %d mAh\n", s->algo.remain_mah);
    printf("  Pack voltage   : %u mV\n",  d->hw->pack_mv);
    printf("  Current        : %+d mA\n", d->hw->current_ma);
    printf("  Charging       : %s\n",     s->algo.charging?"YES":"NO");
    printf("  TTE            : ");
    if (s->algo.tte_min>=0) printf("%d min\n", s->algo.tte_min);
    else                     printf("N/A\n");
    printf("  TTF            : ");
    if (s->algo.ttf_min>=0) printf("%d min\n", s->algo.ttf_min);
    else                     printf("N/A\n");
    printf("  Temp[0]        : %.1f °C\n",
           (float)d->hw->temp_dc[0] / 10.0f);
    printf("  Balance mask   : 0x%04X  active=%s\n",
           s->bal.balance_mask,
           uiox_bms_bal_active(&s->bal)?"YES":"NO");
    printf("  Fault flags    : 0x%08X\n", d->hw->fault_flags);
    printf("  Uptime         : %llu ms\n",(unsigned long long)s->uptime_ms);
    printf("  Tick count     : %u\n",      s->tick_count);
    printf("  Events         : %u\n",      uiox_bms_event_count());
    for (uint8_t i=0; i<d->hw->num_cells; i++)
        printf("  Cell[%u]        : %u mV\n", i, d->hw->cell_mv[i]);
    uiox_bms_if_stats_t is;
    uiox_bms_if_stats_get(&d->subsys.bif, &is);
    printf("  Measurements   : %llu\n",(unsigned long long)is.measurements);
    printf("  IRQ count      : %u\n",  is.irq_count);
    printf("  Fault count    : %u\n",  is.fault_count);
    printf("  Balance ops    : %u\n",  is.balance_ops);
}

void uiox_bms_print_events(void)
{
    printf("  Event log (%u entries):\n", uiox_bms_event_count());
    uiox_bms_event_t ev;
    uint8_t n = uiox_bms_event_count();
    for (uint8_t i=0; i<n; i++) {
        if (!uiox_bms_event_pop(&ev)) break;
        printf("    [%2u] %-20s  soc=%3u%%  pack=%4u mV  I=%+5d mA\n",
               i, uiox_bms_ev_name(ev.type),
               ev.soc_pct, ev.pack_mv, ev.current_ma);
    }
}

const char *uiox_bms_state_name(uiox_bms_subsys_state_t s)
{
    switch(s){
    case UIOX_BMS_SUBSYS_STOPPED:     return "STOPPED";
    case UIOX_BMS_SUBSYS_IDLE:        return "IDLE";
    case UIOX_BMS_SUBSYS_CHARGING:    return "CHARGING";
    case UIOX_BMS_SUBSYS_DISCHARGING: return "DISCHARGING";
    case UIOX_BMS_SUBSYS_FAULT:       return "FAULT";
    case UIOX_BMS_SUBSYS_BALANCED:    return "BALANCED";
    default:                           return "UNKNOWN";
    }
}

const char *uiox_bms_ev_name(uiox_bms_ev_t ev)
{
    switch(ev){
    case UIOX_BMS_EV_PACK_INSERT:   return "PACK_INSERT";
    case UIOX_BMS_EV_PACK_REMOVE:   return "PACK_REMOVE";
    case UIOX_BMS_EV_CHG_START:     return "CHG_START";
    case UIOX_BMS_EV_CHG_STOP:      return "CHG_STOP";
    case UIOX_BMS_EV_DSG_START:     return "DSG_START";
    case UIOX_BMS_EV_DSG_STOP:      return "DSG_STOP";
    case UIOX_BMS_EV_FULL:          return "FULL";
    case UIOX_BMS_EV_EMPTY:         return "EMPTY";
    case UIOX_BMS_EV_OVP:           return "OVP";
    case UIOX_BMS_EV_UVP:           return "UVP";
    case UIOX_BMS_EV_OCP_CHG:       return "OCP_CHG";
    case UIOX_BMS_EV_OCP_DSG:       return "OCP_DSG";
    case UIOX_BMS_EV_SCP:           return "SCP";
    case UIOX_BMS_EV_OTP:           return "OTP";
    case UIOX_BMS_EV_UTP:           return "UTP";
    case UIOX_BMS_EV_CELL_IMBALANCE:return "CELL_IMBALANCE";
    case UIOX_BMS_EV_SOC_LOW:       return "SOC_LOW";
    case UIOX_BMS_EV_SOC_CRITICAL:  return "SOC_CRITICAL";
    case UIOX_BMS_EV_FAULT:         return "FAULT";
    default:                         return "UNKNOWN";
    }
}
