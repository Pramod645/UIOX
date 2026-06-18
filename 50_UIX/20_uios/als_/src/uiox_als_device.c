/**
 * @file  uiox_als_device.c
 * @brief UIOX ALS application device API.
 * @date  2026-06-11
 */

 #include "uiox_als_device.h"
 #include <string.h>
 #include <errno.h>
 #include <stdio.h>
 
 int uiox_als_open(uiox_als_device_t *dev,
                    const uiox_als_open_params_t *p)
 {
     if (!dev || !p || !p->hw || !p->hw_ops || !p->coeff) return -EINVAL;
     memset(dev, 0, sizeof(*dev));
     dev->hw = p->hw;
     int rc  = uiox_als_hw_init(p->hw, p->hw_ops);
     if (rc < 0) return rc;
     rc = uiox_als_subsys_init(&dev->subsys, p->hw, p->coeff);
     if (rc < 0) return rc;
     /* Override continuous flag */
     dev->subsys.aif.continuous = p->continuous;
     if (p->evt_cb)
         uiox_als_subsys_set_cb(&dev->subsys, p->evt_cb, p->evt_ctx);
     dev->open = true;
     return 0;
 }
 
 int  uiox_als_start(uiox_als_device_t *dev)
 { if (!dev || !dev->open) return -EINVAL;
   return uiox_als_subsys_start(&dev->subsys); }
 
 void uiox_als_stop(uiox_als_device_t *dev)
 { if (!dev || !dev->open) return;
   uiox_als_subsys_stop(&dev->subsys); }
 
 void uiox_als_close(uiox_als_device_t *dev)
 { if (!dev || !dev->open) return;
   uiox_als_stop(dev);
   uiox_als_hw_deinit(dev->hw);
   dev->open = false; }
 
 void uiox_als_tick(uiox_als_device_t *dev, uint32_t now_ms)
 { if (!dev || !dev->open) return;
   uiox_als_subsys_tick(&dev->subsys, now_ms); }
 
 int uiox_als_set_gain(uiox_als_device_t *dev, uiox_als_gain_t g)
 { if (!dev || !dev->open) return -EINVAL;
   return uiox_als_subsys_set_gain(&dev->subsys, g); }
 
 int uiox_als_set_itime(uiox_als_device_t *dev, uiox_als_itime_t t)
 { if (!dev || !dev->open) return -EINVAL;
   return uiox_als_subsys_set_itime(&dev->subsys, t); }
 
 int uiox_als_set_thresh(uiox_als_device_t *dev,
                          uint32_t low_milli, uint32_t high_milli)
 { if (!dev || !dev->open) return -EINVAL;
   return uiox_als_subsys_set_thresh(&dev->subsys,
                                      low_milli, high_milli); }
 
 void uiox_als_auto_gain(uiox_als_device_t *dev, bool en)
 { if (dev && dev->open) uiox_als_subsys_auto_gain(&dev->subsys, en); }
 
 int uiox_als_set_trim(uiox_als_device_t *dev, uint32_t trim)
 { if (!dev || !dev->open) return -EINVAL;
   uiox_als_cal_set_trim(&dev->subsys.cal, trim); return 0; }
 
 int uiox_als_get_lux(uiox_als_device_t *dev, uint32_t *lux_milli)
 { if (!dev || !dev->open || !lux_milli) return -EINVAL;
   *lux_milli = dev->subsys.last_sample.lux_milli; return 0; }
 
 int uiox_als_get_cct(uiox_als_device_t *dev, uint32_t *cct_k)
 { if (!dev || !dev->open || !cct_k) return -EINVAL;
   *cct_k = dev->subsys.last_sample.cct_k; return 0; }
 
 int uiox_als_get_raw(uiox_als_device_t *dev,
                       uint16_t *als, uint16_t *white, uint16_t *ir)
 {
     if (!dev || !dev->open) return -EINVAL;
     if (als)   *als   = dev->subsys.last_sample.raw_als;
     if (white) *white = dev->subsys.last_sample.raw_white;
     if (ir)    *ir    = dev->subsys.last_sample.raw_ir;
     return 0;
 }
 
 const uiox_als_sample_t *uiox_als_last_sample(const uiox_als_device_t *dev)
 { return (dev && dev->open) ? &dev->subsys.last_sample : NULL; }
 
 void uiox_als_print_info(const uiox_als_device_t *dev)
 {
     if (!dev) return;
     const uiox_als_hw_t    *hw = dev->hw;
     const uiox_als_subsys_t *s = &dev->subsys;
     printf("  Model          : %s\n", hw->model);
     printf("  IC type        : %s\n", uiox_als_ic_name(hw->ic_type));
     printf("  I2C addr       : 0x%02X  bus=%u\n",
            hw->i2c_addr, hw->i2c_bus);
     printf("  IRQ            : %u\n",   hw->irq);
     printf("  Capabilities   : 0x%08X\n", hw->caps);
     printf("  State          : %s\n",   uiox_als_state_name(s->state));
     printf("  Mode           : %s\n",
            s->aif.continuous ? "continuous" : "one-shot");
     printf("  Gain           : %s\n",   uiox_als_gain_name(hw->gain));
     printf("  Int. time      : %s\n",   uiox_als_itime_name(hw->itime));
     printf("  Auto-gain      : %s\n",   s->auto_gain_en ? "ON" : "OFF");
     printf("  Scene          : %s\n",   s->scene_dark ? "DARK" : "BRIGHT");
     printf("  Thresh low     : %u mlux\n", s->thresh_low_milli);
     printf("  Thresh high    : %u mlux\n", s->thresh_high_milli);
 }
 
 void uiox_als_print_stats(uiox_als_device_t *dev)
 {
     if (!dev) return;
     const uiox_als_subsys_t *s = &dev->subsys;
     printf("  Uptime         : %llu ms\n", (unsigned long long)s->uptime_ms);
     printf("  Tick count     : %u\n",   s->tick_count);
     printf("  Samples read   : %u\n",   s->sample_count);
     printf("  Thresh H fires : %u\n",   s->thresh_high_count);
     printf("  Thresh L fires : %u\n",   s->thresh_low_count);
     printf("  Gain changes   : %u\n",   s->gain_change_count);
     uiox_als_if_stats_t is;
     uiox_als_if_stats_get(&dev->subsys.aif, &is);
     printf("  IRQ count      : %u\n",   is.irq_count);
     printf("  IRQ data-ready : %llu\n", (unsigned long long)is.irq_data_ready);
     printf("  IRQ thresh-H   : %llu\n", (unsigned long long)is.irq_thresh_high);
     printf("  IRQ thresh-L   : %llu\n", (unsigned long long)is.irq_thresh_low);
     printf("  Gain changes   : %u\n",   is.gain_changes);
     printf("  Saturations    : %u\n",   is.saturations);
     printf("  Errors         : %u\n",   is.errors);
     printf("  Last lux       : %u.%03u lux\n",
            s->last_sample.lux_milli / 1000u,
            s->last_sample.lux_milli % 1000u);
     printf("  Last CCT       : %u K\n", s->last_sample.cct_k);
     printf("  Last raw ALS   : %u\n",   s->last_sample.raw_als);
     printf("  Last raw white : %u\n",   s->last_sample.raw_white);
     printf("  Last raw IR    : %u\n",   s->last_sample.raw_ir);
     printf("  Sample pool    : %u / %u free\n",
            uiox_als_sample_free_cnt(), UIOX_ALS_SAMPLE_POOL_SIZE);
     printf("  Event pool     : %u / %u free\n",
            uiox_als_evt_free_cnt(), UIOX_ALS_EVT_POOL_SIZE);
 }
 
 const char *uiox_als_state_name(uiox_als_state_t s)
 {
     switch(s){
     case UIOX_ALS_STATE_OFF:   return "OFF";
     case UIOX_ALS_STATE_INIT:  return "INIT";
     case UIOX_ALS_STATE_READY: return "READY";
     case UIOX_ALS_STATE_ERROR: return "ERROR";
     default:                    return "UNKNOWN";
     }
 }
 
 const char *uiox_als_ev_name(uiox_als_ev_t ev)
 {
     switch(ev){
     case UIOX_ALS_EV_DATA_READY:   return "DATA_READY";
     case UIOX_ALS_EV_THRESH_HIGH:  return "THRESH_HIGH";
     case UIOX_ALS_EV_THRESH_LOW:   return "THRESH_LOW";
     case UIOX_ALS_EV_DARK:         return "DARK";
     case UIOX_ALS_EV_BRIGHT:       return "BRIGHT";
     case UIOX_ALS_EV_GAIN_CHANGED: return "GAIN_CHANGED";
     case UIOX_ALS_EV_SATURATED:    return "SATURATED";
     case UIOX_ALS_EV_ERROR:        return "ERROR";
     default:                        return "UNKNOWN";
     }
 }
 
 const char *uiox_als_ic_name(uiox_als_ic_t ic)
 {
     switch(ic){
     case UIOX_ALS_IC_VEML7700: return "Vishay VEML7700";
     case UIOX_ALS_IC_OPT3001:  return "TI OPT3001";
     case UIOX_ALS_IC_BH1750:   return "ROHM BH1750";
     case UIOX_ALS_IC_TSL2591:  return "AMS TSL2591";
     default:                    return "Unknown";
     }
 }
 
 const char *uiox_als_gain_name(uiox_als_gain_t g)
 {
     switch(g){
     case UIOX_ALS_GAIN_1_8X: return "1/8x";
     case UIOX_ALS_GAIN_1_4X: return "1/4x";
     case UIOX_ALS_GAIN_1X:   return "1x";
     case UIOX_ALS_GAIN_2X:   return "2x";
     case UIOX_ALS_GAIN_8X:   return "8x";
     case UIOX_ALS_GAIN_16X:  return "16x";
     case UIOX_ALS_GAIN_48X:  return "48x";
     default:                  return "?";
     }
 }
 
 const char *uiox_als_itime_name(uiox_als_itime_t t)
 {
     switch(t){
     case UIOX_ALS_ITIME_25MS:  return "25ms";
     case UIOX_ALS_ITIME_50MS:  return "50ms";
     case UIOX_ALS_ITIME_100MS: return "100ms";
     case UIOX_ALS_ITIME_200MS: return "200ms";
     case UIOX_ALS_ITIME_400MS: return "400ms";
     case UIOX_ALS_ITIME_800MS: return "800ms";
     default:                    return "?";
     }
 }
 