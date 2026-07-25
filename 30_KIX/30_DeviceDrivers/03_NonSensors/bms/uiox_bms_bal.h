/**
 * @file    uiox_bms_bal.h
 * @brief   UIOX BMS cell balancing (passive/active).
 * @date    2026-06-04
 */
//Layer 2b — Cell Balancing
 #ifndef UIOX_BMS_BAL_H
 #define UIOX_BMS_BAL_H
 
 #include "uiox_bms_if.h"
 #include "uiox_klibc.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 #define UIOX_BMS_BAL_DELTA_MV_DEFAULT  10u  /**< Start balance if > 10 mV */
 #define UIOX_BMS_BAL_STOP_MV_DEFAULT   5u   /**< Stop if delta < 5 mV     */
 #define UIOX_BMS_BAL_MAX_CELLS_SIMULT  4u   /**< Max cells balanced at once*/
 
 typedef enum {
     UIOX_BMS_BAL_MODE_OFF     = 0,
     UIOX_BMS_BAL_MODE_PASSIVE,     /**< Resistive dissipation            */
     UIOX_BMS_BAL_MODE_ACTIVE,      /**< Inductor / cap shuttle           */
 } uiox_bms_bal_mode_t;
 
 typedef struct {
     uiox_bms_if_t      *bif;
     uiox_bms_bal_mode_t mode;
     uint16_t            balance_mask;   /**< Active balance cell bitmask  */
     uint32_t            delta_mv;       /**< Imbalance threshold (mV)     */
     uint32_t            stop_mv;        /**< Stop threshold (mV)          */
     uint32_t            bal_time_ms;    /**< Total balancing time (ms)    */
     uint32_t            last_bal_ms;
     bool                enabled;
 } uiox_bms_bal_t;
 
 int  uiox_bms_bal_init    (uiox_bms_bal_t *bal, uiox_bms_if_t *bif,
                             uiox_bms_bal_mode_t mode,
                             uint32_t delta_mv, uint32_t stop_mv);
 void uiox_bms_bal_tick    (uiox_bms_bal_t *bal, uint32_t now_ms);
 void uiox_bms_bal_stop    (uiox_bms_bal_t *bal);
 bool uiox_bms_bal_active  (const uiox_bms_bal_t *bal);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_BMS_BAL_H */
 