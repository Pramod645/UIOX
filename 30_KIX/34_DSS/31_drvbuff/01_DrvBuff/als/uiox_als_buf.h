/**
 * @file  uiox_als_buf.h
 * @brief UIOX ALS sample and event buffer pool.
 * @date  2026-06-11
 */

 #ifndef UIOX_ALS_BUF_H
 #define UIOX_ALS_BUF_H
 
 #include "uiox_als_hw.h"
 #include "uiox_klibc.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 #define UIOX_ALS_SAMPLE_POOL_SIZE   32u
 #define UIOX_ALS_EVT_POOL_SIZE      16u
 
 /* =========================================================================
  * Sample record
  * ====================================================================== */
 
 typedef struct {
     uint32_t         timestamp_ms;
     uint16_t         raw_als;
     uint16_t         raw_white;
     uint16_t         raw_ir;
     uint32_t         lux_milli;    /**< Lux × 1000 (integer fixed-point)  */
     uint32_t         cct_k;        /**< Correlated colour temp (Kelvin)   */
     uiox_als_gain_t  gain;
     uiox_als_itime_t itime;
     uint8_t          saturated;    /**< 1 = ADC near full-scale           */
     uint8_t          in_use;
 } uiox_als_sample_t;
 
 /* =========================================================================
  * Event record
  * ====================================================================== */
 
 typedef enum {
     UIOX_ALS_EVT_NONE        = 0,
     UIOX_ALS_EVT_DATA_READY,       /**< New sample available              */
     UIOX_ALS_EVT_THRESH_HIGH,      /**< Lux crossed high threshold        */
     UIOX_ALS_EVT_THRESH_LOW,       /**< Lux crossed low  threshold        */
     UIOX_ALS_EVT_GAIN_CHANGED,     /**< Auto-gain adjusted gain/itime     */
     UIOX_ALS_EVT_DARK,             /**< Scene transitioned to dark        */
     UIOX_ALS_EVT_BRIGHT,           /**< Scene transitioned to bright      */
     UIOX_ALS_EVT_SATURATED,        /**< ADC saturated (gain too high)     */
     UIOX_ALS_EVT_ERROR,
 } uiox_als_evt_type_t;
 
 typedef struct {
     uiox_als_evt_type_t  type;
     uint32_t             timestamp_ms;
     uint32_t             lux_milli;
     uiox_als_gain_t      gain;
     uiox_als_itime_t     itime;
     uint8_t              in_use;
 } uiox_als_evt_t;
 
 /* =========================================================================
  * Pool API
  * ====================================================================== */
 
 void               uiox_als_buf_init       (void);
 
 uiox_als_sample_t *uiox_als_sample_alloc   (void);
 void               uiox_als_sample_free    (uiox_als_sample_t *s);
 uint8_t            uiox_als_sample_free_cnt(void);
 
 uiox_als_evt_t    *uiox_als_evt_alloc      (void);
 void               uiox_als_evt_free       (uiox_als_evt_t *e);
 uint8_t            uiox_als_evt_free_cnt   (void);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_ALS_BUF_H */
 