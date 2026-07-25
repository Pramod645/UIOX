/**
 * @file    uiox_pmic_policy.h
 * @brief   UIOX PMIC power policy: DVFS, sleep, wake, load balance.
 * @date    2026-06-04
 */
//Layer 3 — Power Policy
 #ifndef UIOX_PMIC_POLICY_H
 #define UIOX_PMIC_POLICY_H
 
 #include "uiox_pmic_rail.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * System power state
  * ====================================================================== */
 
 typedef enum {
     UIOX_PMIC_PS_ACTIVE   = 0,  /**< Full performance                    */
     UIOX_PMIC_PS_BALANCED,       /**< Balanced power/performance          */
     UIOX_PMIC_PS_POWERSAVE,      /**< Minimum power consumption          */
     UIOX_PMIC_PS_SLEEP,          /**< Suspend to RAM / deep sleep         */
     UIOX_PMIC_PS_HIBERNATE,      /**< Suspend to disk                    */
     UIOX_PMIC_PS_SHUTDOWN,       /**< Complete power-off                 */
 } uiox_pmic_ps_t;
 
 /* =========================================================================
  * DVFS operating point
  * ====================================================================== */
 
 #define UIOX_PMIC_MAX_OPP    8
 
 typedef struct {
     uint32_t  cpu_freq_mhz;
     uint32_t  vcore_mv;     /**< CPU core voltage at this OPP            */
     uint32_t  power_mw;     /**< Estimated power consumption             */
 } uiox_pmic_opp_t;
 
 /* =========================================================================
  * Thermal policy
  * ====================================================================== */
 
 typedef struct {
     int8_t   throttle_temp_c;  /**< Start throttling (default 85°C)     */
     int8_t   critical_temp_c;  /**< Emergency shutdown (default 105°C)  */
     int8_t   resume_temp_c;    /**< Resume full speed (default 75°C)    */
     uint32_t throttle_mv;      /**< Voltage to set on throttle           */
 } uiox_pmic_thermal_cfg_t;
 
 /* =========================================================================
  * Power policy context
  * ====================================================================== */
 
 typedef struct {
     uiox_pmic_rail_mgr_t    *mgr;
     uiox_pmic_ps_t           current_ps;
     uiox_pmic_opp_t          opps[UIOX_PMIC_MAX_OPP];
     uint8_t                  num_opps;
     uint8_t                  cur_opp;
     uiox_pmic_thermal_cfg_t  thermal;
     bool                     throttled;
     uint32_t                 cpu_load_pct;  /**< 0..100 %                 */
     const char              *vcore_rail;    /**< Name of CPU core rail    */
     const char              *vmem_rail;     /**< Name of DDR rail         */
     const char              *vio_rail;      /**< Name of I/O rail         */
 } uiox_pmic_policy_t;
 
 /* =========================================================================
  * Policy API
  * ====================================================================== */
 
 int  uiox_pmic_policy_init      (uiox_pmic_policy_t      *pol,
                                   uiox_pmic_rail_mgr_t    *mgr,
                                   const uiox_pmic_thermal_cfg_t *thermal,
                                   const char *vcore, const char *vmem,
                                   const char *vio);
 
 int  uiox_pmic_policy_add_opp   (uiox_pmic_policy_t *pol,
                                   uint32_t cpu_mhz, uint32_t vcore_mv,
                                   uint32_t power_mw);
 
 int  uiox_pmic_policy_set_ps    (uiox_pmic_policy_t *pol,
                                   uiox_pmic_ps_t ps);
 
 int  uiox_pmic_policy_set_opp   (uiox_pmic_policy_t *pol, uint8_t opp_idx);
 
 /** Update CPU load and auto-adjust OPP (called from tick). */
 void uiox_pmic_policy_update_load(uiox_pmic_policy_t *pol,
                                    uint32_t load_pct, uint32_t now_ms);
 
 /** Thermal check — apply throttle if needed (called from tick). */
 void uiox_pmic_policy_thermal_tick(uiox_pmic_policy_t *pol, int8_t temp_c);
 
 /** Enter sleep — disable non-essential rails. */
 int  uiox_pmic_policy_sleep     (uiox_pmic_policy_t *pol);
 
 /** Wake from sleep — restore rails. */
 int  uiox_pmic_policy_wake      (uiox_pmic_policy_t *pol);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_PMIC_POLICY_H */
 