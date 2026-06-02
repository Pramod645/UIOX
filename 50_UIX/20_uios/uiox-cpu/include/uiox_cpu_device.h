/**
 * @file    uiox_cpu_device.h
 * @brief   UIOX CPU/SoC top-level application-facing device API.
 * @date    2026-06-02
 */

 #ifndef UIOX_CPU_DEVICE_H
 #define UIOX_CPU_DEVICE_H
 
 #include "uiox_cpu_subsys.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 typedef struct {
     uiox_cpu_hw_t            *hw;
     const uiox_cpu_hw_ops_t  *hw_ops;
     uint32_t                  timer_interval_ns;
     uiox_cpu_governor_t       governor;
     uiox_cpu_evt_cb_t         evt_cb;
     void                     *evt_ctx;
 } uiox_cpu_open_params_t;
 
 typedef struct {
     uiox_cpu_subsys_t  subsys;
     uiox_cpu_hw_t     *hw;
     bool               open;
 } uiox_cpu_device_t;
 
 /* =========================================================================
  * Application API
  * ====================================================================== */
 
 int  uiox_cpu_open          (uiox_cpu_device_t           *dev,
                               const uiox_cpu_open_params_t *p);
 int  uiox_cpu_start         (uiox_cpu_device_t *dev);
 void uiox_cpu_stop          (uiox_cpu_device_t *dev);
 void uiox_cpu_close         (uiox_cpu_device_t *dev);
 
 /** Periodic tick — call from system timer ISR bottom-half. */
 void uiox_cpu_tick          (uiox_cpu_device_t *dev, uint32_t now_ms);
 
 /** Bring a secondary CPU core online at the given entry point. */
 int  uiox_cpu_core_up       (uiox_cpu_device_t *dev,
                               uint8_t core_id, uintptr_t entry);
 
 /** Take a secondary CPU core offline. */
 int  uiox_cpu_core_down     (uiox_cpu_device_t *dev, uint8_t core_id);
 
 /** Send IPI to a remote core. */
 int  uiox_cpu_send_ipi      (uiox_cpu_device_t *dev,
                               uint8_t target_core,
                               uiox_ipi_type_t type,
                               uint64_t arg);
 
 /** Set DVFS governor. */
 void uiox_cpu_set_governor  (uiox_cpu_device_t *dev, uiox_cpu_governor_t gov);
 
 /** Set a specific OPP (freq/voltage) for a core. */
 int  uiox_cpu_set_opp       (uiox_cpu_device_t *dev,
                               uint8_t core_id, uint8_t opp_idx);
 
 /** Read core temperature. */
 int  uiox_cpu_read_temp     (uiox_cpu_device_t *dev,
                               uint8_t core_id, int8_t *temp_out);
 
 /** Update CPU load estimate for governor. */
 void uiox_cpu_update_load   (uiox_cpu_device_t *dev,
                               uint8_t core_id, uint32_t load_pct);
 
 /** Flush cache range [addr, addr+size). */
 void uiox_cpu_cache_flush   (uiox_cpu_device_t *dev,
                               uintptr_t addr, size_t size);
 
 /** Start a PMU hardware counter. */
 int  uiox_cpu_pmu_start     (uiox_cpu_device_t *dev,
                               uint8_t counter_id, uint32_t event_id);
 
 /** Stop a PMU counter. */
 void uiox_cpu_pmu_stop      (uiox_cpu_device_t *dev, uint8_t counter_id);
 
 /** Read a PMU counter value. */
 uint64_t uiox_cpu_pmu_read  (uiox_cpu_device_t *dev, uint8_t counter_id);
 
 /** Get current CPU frequency for a core (MHz). */
 uint32_t uiox_cpu_freq      (const uiox_cpu_device_t *dev, uint8_t core_id);
 
 /** Get current cycle count (arch-specific). */
 uint64_t uiox_cpu_cycles    (void);
 
 /** Get hardware timestamp. */
 uint64_t uiox_cpu_timestamp (const uiox_cpu_device_t *dev);
 
 /** Get system uptime (ms). */
 uint64_t uiox_cpu_uptime_ms (const uiox_cpu_device_t *dev);
 
 /** Query current core state. */
 uiox_cpu_core_state_t uiox_cpu_core_state(const uiox_cpu_device_t *dev,
                                            uint8_t core_id);
 
 void uiox_cpu_print_info    (const uiox_cpu_device_t *dev);
 void uiox_cpu_print_stats   (const uiox_cpu_device_t *dev);
 
 const char *uiox_cpu_arch_name (uiox_cpu_arch_t arch);
 const char *uiox_cpu_state_name(uiox_cpu_subsys_state_t s);
 const char *uiox_cpu_evt_name  (uiox_cpu_evt_t evt);
 const char *uiox_cpu_gov_name  (uiox_cpu_governor_t gov);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_CPU_DEVICE_H */
 