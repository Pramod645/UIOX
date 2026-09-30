/**
 * @file  uiox_als_if.h
 * @brief UIOX ALS interface driver — I²C access, IRQ, integration timer.
 * @date  2026-06-11
 */

 #ifndef UIOX_ALS_IF_H
 #define UIOX_ALS_IF_H
 
 #include "uiox_als_hw.h"
 #include "uiox_als_buf.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 typedef struct {
     uint64_t  samples_read;
     uint64_t  irq_data_ready;
     uint64_t  irq_thresh_high;
     uint64_t  irq_thresh_low;
     uint32_t  gain_changes;
     uint32_t  saturations;
     uint32_t  errors;
     uint32_t  irq_count;
 } uiox_als_if_stats_t;
 
 typedef struct {
     uiox_als_hw_t      *hw;
     uiox_als_if_stats_t stats;
     bool                primed;
     bool                continuous;   /**< true = continuous, false = OS  */
     uint32_t            next_sample_ms; /**< Earliest time for next read  */
 } uiox_als_if_t;
 
 int  uiox_als_if_config      (uiox_als_if_t *aif, uiox_als_hw_t *hw,
                                bool continuous);
 int  uiox_als_if_start       (uiox_als_if_t *aif);
 void uiox_als_if_stop        (uiox_als_if_t *aif);
 
 /* Trigger a one-shot measurement (non-continuous mode) */
 int  uiox_als_if_trigger     (uiox_als_if_t *aif);
 
 /* Fetch raw counts — returns 0 if data not ready yet */
 int  uiox_als_if_fetch       (uiox_als_if_t *aif, uint32_t now_ms,
                                uint16_t *als, uint16_t *white,
                                uint16_t *ir);
 
 /* Gain / itime */
 int  uiox_als_if_set_gain    (uiox_als_if_t *aif, uiox_als_gain_t g);
 int  uiox_als_if_set_itime   (uiox_als_if_t *aif, uiox_als_itime_t t);
 
 /* Threshold */
 int  uiox_als_if_set_threshold(uiox_als_if_t *aif,
                                 uint16_t low, uint16_t high);
 
 /* IRQ handler — call from platform ISR or tick */
 uiox_als_evt_t *uiox_als_if_irq_handle(uiox_als_if_t *aif,
                                         uint32_t now_ms);
 
 void uiox_als_if_stats_get   (const uiox_als_if_t *aif,
                                uiox_als_if_stats_t *out);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_ALS_IF_H */
 