/**
 * @file    uiox_fan_if.c
 * @brief   UIOX Fan Controller interface driver implementation.
 * @date    2026-06-05
 */

 #include "uiox_fan_if.h"
 
 int uiox_fan_if_config(uiox_fan_if_t *fif, uiox_fan_hw_t *hw)
 {
     if (!fif || !hw) return -EINVAL;
     memset(fif, 0, sizeof(*fif));
     fif->hw     = hw;
     fif->primed = true;
     uiox_fan_buf_init();
     return 0;
 }
 
 int uiox_fan_if_start(uiox_fan_if_t *fif)
 {
     if (!fif || !fif->primed) return -EINVAL;
 
     /* Read device/manufacturer IDs */
     const uiox_fan_hw_ops_t *ops =
         (const uiox_fan_hw_ops_t *)fif->hw->priv;
     if (ops && ops->reg_read) {
         ops->reg_read(fif->hw, UIOX_REG_DEVICE_ID, &fif->device_id);
         ops->reg_read(fif->hw, UIOX_REG_MFR_ID,    &fif->mfr_id);
     }
 
     /* Configure each fan channel */
     for (uint8_t i = 0; i < fif->hw->num_fans; i++) {
         uiox_fan_hw_chan_enable(fif->hw, i, true);
         /* Set minimum PWM (20%) at startup to prevent inrush */
         uiox_fan_hw_set_pwm(fif->hw, i, 51u);
     }
 
     /* Unmask stall + drive-fail interrupts */
     if (ops && ops->reg_write)
         ops->reg_write(fif->hw, UIOX_REG_INTERRUPT_ENABLE, 0x03u);
 
     return 0;
 }
 
 void uiox_fan_if_stop(uiox_fan_if_t *fif)
 {
     if (!fif) return;
     /* Ramp down fans before stopping */
     for (uint8_t i = 0; i < fif->hw->num_fans; i++)
         uiox_fan_hw_set_pwm(fif->hw, i, 0u);
 }
 
 int uiox_fan_if_measure(uiox_fan_if_t *fif)
 {
     if (!fif || !fif->primed) return -EINVAL;
     fif->stats.measurements++;
     int rc = 0;
 
     for (uint8_t i = 0; i < fif->hw->num_fans; i++) {
         uint16_t rpm = 0;
         int r = uiox_fan_hw_read_rpm(fif->hw, i, &rpm);
         if (r < 0) { fif->stats.comm_errors++; rc = r; }
     }
     for (uint8_t i = 0; i < fif->hw->num_temps; i++) {
         int16_t temp = 0;
         int r = uiox_fan_hw_read_temp(fif->hw, i, &temp);
         if (r < 0) fif->stats.comm_errors++;
     }
     return rc;
 }
 
 int uiox_fan_if_set_pwm(uiox_fan_if_t *fif, uint8_t ch,
                          uint8_t duty, uint32_t now_ms)
 {
     if (!fif || ch >= fif->hw->num_fans) return -EINVAL;
     uint8_t old = fif->hw->chan[ch].pwm_duty;
     int rc = uiox_fan_hw_set_pwm(fif->hw, ch, duty);
     if (rc == 0 && duty != old) {
         fif->stats.pwm_changes++;
         uiox_fan_event_t ev = {
             .type     = UIOX_FAN_EV_PWM_CHANGE,
             .fan_id   = ch,
             .pwm_duty = duty,
             .rpm      = fif->hw->chan[ch].rpm_measured,
             .ts_ms    = now_ms,
             .valid    = true,
         };
         uiox_fan_event_push(&ev);
     }
     return rc;
 }
 
 int uiox_fan_if_irq_handle(uiox_fan_if_t *fif, uint32_t now_ms)
 {
     if (!fif) return -EINVAL;
     fif->stats.irq_count++;
 
     uint32_t flags = 0;
     int rc = uiox_fan_hw_fault_status(fif->hw, &flags);
     if (rc < 0 || !flags) return rc;
 
     uiox_fan_hw_fault_clear(fif->hw, flags);
 
     static const struct { uint32_t bit; uiox_fan_ev_t ev; }
     fault_map[] = {
         { UIOX_FAN_FAULT_STALL,        UIOX_FAN_EV_STALL        },
         { UIOX_FAN_FAULT_DRIVE_FAIL,   UIOX_FAN_EV_FAULT        },
         { UIOX_FAN_FAULT_SPIN_UP_FAIL, UIOX_FAN_EV_SPIN_UP_FAIL },
         { UIOX_FAN_FAULT_OTP,          UIOX_FAN_EV_OVERHEAT     },
         { UIOX_FAN_FAULT_WATCHDOG,     UIOX_FAN_EV_WATCHDOG     },
     };
     for (size_t i = 0; i < sizeof(fault_map)/sizeof(fault_map[0]); i++) {
         if (flags & fault_map[i].bit) {
             uiox_fan_event_t ev = {
                 .type        = fault_map[i].ev,
                 .fan_id      = 0xFFu,
                 .ts_ms       = now_ms,
                 .fault_flags = flags,
                 .valid       = true,
             };
             uiox_fan_event_push(&ev);
             fif->stats.fault_count++;
             if (flags & UIOX_FAN_FAULT_STALL)
                 fif->stats.stall_count++;
         }
     }
     return (int)flags;
 }
 
 int uiox_fan_if_telemetry(uiox_fan_if_t *fif,
                            uiox_fan_telem_t *out, uint32_t now_ms)
 {
     if (!fif || !out) return -EINVAL;
     out->ts_ms       = now_ms;
     out->fault_flags = fif->hw->global_fault;
     for (uint8_t i = 0; i < fif->hw->num_fans; i++) {
         out->rpm[i] = fif->hw->chan[i].rpm_measured;
         out->pwm[i] = fif->hw->chan[i].pwm_duty;
     }
     for (uint8_t i = 0; i < fif->hw->num_temps; i++)
         out->temp_dc[i] = fif->hw->temp_dc[i];
     return 0;
 }
 
 void uiox_fan_if_stats_get(const uiox_fan_if_t *fif,
                             uiox_fan_if_stats_t *out)
 { if (!fif || !out) return; memcpy(out, &fif->stats, sizeof(*out)); }
 
 void uiox_fan_if_stats_reset(uiox_fan_if_t *fif)
 { if (!fif) return; memset(&fif->stats, 0, sizeof(fif->stats)); }
 