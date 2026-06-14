/**
 * @file  uiox_als_subsys.h
 * @brief UIOX ALS Subsystem — threshold, auto-gain, dark/bright events.
 * @date  2026-06-11
 */

 #ifndef UIOX_ALS_SUBSYS_H
 #define UIOX_ALS_SUBSYS_H
 
 #include "uiox_als_cal.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * Subsystem events
  * ====================================================================== */
 
 typedef enum {
     UIOX_ALS_EV_DATA_READY    = 0,
     UIOX_ALS_EV_THRESH_HIGH,
     UIOX_ALS_EV_THRESH_LOW,
     UIOX_ALS_EV_DARK,
     UIOX_ALS_EV_BRIGHT,
     UIOX_ALS_EV_GAIN_CHANGED,
     UIOX_ALS_EV_SATURATED,
     UIOX_ALS_EV_ERROR,
 } uiox_als_ev_t;
 
 typedef void (*uiox_als_evt_cb_t)(uiox_als_ev_t ev,
                                    const uiox_als_sample_t *sample,
                                    void *ctx);
 
 /* =========================================================================
  * Subsystem state
  * ====================================================================== */
 
 typedef enum {
     UIOX_ALS_STATE_OFF   = 0,
     UIOX_ALS_STATE_INIT,
     UIOX_ALS_STATE_READY,
     UIOX_ALS_STATE_ERROR,
 } uiox_als_state_t;
 
 /* Lux thresholds for dark/bright scene transitions (×1000) */
 #define UIOX_ALS_DARK_THRESH_MILLI    10000u   /**< < 10 lux = dark        */
 #define UIOX_ALS_BRIGHT_THRESH_MILLI  100000u  /**< > 100 lux = bright     */
 
 typedef struct {
     uiox_als_if_t        aif;
     uiox_als_cal_t       cal;
     uiox_als_state_t     state;
     uiox_als_evt_cb_t    evt_cb;
     void                *evt_ctx;
     /* User-configurable lux thresholds (milli-lux) */
     uint32_t             thresh_high_milli;
     uint32_t             thresh_low_milli;
     /* Scene state */
     bool                 scene_dark;
     bool                 auto_gain_en;
     /* Last sample (copy; not from pool) */
     uiox_als_sample_t    last_sample;
     /* Statistics */
     uint32_t             tick_count;
     uint64_t             uptime_ms;
     uint32_t             sample_count;
     uint32_t             thresh_high_count;
     uint32_t             thresh_low_count;
     uint32_t             gain_change_count;
 } uiox_als_subsys_t;
 
 /* =========================================================================
  * Subsystem API
  * ====================================================================== */
 
 int  uiox_als_subsys_init       (uiox_als_subsys_t *sys,
                                   uiox_als_hw_t *hw,
                                   const uiox_als_coeff_t *coeff);
 int  uiox_als_subsys_start      (uiox_als_subsys_t *sys);
 void uiox_als_subsys_stop       (uiox_als_subsys_t *sys);
 void uiox_als_subsys_tick       (uiox_als_subsys_t *sys, uint32_t now_ms);
 void uiox_als_subsys_set_cb     (uiox_als_subsys_t *sys,
                                   uiox_als_evt_cb_t cb, void *ctx);
 int  uiox_als_subsys_set_thresh (uiox_als_subsys_t *sys,
                                   uint32_t low_milli, uint32_t high_milli);
 void uiox_als_subsys_auto_gain  (uiox_als_subsys_t *sys, bool en);
 int  uiox_als_subsys_set_gain   (uiox_als_subsys_t *sys,
                                   uiox_als_gain_t g);
 int  uiox_als_subsys_set_itime  (uiox_als_subsys_t *sys,
                                   uiox_als_itime_t t);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_ALS_SUBSYS_H */
 