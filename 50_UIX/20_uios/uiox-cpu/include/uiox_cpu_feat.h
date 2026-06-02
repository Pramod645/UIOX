/**
 * @file    uiox_cpu_feat.h
 * @brief   UIOX CPU feature detection: cache, ISA, PMU, topology.
 * @date    2026-06-02
 */

 #ifndef UIOX_CPU_FEAT_H
 #define UIOX_CPU_FEAT_H
 
 #include "uiox_cpu_pm.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 #define UIOX_PMU_MAX_COUNTERS   8
 
 typedef struct {
     uint8_t   counter_id;
     uint32_t  event_id;     /**< Arch-specific event (e.g. 0x08 = inst)  */
     uint64_t  value;
     bool      enabled;
 } uiox_pmu_counter_t;
 
 typedef struct {
     uiox_cpu_hw_t      *hw;
     uiox_pmu_counter_t  counters[UIOX_PMU_MAX_COUNTERS];
     uint8_t             num_counters;
 
     /* CPUID / ISA info */
     uint32_t            cpuid_family;
     uint32_t            cpuid_model;
     uint32_t            cpuid_stepping;
     char                brand_string[64];
 
     /* Detected topology */
     uint8_t             num_sockets;
     uint8_t             cores_per_socket;
     uint8_t             threads_per_core;
 
     /* Cache sizes */
     uiox_cpu_cache_t    l1i, l1d, l2, l3;
 } uiox_cpu_feat_t;
 
 int      uiox_cpu_feat_init       (uiox_cpu_feat_t *feat, uiox_cpu_hw_t *hw);
 int      uiox_cpu_feat_detect     (uiox_cpu_feat_t *feat);
 int      uiox_cpu_feat_pmu_start  (uiox_cpu_feat_t *feat,
                                     uint8_t counter_id, uint32_t event_id);
 void     uiox_cpu_feat_pmu_stop   (uiox_cpu_feat_t *feat, uint8_t counter_id);
 uint64_t uiox_cpu_feat_pmu_read   (uiox_cpu_feat_t *feat, uint8_t counter_id);
 void     uiox_cpu_feat_print      (const uiox_cpu_feat_t *feat);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_CPU_FEAT_H */
 