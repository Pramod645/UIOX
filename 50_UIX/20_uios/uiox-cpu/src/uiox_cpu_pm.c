/**
 * @file    uiox_cpu_pm.c
 * @brief   UIOX CPU power management implementation.
 * @date    2026-06-02
 */

 #include "uiox_cpu_pm.h"
 #include <string.h>
 #include <errno.h>
 
 int uiox_cpu_pm_init(uiox_cpu_pm_t *pm, uiox_cpu_if_t *cif)
 {
     if (!pm || !cif) return -EINVAL;
     memset(pm, 0, sizeof(*pm));
     pm->cif      = cif;
     pm->governor = UIOX_CPU_GOV_ONDEMAND;
     for (uint8_t i = 0; i < UIOX_CPU_MAX_CORES; i++) {
         pm->idle_state[i] = UIOX_CPU_IDLE_C0;
         pm->temp[i]       = 40;   /* Default 40 °C */
         pm->load_pct[i]   = 0;
     }
     return 0;
 }
 
 int uiox_cpu_pm_add_opp(uiox_cpu_pm_t *pm, uint32_t freq_mhz,
                          uint32_t voltage_uv, uint32_t power_mw)
 {
     if (!pm || pm->num_opps >= UIOX_CPU_PM_MAX_OPP) return -ENOSPC;
     /* Insert sorted by frequency (ascending) */
     uint8_t pos = pm->num_opps;
     for (uint8_t i = 0; i < pm->num_opps; i++) {
         if (freq_mhz < pm->opps[i].freq_mhz) { pos = i; break; }
     }
     /* Shift up */
     for (uint8_t i = pm->num_opps; i > pos; i--)
         pm->opps[i] = pm->opps[i-1];
     pm->opps[pos].freq_mhz   = freq_mhz;
     pm->opps[pos].voltage_uv = voltage_uv;
     pm->opps[pos].power_mw   = power_mw;
     pm->num_opps++;
     return 0;
 }
 
 int uiox_cpu_pm_set_governor(uiox_cpu_pm_t *pm, uiox_cpu_governor_t gov)
 {
     if (!pm) return -EINVAL;
     pm->governor = gov;
     return 0;
 }
 
 int uiox_cpu_pm_set_opp(uiox_cpu_pm_t *pm, uint8_t core_id, uint8_t opp_idx)
 {
     if (!pm || opp_idx >= pm->num_opps || core_id >= UIOX_CPU_MAX_CORES)
         return -EINVAL;
     pm->cur_opp[core_id] = opp_idx;
     uint32_t freq = pm->opps[opp_idx].freq_mhz;
     return uiox_cpu_hw_set_freq(pm->cif->hw, core_id, freq);
 }
 
 int uiox_cpu_pm_enter_idle(uiox_cpu_pm_t *pm, uint8_t core_id,
                             uiox_cpu_idle_state_t state)
 {
     if (!pm || core_id >= UIOX_CPU_MAX_CORES) return -EINVAL;
     pm->idle_state[core_id] = state;
     uiox_percpu[core_id].state = UIOX_CPU_STATE_IDLE;
     switch (state) {
     case UIOX_CPU_IDLE_C1:
 #if defined(UIOX_ARCH_ARM64)
         uiox_cpu_wfi();
 #elif defined(UIOX_ARCH_X86_64)
         uiox_cpu_hlt();
 #elif defined(UIOX_ARCH_RV64)
         uiox_cpu_wfi();
 #endif
         break;
     case UIOX_CPU_IDLE_C2:
     case UIOX_CPU_IDLE_C3:
         /* Deep sleep — platform-specific power controller call */
         uiox_cpu_wfi();
         break;
     default:
         break;
     }
     return 0;
 }
 
 void uiox_cpu_pm_exit_idle(uiox_cpu_pm_t *pm, uint8_t core_id)
 {
     if (!pm || core_id >= UIOX_CPU_MAX_CORES) return;
     pm->idle_state[core_id]        = UIOX_CPU_IDLE_C0;
     uiox_percpu[core_id].state     = UIOX_CPU_STATE_RUNNING;
 }
 
 void uiox_cpu_pm_update_load(uiox_cpu_pm_t *pm, uint8_t core_id,
                                uint32_t load_pct)
 {
     if (!pm || core_id >= UIOX_CPU_MAX_CORES) return;
     if (load_pct > 100u) load_pct = 100u;
     pm->load_pct[core_id] = load_pct;
 }
 
 void uiox_cpu_pm_tick(uiox_cpu_pm_t *pm, uint32_t now_ms)
 {
     if (!pm) return;
     (void)now_ms;
 
     uint8_t num = pm->cif->hw->num_cores;
 
     for (uint8_t i = 0; i < num; i++) {
         /* Read temperature */
         int8_t temp = 0;
         uiox_cpu_hw_read_temp(pm->cif->hw, i, &temp);
         pm->temp[i] = temp;
 
         /* Thermal throttle */
         if (temp >= UIOX_CPU_THERMAL_LIMIT) {
             pm->thermal_limit_hit = true;
             pm->throttle_count++;
             /* Force down to lowest OPP */
             if (pm->num_opps > 0)
                 uiox_cpu_pm_set_opp(pm, i, 0u);
             continue;
         }
         pm->thermal_limit_hit = false;
 
         /* DVFS governor */
         if (pm->num_opps == 0) continue;
         uint8_t target_opp = pm->cur_opp[i];
 
         switch (pm->governor) {
         case UIOX_CPU_GOV_PERFORMANCE:
             target_opp = pm->num_opps - 1u;
             break;
         case UIOX_CPU_GOV_POWERSAVE:
             target_opp = 0u;
             break;
         case UIOX_CPU_GOV_ONDEMAND:
         case UIOX_CPU_GOV_SCHEDUTIL: {
             uint32_t load = pm->load_pct[i];
             if (load > 80u && target_opp < pm->num_opps - 1u)
                 target_opp++;
             else if (load < 20u && target_opp > 0u)
                 target_opp--;
             break;
         }
         case UIOX_CPU_GOV_CONSERVATIVE:
             if (pm->load_pct[i] > 70u && target_opp < pm->num_opps - 1u)
                 target_opp++;
             else if (pm->load_pct[i] < 30u && target_opp > 0u)
                 target_opp--;
             break;
         }
 
         if (target_opp != pm->cur_opp[i])
             uiox_cpu_pm_set_opp(pm, i, target_opp);
     }
 }
 