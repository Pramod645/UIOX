/**
 * @file    uiox_cpu_subsys.h
 * @brief   UIOX CPU subsystem — topology, scheduler hints, SMP, events.
 * @date    2026-06-02
 */

 #ifndef UIOX_CPU_SUBSYS_H
 #define UIOX_CPU_SUBSYS_H
 
 #include "uiox_cpu_feat.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 typedef enum {
     UIOX_CPU_EVT_CORE_ONLINE = 0,
     UIOX_CPU_EVT_CORE_OFFLINE,
     UIOX_CPU_EVT_FREQ_CHANGE,
     UIOX_CPU_EVT_THERMAL_THROTTLE,
     UIOX_CPU_EVT_FAULT,
     UIOX_CPU_EVT_IPI_RECEIVED,
     UIOX_CPU_EVT_TIMER_TICK,
 } uiox_cpu_evt_t;
 
 typedef void (*uiox_cpu_evt_cb_t)(uiox_cpu_evt_t evt,
                                    uint8_t core_id, void *ctx);
 
 typedef enum {
     UIOX_CPU_SUBSYS_STOPPED = 0,
     UIOX_CPU_SUBSYS_BOOTING,
     UIOX_CPU_SUBSYS_RUNNING,
     UIOX_CPU_SUBSYS_SUSPENDED,
 } uiox_cpu_subsys_state_t;
 
 typedef struct {
     uiox_cpu_if_t           cif;
     uiox_cpu_pm_t            pm;
     uiox_cpu_feat_t          feat;
     uiox_cpu_subsys_state_t  state;
     uiox_cpu_evt_cb_t        evt_cb;
     void                    *evt_ctx;
     uint32_t                 tick_ms;
     uint64_t                 uptime_ms;
     uint32_t                 ipi_count;
     uint32_t                 fault_count;
     uint32_t                 timer_ticks;
 } uiox_cpu_subsys_t;
 
 int  uiox_cpu_subsys_init      (uiox_cpu_subsys_t *sys,
                                  uiox_cpu_hw_t     *hw,
                                  uint32_t           timer_interval_ns);
 int  uiox_cpu_subsys_start     (uiox_cpu_subsys_t *sys);
 void uiox_cpu_subsys_stop      (uiox_cpu_subsys_t *sys);
 void uiox_cpu_subsys_tick      (uiox_cpu_subsys_t *sys, uint32_t now_ms);
 
 int  uiox_cpu_subsys_core_up   (uiox_cpu_subsys_t *sys,
                                  uint8_t core_id, uintptr_t entry);
 int  uiox_cpu_subsys_core_down (uiox_cpu_subsys_t *sys, uint8_t core_id);
 int  uiox_cpu_subsys_ipi       (uiox_cpu_subsys_t *sys,
                                  uint8_t target_core,
                                  uiox_ipi_type_t type,
                                  uint64_t arg0);
 
 void uiox_cpu_subsys_set_cb    (uiox_cpu_subsys_t *sys,
                                  uiox_cpu_evt_cb_t cb, void *ctx);
 void uiox_cpu_subsys_set_gov   (uiox_cpu_subsys_t *sys,
                                  uiox_cpu_governor_t gov);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_CPU_SUBSYS_H */
 