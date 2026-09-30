/**
 * @file    uiox_bms_algo.h
 * @brief   UIOX BMS algorithms: SoC, SoH, OCV lookup.
 * @date    2026-06-04
 */
//Layer 3 — BMS Algorithms
 #ifndef UIOX_BMS_ALGO_H
 #define UIOX_BMS_ALGO_H
 
 #include "uiox_bms_bal.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * OCV-SoC lookup table (NMC 3.5 Ah cell, 25°C)
  * ====================================================================== */
 
 #define UIOX_BMS_OCV_TABLE_SIZE  21
 
 /* =========================================================================
  * Battery parameters
  * ====================================================================== */
 
 typedef struct {
     uint32_t  nominal_mah;    /**< Nominal capacity (mAh)                 */
     uint32_t  full_mah;       /**< Current full charge capacity (mAh)     */
     uint32_t  design_mah;     /**< Design capacity (mAh)                  */
     uint32_t  vfull_mv;       /**< Fully charged voltage (mV)            */
     uint32_t  vempty_mv;      /**< Empty voltage (mV)                    */
     uint32_t  vcell_ovp_mv;   /**< OVP threshold per cell (mV)           */
     uint32_t  vcell_uvp_mv;   /**< UVP threshold per cell (mV)           */
     uint32_t  ocp_chg_ma;     /**< Charge overcurrent (mA)               */
     uint32_t  ocp_dsg_ma;     /**< Discharge overcurrent (mA)            */
     int16_t   max_temp_dc;    /**< Max temperature (°C × 10)             */
     int16_t   min_temp_dc;    /**< Min temperature (°C × 10)             */
     uint16_t  cycle_count;    /**< Charge/discharge cycles                */
 } uiox_bms_batt_t;
 
 /* =========================================================================
  * Algorithm context
  * ====================================================================== */
 
 typedef struct {
     uiox_bms_batt_t  batt;
     int32_t          remain_mah;   /**< Remaining capacity (coulomb count)*/
     uint8_t          soc_pct;      /**< State of Charge (0..100 %)        */
     uint8_t          soh_pct;      /**< State of Health (0..100 %)        */
     int32_t          tte_min;      /**< Time to empty (minutes, -1=N/A)  */
     int32_t          ttf_min;      /**< Time to full  (minutes, -1=N/A)  */
     bool             charging;
     bool             full_detected;
     uint32_t         last_full_mv; /**< Pack voltage when full detected   */
 } uiox_bms_algo_t;
 
 /* =========================================================================
  * Algorithm API
  * ====================================================================== */
 
 int  uiox_bms_algo_init      (uiox_bms_algo_t       *algo,
                                const uiox_bms_batt_t *batt);
 
 /**
  * @brief  Update SoC from coulomb counter + OCV cross-check.
  * @param  current_ma  Pack current (+ve=charge, -ve=discharge).
  * @param  pack_mv     Pack voltage (mV).
  * @param  dt_ms       Elapsed time since last call (ms).
  */
 void uiox_bms_algo_update_soc(uiox_bms_algo_t *algo,
                                int32_t current_ma,
                                uint32_t pack_mv,
                                uint32_t dt_ms);
 
 /** Update SoH from full-charge capacity vs. design capacity. */
 void uiox_bms_algo_update_soh(uiox_bms_algo_t *algo,
                                int32_t measured_full_mah);
 
 /** Estimate TTE and TTF from current consumption. */
 void uiox_bms_algo_update_tte(uiox_bms_algo_t *algo, int32_t current_ma);
 
 /** Lookup SoC from OCV table (for rest/idle state). */
 uint8_t uiox_bms_algo_ocv_to_soc(uint32_t ocv_mv);
 
 /** Check whether pack is fully charged. */
 bool uiox_bms_algo_check_full(uiox_bms_algo_t *algo,
                                uint32_t pack_mv, int32_t current_ma);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_BMS_ALGO_H */
 