/**
 * @file  uiox_chg_device.h
 * @brief UIOX Charger application-facing device API (Layer 5).
 * @date  2026-06-11
 */

 #ifndef UIOX_CHG_DEVICE_H
 #define UIOX_CHG_DEVICE_H
 
 #include "uiox_chg_subsys.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 typedef struct {
     uiox_chg_hw_t             *hw;
     const uiox_chg_hw_ops_t   *hw_ops;
     const uiox_chg_profile_t  *profile;
     uiox_chg_evt_cb_t          evt_cb;
     void                      *evt_ctx;
 } uiox_chg_open_params_t;
 
 typedef struct {
     uiox_chg_subsys_t  subsys;
     uiox_chg_hw_t     *hw;
     bool               open;
 } uiox_chg_device_t;
 
 /* Lifecycle */
 int  uiox_chg_open         (uiox_chg_device_t *dev,
                              const uiox_chg_open_params_t *p);
 int  uiox_chg_start        (uiox_chg_device_t *dev);
 void uiox_chg_stop         (uiox_chg_device_t *dev);
 void uiox_chg_close        (uiox_chg_device_t *dev);
 void uiox_chg_tick         (uiox_chg_device_t *dev, uint32_t now_ms);
 
 /* Control */
 int  uiox_chg_enable       (uiox_chg_device_t *dev, bool en);
 int  uiox_chg_otg_enable   (uiox_chg_device_t *dev, bool en);
 int  uiox_chg_set_profile  (uiox_chg_device_t *dev,
                              const uiox_chg_profile_t *p);
 
 /* Status */
 int  uiox_chg_get_adc      (uiox_chg_device_t *dev,
                              uiox_chg_adc_ch_t ch, int32_t *val_mv);
 int  uiox_chg_get_status   (uiox_chg_device_t *dev,
                              uiox_chg_chrg_t *chrg,
                              uiox_chg_src_t  *src,
                              uint32_t        *faults);
 
 /* Info / stats */
 void uiox_chg_print_info   (const uiox_chg_device_t *dev);
 void uiox_chg_print_stats  (uiox_chg_device_t *dev);
 
 /* Name helpers */
 const char *uiox_chg_state_name (uiox_chg_state_t s);
 const char *uiox_chg_ev_name    (uiox_chg_ev_t ev);
 const char *uiox_chg_src_name   (uiox_chg_src_t src);
 const char *uiox_chg_chrg_name  (uiox_chg_chrg_t c);
 const char *uiox_chg_ic_name    (uiox_chg_ic_t ic);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_CHG_DEVICE_H */
 