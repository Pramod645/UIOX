/**
 * @file  uiox_rtc_buf.h
 * @brief UIOX RTC alarm/event queue buffer pool.
 * @date  2026-06-10
 */

 #ifndef UIOX_RTC_BUF_H
 #define UIOX_RTC_BUF_H
 
 #include "uiox_rtc_hw.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 #define UIOX_RTC_EVT_POOL_SIZE   16u   /**< Circular event queue depth    */
 #define UIOX_RTC_ALM_POOL_SIZE   8u    /**< Pending alarm slots           */
 
 /* =========================================================================
  * Event record
  * ====================================================================== */
 
 typedef enum {
     UIOX_RTC_EVT_NONE      = 0,
     UIOX_RTC_EVT_ALARM,            /**< Alarm fired                       */
     UIOX_RTC_EVT_PERIODIC,         /**< Periodic tick                     */
     UIOX_RTC_EVT_UPDATE,           /**< 1 Hz update-ended tick            */
     UIOX_RTC_EVT_BAT_LOW,          /**< Battery low detected              */
     UIOX_RTC_EVT_SET_TIME,         /**< Time was programmed               */
 } uiox_rtc_evt_type_t;
 
 typedef struct {
     uiox_rtc_evt_type_t type;
     uint32_t            timestamp_ms;  /**< System ms at event time       */
     uint8_t             flags;         /**< Raw Register C value          */
     uint8_t             in_use;
 } uiox_rtc_evt_t;
 
 /* =========================================================================
  * Alarm record
  * ====================================================================== */
 
 typedef struct {
     uint8_t  sec;      /**< 0xFF = don't-care                             */
     uint8_t  min;
     uint8_t  hr;
     bool     enabled;
     bool     pending;
     uint8_t  in_use;
 } uiox_rtc_alm_t;
 
 /* =========================================================================
  * Pool API
  * ====================================================================== */
 
 void             uiox_rtc_buf_init    (void);
 uiox_rtc_evt_t  *uiox_rtc_evt_alloc  (void);
 void             uiox_rtc_evt_free    (uiox_rtc_evt_t *e);
 uint8_t          uiox_rtc_evt_free_cnt(void);
 
 uiox_rtc_alm_t  *uiox_rtc_alm_alloc  (void);
 void             uiox_rtc_alm_free    (uiox_rtc_alm_t *a);
 uint8_t          uiox_rtc_alm_free_cnt(void);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_RTC_BUF_H */
 