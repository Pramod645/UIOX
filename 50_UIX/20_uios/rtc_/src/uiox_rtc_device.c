/**
 * @file  uiox_rtc_device.c
 * @brief UIOX RTC application device API implementation.
 * @date  2026-06-10
 */

 #include "uiox_rtc_device.h"
 #include <string.h>
 #include <errno.h>
 #include <stdio.h>
 
 int uiox_rtc_open(uiox_rtc_device_t *dev, const uiox_rtc_open_params_t *p)
 {
     if (!dev || !p || !p->hw || !p->hw_ops) return -EINVAL;
     memset(dev, 0, sizeof(*dev));
     dev->hw   = p->hw;
     int rc    = uiox_rtc_hw_init(p->hw, p->hw_ops);
     if (rc < 0) return rc;
     rc = uiox_rtc_subsys_init(&dev->subsys, p->hw);
     if (rc < 0) return rc;
     if (p->evt_cb)
         uiox_rtc_subsys_set_cb(&dev->subsys, p->evt_cb, p->evt_ctx);
     dev->open = true;
     return 0;
 }
 
 int uiox_rtc_start(uiox_rtc_device_t *dev)
 { if (!dev || !dev->open) return -EINVAL;
   return uiox_rtc_subsys_start(&dev->subsys); }
 
 void uiox_rtc_stop(uiox_rtc_device_t *dev)
 { if (!dev || !dev->open) return;
   uiox_rtc_subsys_stop(&dev->subsys); }
 
 void uiox_rtc_close(uiox_rtc_device_t *dev)
 { if (!dev || !dev->open) return;
   uiox_rtc_stop(dev);
   uiox_rtc_hw_deinit(dev->hw);
   dev->open = false; }
 
 void uiox_rtc_tick(uiox_rtc_device_t *dev, uint32_t now_ms)
 { if (!dev || !dev->open) return;
   uiox_rtc_subsys_tick(&dev->subsys, now_ms); }
 
 int uiox_rtc_get_time(uiox_rtc_device_t *dev, uiox_rtc_tm_t *tm)
 { if (!dev || !dev->open) return -EINVAL;
   return uiox_rtc_subsys_get_time(&dev->subsys, tm); }
 
 int uiox_rtc_set_time(uiox_rtc_device_t *dev, const uiox_rtc_tm_t *tm)
 { if (!dev || !dev->open) return -EINVAL;
   return uiox_rtc_subsys_set_time(&dev->subsys, tm); }
 
 int uiox_rtc_set_alarm(uiox_rtc_device_t *dev, const uiox_rtc_alarm_t *alm)
 { if (!dev || !dev->open) return -EINVAL;
   return uiox_rtc_subsys_set_alarm(&dev->subsys, alm); }
 
 int uiox_rtc_get_alarm(uiox_rtc_device_t *dev, uiox_rtc_alarm_t *alm)
 { if (!dev || !dev->open || !alm) return -EINVAL;
   return uiox_rtc_clock_alarm_read(&dev->subsys.clk, alm); }
 
 void uiox_rtc_print_info(const uiox_rtc_device_t *dev)
 {
     if (!dev) return;
     const uiox_rtc_hw_t   *hw = dev->hw;
     const uiox_rtc_subsys_t *s = &dev->subsys;
     printf("  Model          : %s\n", hw->model);
     printf("  Version        : %s\n", uiox_rtc_ver_name(hw->version));
     printf("  Capabilities   : 0x%08X\n", hw->caps);
     printf("  State          : %s\n", uiox_rtc_state_name(s->state));
     printf("  Battery        : %s\n", uiox_rtc_bat_name(s->bat_state));
     printf("  Index port     : 0x%03X\n", hw->index_port);
     printf("  IRQ            : %u\n",   hw->irq);
     printf("  Mode           : %s  %s\n",
            (hw->caps & UIOX_RTC_CAP_BCD)    ? "BCD"    : "binary",
            (hw->caps & UIOX_RTC_CAP_24H)    ? "24h"    : "12h");
     printf("  Century reg    : 0x%02X\n", hw->century_reg);
 }
 
 void uiox_rtc_print_stats(uiox_rtc_device_t *dev)
 {
     if (!dev) return;
     const uiox_rtc_subsys_t *s = &dev->subsys;
     printf("  Uptime         : %llu ms\n", (unsigned long long)s->uptime_ms);
     printf("  Tick count     : %u\n",   s->tick_count);
     printf("  Alarm fires    : %u\n",   s->alarm_fire_count);
     printf("  Periodic ticks : %u\n",   s->periodic_count);
     uiox_rtc_if_stats_t is;
     uiox_rtc_if_stats_get(&dev->subsys.rif, &is);
     printf("  IRQ alarm      : %llu\n", (unsigned long long)is.irq_alarm);
     printf("  IRQ periodic   : %llu\n", (unsigned long long)is.irq_periodic);
     printf("  IRQ update     : %llu\n", (unsigned long long)is.irq_update);
     printf("  UIP timeouts   : %u\n",   is.uip_timeouts);
     printf("  Bat low events : %u\n",   is.bat_low_events);
     printf("  Errors         : %u\n",   is.errors);
     printf("  Evt pool free  : %u / %u\n",
            uiox_rtc_evt_free_cnt(), UIOX_RTC_EVT_POOL_SIZE);
     printf("  Alm pool free  : %u / %u\n",
            uiox_rtc_alm_free_cnt(), UIOX_RTC_ALM_POOL_SIZE);
 }
 
 const char *uiox_rtc_state_name(uiox_rtc_state_t s)
 {
     switch(s){
     case UIOX_RTC_STATE_OFF:   return "OFF";
     case UIOX_RTC_STATE_INIT:  return "INIT";
     case UIOX_RTC_STATE_READY: return "READY";
     case UIOX_RTC_STATE_ERROR: return "ERROR";
     default:                    return "UNKNOWN";
     }
 }
 
 const char *uiox_rtc_ev_name(uiox_rtc_ev_t ev)
 {
     switch(ev){
     case UIOX_RTC_EV_ALARM_FIRED:       return "ALARM_FIRED";
     case UIOX_RTC_EV_PERIODIC_TICK:     return "PERIODIC_TICK";
     case UIOX_RTC_EV_UPDATE_TICK:       return "UPDATE_TICK";
     case UIOX_RTC_EV_TIME_SET:          return "TIME_SET";
     case UIOX_RTC_EV_BATTERY_LOW:       return "BATTERY_LOW";
     case UIOX_RTC_EV_BATTERY_RESTORED:  return "BATTERY_RESTORED";
     case UIOX_RTC_EV_OSCILLATOR_FAIL:   return "OSCILLATOR_FAIL";
     case UIOX_RTC_EV_ERROR:             return "ERROR";
     default:                             return "UNKNOWN";
     }
 }
 
 const char *uiox_rtc_bat_name(uiox_rtc_bat_t b)
 {
     switch(b){
     case UIOX_RTC_BAT_GOOD:    return "GOOD";
     case UIOX_RTC_BAT_LOW:     return "LOW/DEAD";
     case UIOX_RTC_BAT_UNKNOWN: return "UNKNOWN";
     default:                    return "?";
     }
 }
 
 const char *uiox_rtc_ver_name(uiox_rtc_ver_t v)
 {
     switch(v){
     case UIOX_RTC_VER_MC146818: return "MC146818A";
     case UIOX_RTC_VER_PIIX4:    return "Intel PIIX4";
     case UIOX_RTC_VER_ICH:      return "Intel ICH/PCH";
     case UIOX_RTC_VER_AMD_FCH:  return "AMD FCH";
     default:                     return "Unknown";
     }
 }
 