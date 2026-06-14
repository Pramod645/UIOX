/**
 * @file  uiox_als_device.h
 * @brief UIOX ALS application-facing device API (Layer 5).
 * @date  2026-06-11
 */

 #ifndef UIOX_ALS_DEVICE_H
 #define UIOX_ALS_DEVICE_H
 
 #include "uiox_als_subsys.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 typedef struct {
     uiox_als_hw_t             *hw;
     const uiox_als_hw_ops_t   *hw_ops;
     const uiox_als_coeff_t    *coeff;
     bool                       continuous;
     uiox_als_evt_cb_t          evt_cb;
     void                      *evt_ctx;
 } uiox_als_open_params_t;
 
 typedef struct {
     uiox_als_subsys_t  subsys;
     uiox_als_hw_t     *hw;
     bool               open;
 } uiox_als_device_t;
 
 /* Lifecycle */
 int  uiox_als_open          (uiox_als_device_t *dev,
                               const uiox_als_open_params_t *p);
 int  uiox_als_start         (uiox_als_device_t *dev);
 void uiox_als_stop          (uiox_als_device_t *dev);
 void uiox_als_close         (uiox_als_device_t *dev);
 void uiox_als_tick          (uiox_als_device_t *dev, uint32_t now_ms);
 
 /* Measurement control */
 int  uiox_als_set_gain      (uiox_als_device_t *dev, uiox_als_gain_t g);
 int  uiox_als_set_itime     (uiox_als_device_t *dev, uiox_als_itime_t t);
 int  uiox_als_set_thresh    (uiox_als_device_t *dev,
                               uint32_t low_milli, uint32_t high_milli);
 void uiox_als_auto_gain     (uiox_als_device_t *dev, bool en);
 int  uiox_als_set_trim      (uiox_als_device_t *dev, uint32_t trim);
 
 /* Data access */
 int  uiox_als_get_lux       (uiox_als_device_t *dev, uint32_t *lux_milli);
 int  uiox_als_get_cct       (uiox_als_device_t *dev, uint32_t *cct_k);
 int  uiox_als_get_raw       (uiox_als_device_t *dev,
                               uint16_t *als, uint16_t *white,
                               uint16_t *ir);
 const uiox_als_sample_t *uiox_als_last_sample(const uiox_als_device_t *dev);
 
 /* Info / stats */
 void uiox_als_print_info    (const uiox_als_device_t *dev);
 void uiox_als_print_stats   (uiox_als_device_t *dev);
 
 /* Name helpers */
 const char *uiox_als_state_name(uiox_als_state_t s);
 const char *uiox_als_ev_name   (uiox_als_ev_t ev);
 const char *uiox_als_ic_name   (uiox_als_ic_t ic);
 const char *uiox_als_gain_name (uiox_als_gain_t g);
 const char *uiox_als_itime_name(uiox_als_itime_t t);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_ALS_DEVICE_H */
 