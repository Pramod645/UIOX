/**
 * @file    uiox_pmic_rail.c
 * @brief   UIOX PMIC rail manager implementation.
 * @date    2026-06-04
 */

 #include "uiox_pmic_rail.h"
 
 int uiox_pmic_rail_init(uiox_pmic_rail_mgr_t *mgr, uiox_pmic_if_t *pif)
 {
     if (!mgr || !pif) return -EINVAL;
     memset(mgr, 0, sizeof(*mgr));
     mgr->pif = pif;
     return 0;
 }
 
 int uiox_pmic_rail_register(uiox_pmic_rail_mgr_t *mgr,
                               const uiox_pmic_rail_t *rail)
 {
     if (!mgr || !rail) return -EINVAL;
     if (mgr->num_rails >= UIOX_PMIC_MAX_RAIL_COUNT) return -ENOSPC;
     memcpy(&mgr->rails[mgr->num_rails++], rail, sizeof(*rail));
     return 0;
 }
 
 uiox_pmic_rail_t *uiox_pmic_rail_find(uiox_pmic_rail_mgr_t *mgr,
                                        const char *name)
 {
     if (!mgr || !name) return NULL;
     for (uint8_t i = 0; i < mgr->num_rails; i++)
         if (strncmp(mgr->rails[i].name, name, UIOX_PMIC_RAIL_NAME_MAX) == 0)
             return &mgr->rails[i];
     return NULL;
 }
 
 static uint8_t mv_to_reg(const uiox_pmic_rail_t *r, uint32_t mv)
 {
     if (mv < r->min_mv) mv = r->min_mv;
     if (mv > r->max_mv) mv = r->max_mv;
     uint32_t steps = (mv - r->min_mv) / r->step_mv;
     return (uint8_t)(steps & r->vset_mask);
 }
 
 int uiox_pmic_rail_enable(uiox_pmic_rail_mgr_t *mgr, const char *name)
 {
     uiox_pmic_rail_t *r = uiox_pmic_rail_find(mgr, name);
     if (!r) return -ENOENT;
     r->consumers++;
     if (r->enabled) return 0;
 
     /* Write enable bit to control register */
     int rc = uiox_pmic_hw_reg_update(mgr->pif->hw,
                                       r->ctrl_reg,
                                       (uint8_t)(1u << r->en_bit),
                                       (uint8_t)(1u << r->en_bit));
     if (rc == 0) {
         r->enabled = true;
         uiox_pmic_event_t ev = {
             .type    = UIOX_PMIC_EV_RAIL_ON,
             .rail_id = r->rail_id,
             .mv      = r->cur_mv,
             .valid   = true,
         };
         uiox_pmic_event_push(&ev);
     }
     return rc;
 }
 
 int uiox_pmic_rail_disable(uiox_pmic_rail_mgr_t *mgr, const char *name)
 {
     uiox_pmic_rail_t *r = uiox_pmic_rail_find(mgr, name);
     if (!r) return -ENOENT;
     if (r->always_on) return -EPERM;
     if (r->consumers > 0) r->consumers--;
     if (r->consumers > 0 || !r->enabled) return 0;
 
     int rc = uiox_pmic_hw_reg_update(mgr->pif->hw,
                                       r->ctrl_reg,
                                       (uint8_t)(1u << r->en_bit),
                                       0u);
     if (rc == 0) {
         r->enabled = false;
         uiox_pmic_event_t ev = {
             .type    = UIOX_PMIC_EV_RAIL_OFF,
             .rail_id = r->rail_id,
             .mv      = r->cur_mv,
             .valid   = true,
         };
         uiox_pmic_event_push(&ev);
     }
     return rc;
 }
 
 int uiox_pmic_rail_set_mv(uiox_pmic_rail_mgr_t *mgr,
                            const char *name, uint32_t mv)
 {
     uiox_pmic_rail_t *r = uiox_pmic_rail_find(mgr, name);
     if (!r) return -ENOENT;
     if (mv < r->min_mv || mv > r->max_mv) return -ERANGE;
 
     uint8_t reg_val = mv_to_reg(r, mv);
     int rc = uiox_pmic_hw_reg_update(mgr->pif->hw,
                                       r->vset_reg, r->vset_mask, reg_val);
     if (rc == 0) {
         uint32_t old_mv = r->cur_mv;
         r->cur_mv = r->min_mv + (uint32_t)reg_val * r->step_mv;
         uiox_pmic_event_t ev = {
             .type    = (r->cur_mv > old_mv) ?
                        UIOX_PMIC_EV_DVFS_UP : UIOX_PMIC_EV_DVFS_DOWN,
             .rail_id = r->rail_id,
             .mv      = r->cur_mv,
             .valid   = true,
         };
         uiox_pmic_event_push(&ev);
     }
     return rc;
 }
 
 int uiox_pmic_rail_get_mv(const uiox_pmic_rail_mgr_t *mgr,
                            const char *name, uint32_t *mv_out)
 {
     if (!mgr || !name || !mv_out) return -EINVAL;
     for (uint8_t i = 0; i < mgr->num_rails; i++) {
         if (strncmp(mgr->rails[i].name, name, UIOX_PMIC_RAIL_NAME_MAX) == 0) {
             *mv_out = mgr->rails[i].cur_mv;
             return 0;
         }
     }
     return -ENOENT;
 }
 
 void uiox_pmic_rail_restore_all(uiox_pmic_rail_mgr_t *mgr)
 {
     if (!mgr) return;
     for (uint8_t i = 0; i < mgr->num_rails; i++) {
         uiox_pmic_rail_t *r = &mgr->rails[i];
         if (r->boot_mv) uiox_pmic_rail_set_mv(mgr, r->name, r->boot_mv);
     }
 }
 