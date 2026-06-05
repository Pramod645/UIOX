/**
 * @file    uiox_fan_thermal.c
 * @brief   UIOX Fan thermal engine implementation.
 * @date    2026-06-05
 */

 #include "uiox_fan_thermal.h"
 #include <string.h>
 #include <errno.h>
 
 int uiox_fan_thermal_init(uiox_fan_thermal_t *th,
                            uiox_fan_drv_t     *drv,
                            int16_t             critical_temp_dc)
 {
     if (!th || !drv) return -EINVAL;
     memset(th, 0, sizeof(*th));
     th->drv              = drv;
     th->critical_temp_dc = critical_temp_dc;
     return 0;
 }
 
 int uiox_fan_thermal_add_zone(uiox_fan_thermal_t *th,
                                const uiox_fan_zone_t *zone)
 {
     if (!th || !zone) return -EINVAL;
     if (th->num_zones >= UIOX_FAN_TEMP_ZONE_MAX) return -ENOSPC;
     memcpy(&th->zones[th->num_zones++], zone, sizeof(*zone));
     return 0;
 }
 
 void uiox_fan_pid_init(uiox_fan_pid_t *pid,
                         float kp, float ki, float kd,
                         int16_t setpoint_dc,
                         float out_min, float out_max)
 {
     if (!pid) return;
     pid->kp             = kp;
     pid->ki             = ki;
     pid->kd             = kd;
     pid->setpoint_dc    = setpoint_dc;
     pid->integral       = 0.0f;
     pid->prev_error     = 0.0f;
     pid->output_min     = out_min;
     pid->output_max     = out_max;
     pid->integral_limit = out_max;
 }
 
 uint8_t uiox_fan_pid_update(uiox_fan_pid_t *pid,
                              int16_t measured_dc, float dt_s)
 {
     if (!pid || dt_s <= 0.0f) return 0u;
     float error     = (float)(measured_dc - pid->setpoint_dc);
     pid->integral  += error * dt_s;
     /* Anti-windup */
     if (pid->integral >  pid->integral_limit) pid->integral =  pid->integral_limit;
     if (pid->integral < -pid->integral_limit) pid->integral = -pid->integral_limit;
     float derivative = (error - pid->prev_error) / dt_s;
     pid->prev_error  = error;
     float output = pid->kp * error +
                    pid->ki * pid->integral +
                    pid->kd * derivative;
     if (output < pid->output_min) output = pid->output_min;
     if (output > pid->output_max) output = pid->output_max;
     return (uint8_t)output;
 }
 
 /* Step-table lookup: binary search for temperature threshold */
 static uint8_t step_lookup(const uiox_fan_trip_t *trips,
                              uint8_t n, int16_t temp_dc)
 {
     uint8_t duty = trips[0].duty;
     for (uint8_t i = 0; i < n; i++) {
         if (temp_dc >= trips[i].temp_dc)
             duty = trips[i].duty;
         else
             break;
     }
     return duty;
 }
 
 void uiox_fan_thermal_tick(uiox_fan_thermal_t *th, uint32_t now_ms)
 {
     if (!th) return;
     static uint32_t s_last_ms = 0;
     float dt_s = (s_last_ms ? (float)(now_ms - s_last_ms) / 1000.0f : 0.1f);
     if (dt_s > 10.0f) dt_s = 0.1f;
     s_last_ms = now_ms;
 
     bool any_critical = false;
 
     for (uint8_t z = 0; z < th->num_zones; z++) {
         uiox_fan_zone_t *zone = &th->zones[z];
         if (!zone->active) continue;
 
         /* Read temperature */
         zone->cur_temp_dc =
             th->drv->fif->hw->temp_dc[zone->temp_sensor_id];
 
         /* Emergency: critical temperature */
         if (zone->cur_temp_dc >= th->critical_temp_dc) {
             any_critical = true;
             uiox_fan_drv_set_duty(th->drv, zone->fan_id,
                                    UIOX_FAN_PWM_MAX, now_ms);
             if (!th->emergency) {
                 th->emergency = true;
                 uiox_fan_event_t ev = {
                     .type     = UIOX_FAN_EV_OVERHEAT,
                     .fan_id   = zone->fan_id,
                     .temp_dc  = zone->cur_temp_dc,
                     .ts_ms    = now_ms,
                     .valid    = true,
                 };
                 uiox_fan_event_push(&ev);
             }
             continue;
         }
 
         /* Check if temp dropped below critical */
         if (th->emergency && zone->cur_temp_dc < th->critical_temp_dc - 100) {
             th->emergency = false;
             uiox_fan_event_t ev = {
                 .type    = UIOX_FAN_EV_TEMP_OK,
                 .fan_id  = zone->fan_id,
                 .temp_dc = zone->cur_temp_dc,
                 .ts_ms   = now_ms,
                 .valid   = true,
             };
             uiox_fan_event_push(&ev);
         }
 
         /* Compute output duty based on controller type */
         uint8_t duty = zone->cur_duty;
         switch (zone->ctrl_type) {
         case UIOX_FAN_CTRL_STEP:
             duty = step_lookup(zone->trips, zone->num_trips,
                                zone->cur_temp_dc);
             break;
         case UIOX_FAN_CTRL_HYSTERESIS:
             if (zone->cur_temp_dc >= zone->hyst_on_dc)
                 duty = UIOX_FAN_PWM_MAX;
             else if (zone->cur_temp_dc < zone->hyst_off_dc)
                 duty = 0u;
             break;
         case UIOX_FAN_CTRL_PID:
             duty = uiox_fan_pid_update(&zone->pid,
                                         zone->cur_temp_dc, dt_s);
             break;
         default:
             break;
         }
 
         if (duty != zone->cur_duty) {
             zone->cur_duty = duty;
             uiox_fan_drv_set_duty(th->drv, zone->fan_id, duty, now_ms);
         }
     }
     (void)any_critical;
 }
 