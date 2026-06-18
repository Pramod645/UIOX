/**
 * @file    uiox_pmic_device.h
 * @brief   UIOX PMIC top-level application-facing device API.
 * @date    2026-06-04
 */

 #ifndef UIOX_PMIC_DEVICE_H
 #define UIOX_PMIC_DEVICE_H
 
 #include "uiox_pmic_subsys.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 typedef struct {
     uiox_pmic_hw_t            *hw;
     const uiox_pmic_hw_ops_t  *hw_ops;
     uiox_pmic_evt_cb_t         evt_cb;
     void                      *evt_ctx;
 } uiox_pmic_open_params_t;
 
 typedef struct {
     uiox_pmic_subsys_t  subsys;
     uiox_pmic_hw_t     *hw;
     bool                open;
 } uiox_pmic_device_t;
 
 int  uiox_pmic_open          (uiox_pmic_device_t           *dev,
                                const uiox_pmic_open_params_t *p);
 int  uiox_pmic_start         (uiox_pmic_device_t *dev);
 void uiox_pmic_stop          (uiox_pmic_device_t *dev);
 void uiox_pmic_close         (uiox_pmic_device_t *dev);
 void uiox_pmic_tick          (uiox_pmic_device_t *dev, uint32_t now_ms);
 
 /* Rail management */
 int  uiox_pmic_rail_add      (uiox_pmic_device_t       *dev,
                                const uiox_pmic_rail_t   *rail);
 int  uiox_pmic_rail_on       (uiox_pmic_device_t *dev, const char *name);
 int  uiox_pmic_rail_off      (uiox_pmic_device_t *dev, const char *name);
 int  uiox_pmic_rail_voltage  (uiox_pmic_device_t *dev,
                                const char *name, uint32_t mv);
 int  uiox_pmic_rail_read_mv  (uiox_pmic_device_t *dev,
                                const char *name, uint32_t *mv_out);
 
 /* Power state */
 int  uiox_pmic_set_ps        (uiox_pmic_device_t *dev, uiox_pmic_ps_t ps);
 int  uiox_pmic_add_opp       (uiox_pmic_device_t *dev,
                                uint32_t cpu_mhz, uint32_t vcore_mv,
                                uint32_t power_mw);
 void uiox_pmic_update_load   (uiox_pmic_device_t *dev,
                                uint32_t load_pct, uint32_t now_ms);
 
 /* ADC */
 int  uiox_pmic_adc_read      (uiox_pmic_device_t *dev,
                                uiox_pmic_adc_ch_t ch, uint32_t *result);
 
 /* Telemetry */
 int  uiox_pmic_get_telemetry (uiox_pmic_device_t *dev,
                                uiox_pmic_telem_t *out, uint32_t now_ms);
 
 /* Watchdog */
 int  uiox_pmic_wdt_kick      (uiox_pmic_device_t *dev);
 
 void uiox_pmic_print_info    (const uiox_pmic_device_t *dev);
 void uiox_pmic_print_stats   (uiox_pmic_device_t *dev);
 void uiox_pmic_print_events  (void);
 
 const char *uiox_pmic_state_name(uiox_pmic_subsys_state_t s);
 const char *uiox_pmic_ps_name   (uiox_pmic_ps_t ps);
 const char *uiox_pmic_ev_name   (uiox_pmic_ev_t ev);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_PMIC_DEVICE_H */
 