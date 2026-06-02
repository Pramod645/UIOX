/**
 * @file    uiox_tpwd_device.c
 * @brief   UIOX Touch-Password device API implementation.
 * @date    2026-06-01
 */

 #include "uiox_tpwd_device.h"
 #include <string.h>
 #include <errno.h>
 #include <stdio.h>
 
 int uiox_tpwd_open(uiox_tpwd_device_t           *dev,
                     const uiox_tpwd_open_params_t *p)
 {
     if (!dev || !p || !p->hw || !p->hw_ops) return -EINVAL;
     memset(dev, 0, sizeof(*dev));
     dev->hw = p->hw;
 
     int rc = uiox_tpwd_hw_init(p->hw, p->hw_ops);
     if (rc < 0) return rc;
 
     rc = uiox_tpwd_subsys_init(&dev->subsys,
                                 p->hw,
                                 &p->gesture,
                                 p->debounce_ms,
                                 p->hold_ms,
                                 p->timeout_ms,
                                 p->rng_seed);
     if (rc < 0) return rc;
 
     if (p->evt_cb)
         uiox_tpwd_subsys_set_cb(&dev->subsys, p->evt_cb, p->evt_ctx);
 
     dev->open = true;
     return 0;
 }
 
 int uiox_tpwd_start(uiox_tpwd_device_t *dev)
 {
     if (!dev || !dev->open) return -EINVAL;
     int rc = uiox_tpwd_hw_power(dev->hw, true);
     if (rc < 0) return rc;
     rc = uiox_tpwd_hw_reset(dev->hw);
     return rc;
 }
 
 void uiox_tpwd_stop(uiox_tpwd_device_t *dev)
 {
     if (!dev || !dev->open) return;
     uiox_tpwd_hw_set_backlight(dev->hw, 0u);
     uiox_tpwd_hw_power(dev->hw, false);
 }
 
 void uiox_tpwd_close(uiox_tpwd_device_t *dev)
 {
     if (!dev || !dev->open) return;
     uiox_tpwd_stop(dev);
     uiox_tpwd_hw_deinit(dev->hw);
     /* Zero security context */
     uiox_tpwd_sec_zero(&dev->subsys.sec, sizeof(dev->subsys.sec));
     dev->open = false;
 }
 
 void uiox_tpwd_tick(uiox_tpwd_device_t *dev,
                      uint32_t now_ms, uint32_t now_s)
 {
     if (!dev || !dev->open) return;
     uiox_tpwd_subsys_tick(&dev->subsys, now_ms, now_s);
 }
 
 int uiox_tpwd_enrol(uiox_tpwd_device_t *dev,
                      const char *id, uint32_t now_ms)
 {
     if (!dev || !dev->open) return -EINVAL;
     return uiox_tpwd_subsys_enrol_start(&dev->subsys, id, now_ms);
 }
 
 int uiox_tpwd_verify(uiox_tpwd_device_t *dev,
                       const char *id, uint32_t now_ms)
 {
     if (!dev || !dev->open) return -EINVAL;
     return uiox_tpwd_subsys_verify_start(&dev->subsys, id, now_ms);
 }
 
 int uiox_tpwd_delete(uiox_tpwd_device_t *dev, const char *id)
 {
     if (!dev || !dev->open) return -EINVAL;
     return uiox_tpwd_sec_delete(&dev->subsys.sec, id);
 }
 
 void uiox_tpwd_logout(uiox_tpwd_device_t *dev)
 {
     if (!dev || !dev->open) return;
     uiox_tpwd_subsys_logout(&dev->subsys);
 }
 
 bool uiox_tpwd_authenticated(const uiox_tpwd_device_t *dev, uint32_t now_s)
 {
     if (!dev || !dev->open) return false;
     return dev->subsys.state == UIOX_TPWD_SUBSYS_AUTHENTICATED &&
            uiox_tpwd_sec_token_valid(&dev->subsys.sec, now_s);
 }
 
 uiox_tpwd_subsys_state_t uiox_tpwd_state(const uiox_tpwd_device_t *dev)
 {
     if (!dev || !dev->open) return UIOX_TPWD_SUBSYS_IDLE;
     return uiox_tpwd_subsys_state(&dev->subsys);
 }
 
 uint8_t uiox_tpwd_attempts_left(const uiox_tpwd_device_t *dev,
                                   const char *id)
 {
     if (!dev || !dev->open) return 0u;
     return uiox_tpwd_subsys_attempts_left(&dev->subsys, id);
 }
 
 int uiox_tpwd_set_backlight(uiox_tpwd_device_t *dev, uint8_t level)
 {
     if (!dev || !dev->open) return -EINVAL;
     return uiox_tpwd_hw_set_backlight(dev->hw, level);
 }
 
 void uiox_tpwd_print_audit(const uiox_tpwd_device_t *dev)
 {
     if (!dev) return;
     uiox_tpwd_subsys_print_audit(&dev->subsys);
 }
 
 void uiox_tpwd_print_stats(const uiox_tpwd_device_t *dev)
 {
     if (!dev) return;
     uiox_tpwd_if_stats_t s;
     uiox_tpwd_if_stats_get(&dev->subsys.tif, &s);
     printf("  Scan cycles       : %u\n",   s.scan_count);
     printf("  Total touches     : %llu\n", (unsigned long long)s.total_touches);
     printf("  Total releases    : %llu\n", (unsigned long long)s.total_releases);
     printf("  Debounce filtered : %llu\n", (unsigned long long)s.debounce_filtered);
     printf("  Event overflows   : %llu\n", (unsigned long long)s.overflow_events);
     printf("  Credentials stored: %u / %u\n",
            dev->subsys.sec.record_count, UIOX_TPWD_SEC_MAX_STORED);
     printf("  Session valid     : %s\n",
            dev->subsys.sec.session_valid ? "YES" : "NO");
     printf("  State             : %s\n",
            uiox_tpwd_state_name(dev->subsys.state));
 }
 
 const char *uiox_tpwd_state_name(uiox_tpwd_subsys_state_t s)
 {
     switch (s) {
     case UIOX_TPWD_SUBSYS_IDLE:          return "IDLE";
     case UIOX_TPWD_SUBSYS_ENROLLING:     return "ENROLLING";
     case UIOX_TPWD_SUBSYS_VERIFYING:     return "VERIFYING";
     case UIOX_TPWD_SUBSYS_LOCKED:        return "LOCKED";
     case UIOX_TPWD_SUBSYS_AUTHENTICATED: return "AUTHENTICATED";
     default:                              return "UNKNOWN";
     }
 }
 
 const char *uiox_tpwd_evt_name(uiox_tpwd_evt_t evt)
 {
     switch (evt) {
     case UIOX_TPWD_EVT_AUTH_OK:   return "AUTH_OK";
     case UIOX_TPWD_EVT_AUTH_FAIL: return "AUTH_FAIL";
     case UIOX_TPWD_EVT_LOCKED:    return "LOCKED";
     case UIOX_TPWD_EVT_TIMEOUT:   return "TIMEOUT";
     case UIOX_TPWD_EVT_CANCELLED: return "CANCELLED";
     case UIOX_TPWD_EVT_ENROLLED:  return "ENROLLED";
     case UIOX_TPWD_EVT_DIGIT:     return "DIGIT";
     case UIOX_TPWD_EVT_BACKSPACE: return "BACKSPACE";
     case UIOX_TPWD_EVT_LOGOUT:    return "LOGOUT";
     default:                       return "UNKNOWN";
     }
 }
 