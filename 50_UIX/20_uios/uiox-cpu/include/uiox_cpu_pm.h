/**
 * @file    uiox_cpu_pm.h
 * @brief   UIOX CPU power management: DVFS, idle, thermal throttling.
 * @date    2026-06-02
 */

 #ifndef UIOX_CPU_PM_H
 #define UIOX_CPU_PM_H
 
 #include "uiox_cpu_if.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 #define UIOX_CPU_PM_MAX_OPP     16  /**< Max operating performance points  */
 #define UIOX_CPU_THERMAL_LIMIT  95  /**< Thermal throttle threshold (°C)   */
 #define UIOX_CPU_CRIT_TEMP      105 /**< Critical shutdown temperature      */
 
 /* =========================================================================
  * Operating Performance Point (OPP)
  * ====================================================================== */
 
  typedef struct {
    uint32_t  freq_mhz;
    uint32_t  voltage_uv;   /**< Supply voltage in microvolts              */
    uint32_t  power_mw;     /**< Estimated power at this OPP (mW)         */
} uiox_cpu_opp_t;

/* =========================================================================
 * CPU idle states (C-states / WFI depth)
 * ====================================================================== */

typedef enum {
    UIOX_CPU_IDLE_C0 = 0,   /**< Active — running                         */
    UIOX_CPU_IDLE_C1,        /**< Clock gated — WFI/HLT                   */
    UIOX_CPU_IDLE_C2,        /**< Power gated — retention                  */
    UIOX_CPU_IDLE_C3,        /**< Deep sleep — DRAM self-refresh           */
} uiox_cpu_idle_state_t;

/* =========================================================================
 * DVFS governor type
 * ====================================================================== */

typedef enum {
    UIOX_CPU_GOV_PERFORMANCE = 0,  /**< Always maximum frequency           */
    UIOX_CPU_GOV_POWERSAVE,        /**< Always minimum frequency           */
    UIOX_CPU_GOV_ONDEMAND,         /**< Scale based on load                */
    UIOX_CPU_GOV_SCHEDUTIL,        /**< Scheduler-driven scaling           */
    UIOX_CPU_GOV_CONSERVATIVE,     /**< Slow to ramp up                   */
} uiox_cpu_governor_t;

/* =========================================================================
 * Power management context
 * ====================================================================== */

typedef struct {
    uiox_cpu_if_t        *cif;
    uiox_cpu_opp_t        opps[UIOX_CPU_PM_MAX_OPP];
    uint8_t               num_opps;
    uint8_t               cur_opp[UIOX_CPU_MAX_CORES];
    uiox_cpu_governor_t   governor;
    uiox_cpu_idle_state_t idle_state[UIOX_CPU_MAX_CORES];
    int8_t                temp[UIOX_CPU_MAX_CORES];
    uint8_t               throttle_count;   /**< Active throttle events    */
    bool                  thermal_limit_hit;
    uint32_t              load_pct[UIOX_CPU_MAX_CORES]; /**< 0..100 %      */
} uiox_cpu_pm_t;

/* =========================================================================
 * Power management API
 * ====================================================================== */

int  uiox_cpu_pm_init        (uiox_cpu_pm_t *pm, uiox_cpu_if_t *cif);
int  uiox_cpu_pm_add_opp     (uiox_cpu_pm_t *pm, uint32_t freq_mhz,
                               uint32_t voltage_uv, uint32_t power_mw);
int  uiox_cpu_pm_set_governor(uiox_cpu_pm_t *pm, uiox_cpu_governor_t gov);
int  uiox_cpu_pm_set_opp     (uiox_cpu_pm_t *pm, uint8_t core_id,
                               uint8_t opp_idx);
int  uiox_cpu_pm_enter_idle  (uiox_cpu_pm_t *pm, uint8_t core_id,
                               uiox_cpu_idle_state_t state);
void uiox_cpu_pm_exit_idle   (uiox_cpu_pm_t *pm, uint8_t core_id);

/**
 * @brief  Periodic DVFS tick — adjusts frequency based on load + thermal.
 * @param  now_ms  Monotonic time (ms).
 */
void uiox_cpu_pm_tick        (uiox_cpu_pm_t *pm, uint32_t now_ms);

/** Update CPU load for DVFS governor. */
void uiox_cpu_pm_update_load (uiox_cpu_pm_t *pm, uint8_t core_id,
                               uint32_t load_pct);

#ifdef __cplusplus
}
#endif
#endif /* UIOX_CPU_PM_H */
