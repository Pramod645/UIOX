/**
 * @file    uiox_therm_policy.c
 * @brief   UIOX Thermal policy implementation.
 * @date    2026-06-05
 */

 #include "uiox_therm_policy.h"
 #include <string.h>
 #include <errno.h>
 
 int uiox_therm_policy_init(uiox_therm_policy_t     *pol,
                             uiox_therm_sensor_mgr_t *mgr)
 {
     if (!pol || !mgr) return -EINVAL;
     memset(pol, 0, sizeof(*pol));
     pol->mgr = mgr;
     return 0;
 }
 
 int uiox_therm_policy_add_zone(uiox_therm_policy_t     *pol,
                                 const uiox_therm_zone_t *zone)
 {
     if (!pol || !zone) return -EINVAL;
     if (pol->num_zones >= UIOX_THERM_MAX_ZONES) return -ENOSPC;
     memcpy(&pol->zones[pol->num_zones++], zone, sizeof(*zone));
     return 0;
 }
 
 void uiox_therm_policy_tick(uiox_therm_policy_t *pol, uint32_t now_ms)
 {
     if (!pol) return;
     (void)now_ms;
 
     for (uint8_t z = 0; z < pol->num_zones; z++) {
         uiox_therm_zone_t *zone = &pol->zones[z];
         if (!zone->active || !zone->sensor_name) continue;
 
         zone->cur_dc = uiox_therm_sensor_get(pol->mgr, zone->sensor_name);
         if (zone->cur_dc == INT16_MIN) continue;
 
         /* Evaluate each trip point in ascending temperature order */
         for (uint8_t t = 0; t < zone->num_trips; t++) {
             uiox_therm_trip_t *trip = &zone->trips[t];
             bool was_active = trip->active;
 
             /* Crossing up */
             if (!trip->active && zone->cur_dc >= trip->temp_dc) {
                 trip->active = true;
 
                 /* Push event */
                 uiox_therm_ev_t ev_type;
                 switch (trip->type) {
                 case UIOX_THERM_TRIP_PASSIVE:
                     ev_type = UIOX_THERM_EV_THROTTLE_ON;
                     pol->throttled = true;
                     pol->throttle_count++;
                     break;
                 case UIOX_THERM_TRIP_HOT:
                     ev_type = UIOX_THERM_EV_ZONE_HOT;
                     break;
                 case UIOX_THERM_TRIP_CRITICAL:
                     ev_type = UIOX_THERM_EV_CRITICAL;
                     pol->emergency = true;
                     pol->emergency_count++;
                     break;
                 default:
                     ev_type = UIOX_THERM_EV_TRIP_CROSSED;
                     break;
                 }
                 uiox_therm_event_t ev = {
                     .type         = ev_type,
                     .zone_id      = zone->zone_id,
                     .temp_dc      = zone->cur_dc,
                     .threshold_dc = trip->temp_dc,
                     .valid        = true,
                 };
                 uiox_therm_event_push(&ev);
 
                 if (trip->action)
                     trip->action(zone->zone_id, trip->type,
                                  true, trip->action_ctx);
             }
             /* Clearing (hysteresis) */
             else if (trip->active &&
                      zone->cur_dc < (trip->temp_dc - trip->hyst_dc)) {
                 trip->active = false;
 
                 uiox_therm_ev_t ev_type;
                 switch (trip->type) {
                 case UIOX_THERM_TRIP_PASSIVE:
                     ev_type = UIOX_THERM_EV_THROTTLE_OFF;
                     pol->throttled = false;
                     break;
                 case UIOX_THERM_TRIP_HOT:
                     ev_type = UIOX_THERM_EV_ZONE_COOL;
                     break;
                 default:
                     ev_type = UIOX_THERM_EV_TRIP_CLEARED;
                     break;
                 }
                 uiox_therm_event_t ev = {
                     .type         = ev_type,
                     .zone_id      = zone->zone_id,
                     .temp_dc      = zone->cur_dc,
                     .threshold_dc = trip->temp_dc,
                     .valid        = true,
                 };
                 uiox_therm_event_push(&ev);
 
                 if (trip->action)
                     trip->action(zone->zone_id, trip->type,
                                  false, trip->action_ctx);
             }
             (void)was_active;
         }
     }
 }
 