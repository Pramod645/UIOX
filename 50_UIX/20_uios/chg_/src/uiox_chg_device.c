/**
 * @file  uiox_chg_device.c
 * @brief UIOX Charger application device API.
 * @date  2026-06-11
 */

 #include "uiox_chg_device.h"
 #include <string.h>
 #include <errno.h>
 #include <stdio.h>
 
 int uiox_chg_open(uiox_chg_device_t *dev, const uiox_chg_open_params_t *p)
 {
     if (!dev || !p || !p->hw || !p->hw_ops || !p->profile) return -EINVAL;
     memset(dev, 0, sizeof(*dev));
     dev->hw = p->hw;
     int rc  = uiox_chg_hw_init(p->hw, p->hw_ops);
     if (rc < 0) return rc;
     rc = uiox_chg_subsys_init(&dev->subsys, p->hw, p->profile);
     if (rc < 0) return rc;
     if (p->evt_cb)
         uiox_chg_subsys_set_cb(&dev->subsys, p->evt_cb, p->evt_ctx);
     dev->open = true;
     return 0;
 }
 
 int  uiox_chg_start(uiox_chg_device_t *dev)
 { if (!dev || !dev->open) return -EINVAL;
   return uiox_chg_subsys_start(&dev->subsys); }
 
 void uiox_chg_stop(uiox_chg_device_t *dev)
 { if (!dev || !dev->open) return;
   uiox_chg_subsys_stop(&dev->subsys); }
 
 void uiox_chg_close(uiox_chg_device_t *dev)
 { if (!dev || !dev->open) return;
   uiox_chg_stop(dev);
   uiox_chg_hw_deinit(dev->hw);
   dev->open = false; }
 
 void uiox_chg_tick(uiox_chg_device_t *dev, uint32_t now_ms)
 { if (!dev || !dev->open) return;
   uiox_chg_subsys_tick(&dev->subsys, now_ms); }
 
 int uiox_chg_enable(uiox_chg_device_t *dev, bool en)
 { if (!dev || !dev->open) return -EINVAL;
   return uiox_chg_subsys_enable(&dev->subsys, en); }
 
 int uiox_chg_otg_enable(uiox_chg_device_t *dev, bool en)
 { if (!dev || !dev->open) return -EINVAL;
   return uiox_chg_subsys_otg(&dev->subsys, en); }
 
 int uiox_chg_set_profile(uiox_chg_device_t *dev,
                           const uiox_chg_profile_t *p)
 { if (!dev || !dev->open) return -EINVAL;
   return uiox_chg_subsys_set_profile(&dev->subsys, p); }
 
 int uiox_chg_get_adc(uiox_chg_device_t *dev,
                       uiox_chg_adc_ch_t ch, int32_t *val_mv)
 { if (!dev || !dev->open || !val_mv) return -EINVAL;
   return uiox_chg_hw_adc_read(dev->hw, ch, val_mv); }
 
 int uiox_chg_get_status(uiox_chg_device_t *dev,
                          uiox_chg_chrg_t *chrg,
                          uiox_chg_src_t  *src,
                          uint32_t        *faults)
 { if (!dev || !dev->open) return -EINVAL;
   return uiox_chg_hw_get_status(dev->hw, chrg, src, faults); }
 
 void uiox_chg_print_info(const uiox_chg_device_t *dev)
 {
     if (!dev) return;
     const uiox_chg_hw_t    *hw  = dev->hw;
     const uiox_chg_subsys_t *s  = &dev->subsys;
     const uiox_chg_policy_t *pol = &s->policy;
     printf("  Model          : %s\n", hw->model);
     printf("  IC type        : %s\n", uiox_chg_ic_name(hw->ic_type));
     printf("  I2C addr       : 0x%02X  bus=%u\n",
            hw->i2c_addr, hw->i2c_bus);
     printf("  IRQ            : %u\n",   hw->irq);
     printf("  Capabilities   : 0x%08X\n", hw->caps);
     printf("  State          : %s\n",   uiox_chg_state_name(s->state));
     printf("  Source         : %s\n",   uiox_chg_src_name(hw->src));
     printf("  Charge state   : %s\n",   uiox_chg_chrg_name(hw->chrg_state));
     printf("  VBUS max       : %u mV\n", hw->vbus_max_mv);
     printf("  VBAT max       : %u mV\n", hw->vbat_max_mv);
     printf("  IBAT max       : %u mA\n", hw->ibat_max_ma);
     printf("  IIN max        : %u mA\n", hw->iin_max_ma);
     printf("  PD contract    : %s  %u mV / %u mA\n",
            uiox_chg_policy_pd_active(pol) ? "YES" : "NO",
            pol->contracted_mv, pol->contracted_ma);
     printf("  Throttled      : %s\n", s->throttled ? "YES" : "NO");
     printf("  OTG active     : %s\n", s->otg_active ? "YES" : "NO");
 }
 
 void uiox_chg_print_stats(uiox_chg_device_t *dev)
 {
     if (!dev) return;
     const uiox_chg_subsys_t *s = &dev->subsys;
     printf("  Uptime         : %llu ms\n", (unsigned long long)s->uptime_ms);
     printf("  Tick count     : %u\n",   s->tick_count);
     printf("  Charge cycles  : %u\n",   s->charge_cycles);
     printf("  Fault count    : %u\n",   s->fault_count);
     printf("  PD contracts   : %u\n",   s->pd_contracts);
     uiox_chg_if_stats_t is;
     uiox_chg_if_stats_get(&dev->subsys.cif, &is);
     printf("  Plug events    : %llu\n", (unsigned long long)is.plug_events);
     printf("  Fault events   : %llu\n", (unsigned long long)is.fault_events);
     printf("  PD TX msgs     : %llu\n", (unsigned long long)is.pd_msgs_tx);
     printf("  PD RX msgs     : %llu\n", (unsigned long long)is.pd_msgs_rx);
     printf("  ADC reads      : %u\n",   is.adc_reads);
     printf("  Wdog kicks     : %u\n",   is.wdog_kicks);
     printf("  IRQ count      : %u\n",   is.irq_count);
     printf("  Errors         : %u\n",   is.errors);
     printf("  Evt pool free  : %u / %u\n",
            uiox_chg_evt_free_cnt(),   UIOX_CHG_EVT_POOL_SIZE);
     printf("  Fault pool free: %u / %u\n",
            uiox_chg_fault_free_cnt(), UIOX_CHG_FAULT_POOL_SIZE);
     /* ADC snapshot */
     static const char *adc_names[] = {
         "VBUS","VBAT","IBAT","VSYS","TDIE","NTC"
     };
     for (int i = 0; i < (int)UIOX_CHG_ADC_MAX; i++)
         printf("  ADC %-6s     : %d\n",
                adc_names[i], dev->hw->adc_mv[i]);
 }
 
 const char *uiox_chg_state_name(uiox_chg_state_t s)
 {
     switch(s){
     case UIOX_CHG_STATE_OFF:   return "OFF";
     case UIOX_CHG_STATE_INIT:  return "INIT";
     case UIOX_CHG_STATE_READY: return "READY";
     case UIOX_CHG_STATE_FAULT: return "FAULT";
     case UIOX_CHG_STATE_ERROR: return "ERROR";
     default:                    return "UNKNOWN";
     }
 }
 
 const char *uiox_chg_ev_name(uiox_chg_ev_t ev)
 {
     switch(ev){
     case UIOX_CHG_EV_PLUG_IN:           return "PLUG_IN";
     case UIOX_CHG_EV_PLUG_OUT:          return "PLUG_OUT";
     case UIOX_CHG_EV_CHRG_START:        return "CHRG_START";
     case UIOX_CHG_EV_CHRG_DONE:         return "CHRG_DONE";
     case UIOX_CHG_EV_FAULT:             return "FAULT";
     case UIOX_CHG_EV_FAULT_CLEAR:       return "FAULT_CLEAR";
     case UIOX_CHG_EV_PD_CONTRACT:       return "PD_CONTRACT";
     case UIOX_CHG_EV_PD_RESET:          return "PD_RESET";
     case UIOX_CHG_EV_OTG_ON:            return "OTG_ON";
     case UIOX_CHG_EV_OTG_OFF:           return "OTG_OFF";
     case UIOX_CHG_EV_THERMAL_THROTTLE:  return "THERMAL_THROTTLE";
     case UIOX_CHG_EV_THERMAL_RESUME:    return "THERMAL_RESUME";
     case UIOX_CHG_EV_ERROR:             return "ERROR";
     default:                             return "UNKNOWN";
     }
 }
 
 const char *uiox_chg_src_name(uiox_chg_src_t src)
 {
     switch(src){
     case UIOX_CHG_SRC_NONE:     return "None";
     case UIOX_CHG_SRC_USBC_PD:  return "USB-C PD";
     case UIOX_CHG_SRC_USBC_STD: return "USB-C 5V";
     case UIOX_CHG_SRC_BARREL:   return "Barrel Jack";
     case UIOX_CHG_SRC_USB_SDP:  return "USB SDP";
     case UIOX_CHG_SRC_USB_CDP:  return "USB CDP";
     case UIOX_CHG_SRC_USB_DCP:  return "USB DCP";
     default:                     return "Unknown";
     }
 }
 
 const char *uiox_chg_chrg_name(uiox_chg_chrg_t c)
 {
     switch(c){
     case UIOX_CHG_CHRG_IDLE:      return "IDLE";
     case UIOX_CHG_CHRG_PRECHARGE: return "PRE-CHARGE";
     case UIOX_CHG_CHRG_FAST:      return "FAST-CHARGE";
     case UIOX_CHG_CHRG_TAPER:     return "TAPER";
     case UIOX_CHG_CHRG_DONE:      return "DONE";
     case UIOX_CHG_CHRG_FAULT:     return "FAULT";
     default:                       return "Unknown";
     }
 }
 
 const char *uiox_chg_ic_name(uiox_chg_ic_t ic)
 {
     switch(ic){
     case UIOX_CHG_IC_BQ25895:        return "TI BQ25895";
     case UIOX_CHG_IC_FUSB302:        return "ONSEMI FUSB302";
     case UIOX_CHG_IC_MAX77958:       return "Maxim MAX77958";
     case UIOX_CHG_IC_GENERIC_BARREL: return "Generic Barrel-Jack";
     default:                          return "Unknown";
     }
 }
 