/**
 * @file    uiox_pmic_policy.c
 * @brief   UIOX PMIC power policy implementation.
 * @date    2026-06-04
 */

 #include "uiox_pmic_policy.h"
 #include <string.h>
 #include <errno.h>
 
 int uiox_pmic_policy_init(uiox_pmic_policy_t      *pol,
                            uiox_pmic_rail_mgr_t    *mgr,
                            const uiox_pmic_thermal_cfg_t *thermal,
                            const char *vcore, const char *vmem,
                            const char *vio)
 {
     if (!pol || !mgr || !thermal) return -EINVAL;
     memset(pol, 0, sizeof(*pol));
     pol->mgr        = mgr;
     pol->current_ps = UIOX_PMIC_PS_ACTIVE;
     pol->vcore_rail = vcore;
     pol->vmem_rail  = vmem;
     pol->vio_rail   = vio;
     memcpy(&pol->thermal, thermal, sizeof(*thermal));
     return 0;
 }
 
 int uiox_pmic_policy_add_opp(uiox_pmic_policy_t *pol,
                                uint32_t cpu_mhz, uint32_t vcore_mv,
                                uint32_t power_mw)
 {
     if (!pol || pol->num_opps >= UIOX_PMIC_MAX_OPP) return -ENOSPC;
     /* Insert sorted by frequency ascending */
     uint8_t pos = pol->num_opps;
     for (uint8_t i = 0; i < pol->num_opps; i++) {
         if (cpu_mhz < pol->opps[i].cpu_freq_mhz) { pos = i; break; }
     }
     for (uint8_t i = pol->num_opps; i > pos; i--)
         pol->opps[i] = pol->opps[i-1];
     pol->opps[pos].cpu_freq_mhz = cpu_mhz;
     pol->opps[pos].vcore_mv     = vcore_mv;
     pol->opps[pos].power_mw     = power_mw;
     pol->num_opps++;
     return 0;
 }
 
 int uiox_pmic_policy_set_opp(uiox_pmic_policy_t *pol, uint8_t opp_idx)
 {
     if (!pol || opp_idx >= pol->num_opps) return -EINVAL;
     pol->cur_opp = opp_idx;
     if (pol->vcore_rail)
         return uiox_pmic_rail_set_mv(pol->mgr, pol->vcore_rail,
                                       pol->opps[opp_idx].vcore_mv);
     return 0;
 }
 
 int uiox_pmic_policy_set_ps(uiox_pmic_policy_t *pol, uiox_pmic_ps_t ps)
 {
     if (!pol) return -EINVAL;
     pol->current_ps = ps;
     switch (ps) {
     case UIOX_PMIC_PS_ACTIVE:
         if (pol->num_opps)
             uiox_pmic_policy_set_opp(pol, pol->num_opps - 1u);
         break;
     case UIOX_PMIC_PS_BALANCED:
         if (pol->num_opps > 1)
             uiox_pmic_policy_set_opp(pol, pol->num_opps / 2u);
         break;
     case UIOX_PMIC_PS_POWERSAVE:
         uiox_pmic_policy_set_opp(pol, 0u);
         break;
     case UIOX_PMIC_PS_SLEEP:
         return uiox_pmic_policy_sleep(pol);
     default:
         break;
     }
     return 0;
 }
 
 void uiox_pmic_policy_update_load(uiox_pmic_policy_t *pol,
                                     uint32_t load_pct, uint32_t now_ms)
 {
     if (!pol || pol->num_opps == 0) return;
     (void)now_ms;
     pol->cpu_load_pct = load_pct;
     if (pol->throttled) return;
 
     uint8_t target = pol->cur_opp;
     if (load_pct > 80u && target < pol->num_opps - 1u) target++;
     else if (load_pct < 20u && target > 0u)             target--;
 
     if (target != pol->cur_opp)
         uiox_pmic_policy_set_opp(pol, target);
 }
 
 void uiox_pmic_policy_thermal_tick(uiox_pmic_policy_t *pol, int8_t temp_c)
 {
     if (!pol) return;
 
     if (temp_c >= pol->thermal.critical_temp_c) {
         /* Emergency: power off */
         uiox_pmic_policy_set_ps(pol, UIOX_PMIC_PS_SHUTDOWN);
         return;
     }
 
     if (temp_c >= pol->thermal.throttle_temp_c && !pol->throttled) {
         pol->throttled = true;
         /* Force lowest OPP */
         uiox_pmic_policy_set_opp(pol, 0u);
         if (pol->vcore_rail && pol->thermal.throttle_mv)
             uiox_pmic_rail_set_mv(pol->mgr, pol->vcore_rail,
                                    pol->thermal.throttle_mv);
     } else if (temp_c <= pol->thermal.resume_temp_c && pol->throttled) {
         pol->throttled = false;
     }
 }
 
 int uiox_pmic_policy_sleep(uiox_pmic_policy_t *pol)
 {
     if (!pol) return -EINVAL;
     /* Disable I/O rail — keep VCORE and VMEM at retention voltage */
     if (pol->vio_rail)
         uiox_pmic_rail_disable(pol->mgr, pol->vio_rail);
     if (pol->vcore_rail && pol->num_opps > 0)
         uiox_pmic_rail_set_mv(pol->mgr, pol->vcore_rail,
                                pol->opps[0].vcore_mv);
     uiox_pmic_event_t ev = { .type = UIOX_PMIC_EV_SLEEP, .valid = true };
     uiox_pmic_event_push(&ev);
     return 0;
 }
 
 int uiox_pmic_policy_wake(uiox_pmic_policy_t *pol)
 {
     if (!pol) return -EINVAL;
     if (pol->vio_rail)
         uiox_pmic_rail_enable(pol->mgr, pol->vio_rail);
     uiox_pmic_rail_restore_all(pol->mgr);
     uiox_pmic_event_t ev = { .type = UIOX_PMIC_EV_WAKE, .valid = true };
     uiox_pmic_event_push(&ev);
     return 0;
 }
 