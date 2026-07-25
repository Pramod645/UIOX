/**
 * @file    uiox_fan_drv.c
 * @brief   UIOX Fan driver implementation.
 * @date    2026-06-05
 */

 #include "uiox_fan_drv.h"
 #include <string.h>
 #include <errno.h>
 
 int uiox_fan_drv_init(uiox_fan_drv_t *drv, uiox_fan_if_t *fif)
 {
     if (!drv || !fif) return -EINVAL;
     memset(drv, 0, sizeof(*drv));
     drv->fif = fif;
     return 0;
 }
 
 int uiox_fan_drv_register(uiox_fan_drv_t *drv, const uiox_fan_ch_t *ch)
 {
     if (!drv || !ch) return -EINVAL;
     if (drv->num_fans >= UIOX_FAN_MAX_FAN_COUNT) return -ENOSPC;
     memcpy(&drv->fans[drv->num_fans++], ch, sizeof(*ch));
     return 0;
 }
 
 uiox_fan_ch_t *uiox_fan_drv_find(uiox_fan_drv_t *drv, uint8_t fan_id)
 {
     if (!drv) return NULL;
     for (uint8_t i = 0; i < drv->num_fans; i++)
         if (drv->fans[i].fan_id == fan_id)
             return &drv->fans[i];
     return NULL;
 }
 
 int uiox_fan_drv_set_duty(uiox_fan_drv_t *drv, uint8_t fan_id,
                            uint8_t duty, uint32_t now_ms)
 {
     uiox_fan_ch_t *f = uiox_fan_drv_find(drv, fan_id);
     if (!f || f->manual) return -EINVAL;
 
     /* Clamp to rated range */
     if (duty < f->min_duty && duty > 0u) duty = f->min_duty;
     if (duty > f->max_duty) duty = f->max_duty;
 
     /* Spin-up: if fan is stopped and we want to spin, use spin-up duty */
     if (duty > 0u && !f->spinning_up && f->cur_rpm < UIOX_FAN_STALL_RPM_MIN
         && f->cur_duty == 0u) {
         f->spinning_up  = true;
         f->spin_up_ts_ms= now_ms;
         uiox_fan_if_set_pwm(drv->fif, fan_id,
                              UIOX_FAN_SPIN_UP_DUTY, now_ms);
         uiox_fan_event_t ev = {
             .type   = UIOX_FAN_EV_START,
             .fan_id = fan_id,
             .ts_ms  = now_ms,
             .valid  = true,
         };
         uiox_fan_event_push(&ev);
         return 0;
     }
 
     f->cur_duty = duty;
     return uiox_fan_if_set_pwm(drv->fif, fan_id, duty, now_ms);
 }
 
 int uiox_fan_drv_set_pct(uiox_fan_drv_t *drv, uint8_t fan_id,
                           uint8_t pct, uint32_t now_ms)
 {
     if (pct > 100u) pct = 100u;
     uint8_t duty = (uint8_t)((uint32_t)pct * UIOX_FAN_PWM_MAX / 100u);
     return uiox_fan_drv_set_duty(drv, fan_id, duty, now_ms);
 }
 
 void uiox_fan_drv_tick(uiox_fan_drv_t *drv, uint32_t now_ms)
 {
     if (!drv) return;
 
     for (uint8_t i = 0; i < drv->num_fans; i++) {
         uiox_fan_ch_t *f = &drv->fans[i];
         if (!f->enabled) continue;
 
         /* Update measured RPM */
         f->cur_rpm = drv->fif->hw->chan[f->fan_id].rpm_measured;
 
         /* Spin-up complete? */
         if (f->spinning_up) {
             if ((now_ms - f->spin_up_ts_ms) >= UIOX_FAN_SPIN_UP_MS) {
                 f->spinning_up = false;
                 /* Apply actual target duty after spin-up */
                 uiox_fan_if_set_pwm(drv->fif, f->fan_id,
                                      f->cur_duty, now_ms);
             }
             continue;
         }
 
         /* Stall detection */
         if (f->cur_duty > f->min_duty &&
             f->cur_rpm < UIOX_FAN_STALL_RPM_MIN) {
             if (!f->stalled) {
                 f->stalled     = true;
                 f->stall_ts_ms = now_ms;
                 uiox_fan_event_t ev = {
                     .type     = UIOX_FAN_EV_STALL,
                     .fan_id   = f->fan_id,
                     .pwm_duty = f->cur_duty,
                     .rpm      = f->cur_rpm,
                     .ts_ms    = now_ms,
                     .valid    = true,
                 };
                 uiox_fan_event_push(&ev);
             }
         } else if (f->stalled && f->cur_rpm >= UIOX_FAN_STALL_RPM_MIN) {
             f->stalled = false;
             uiox_fan_event_t ev = {
                 .type   = UIOX_FAN_EV_STALL_CLEAR,
                 .fan_id = f->fan_id,
                 .rpm    = f->cur_rpm,
                 .ts_ms  = now_ms,
                 .valid  = true,
             };
             uiox_fan_event_push(&ev);
         }
     }
 }
 
 void uiox_fan_drv_set_manual(uiox_fan_drv_t *drv, uint8_t fan_id,
                               bool manual, uint8_t duty, uint32_t now_ms)
 {
     uiox_fan_ch_t *f = uiox_fan_drv_find(drv, fan_id);
     if (!f) return;
     f->manual = manual;
     uiox_fan_event_t ev = {
         .type   = manual ? UIOX_FAN_EV_MANUAL_OVERRIDE
                          : UIOX_FAN_EV_AUTO_RESTORE,
         .fan_id = fan_id,
         .ts_ms  = now_ms,
         .valid  = true,
     };
     uiox_fan_event_push(&ev);
     if (manual)
         uiox_fan_if_set_pwm(drv->fif, fan_id, duty, now_ms);
 }
 