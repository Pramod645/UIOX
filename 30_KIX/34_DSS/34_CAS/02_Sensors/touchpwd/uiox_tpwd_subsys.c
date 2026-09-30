/**
 * @file    uiox_tpwd_subsys.c
 * @brief   UIOX Touch-Password subsystem implementation.
 * @date    2026-06-01
 */

 #include "uiox_tpwd_subsys.h"
 #include <string.h>
 #include <errno.h>
 #include <stdio.h>
 
 static void fire(uiox_tpwd_subsys_t *sys,
                  uiox_tpwd_evt_t evt, uint8_t remaining)
 {
     if (sys->evt_cb) sys->evt_cb(evt, remaining, sys->evt_ctx);
 }
 
 static void audit_push(uiox_tpwd_subsys_t *sys,
                         uiox_tpwd_evt_t evt, const char *id)
 {
     uiox_tpwd_audit_entry_t *e =
         &sys->audit[sys->audit_head % UIOX_TPWD_AUDIT_MAX];
     e->evt   = evt;
     e->ts_s  = sys->now_s;
     e->valid = true;
     if (id) strncpy(e->id, id, UIOX_TPWD_SEC_ID_LEN - 1);
     sys->audit_head++;
 }
 
 int uiox_tpwd_subsys_init(uiox_tpwd_subsys_t           *sys,
                            uiox_tpwd_hw_t               *hw,
                            const uiox_tpwd_gesture_cfg_t *gcfg,
                            uint32_t debounce_ms,
                            uint32_t hold_ms,
                            uint32_t timeout_ms,
                            uint32_t rng_seed)
 {
     if (!sys || !hw || !gcfg) return -EINVAL;
     memset(sys, 0, sizeof(*sys));
 
     uiox_tpwd_cred_pool_init();
     uiox_tpwd_evtbuf_init(&sys->gesture_rb);
 
     int rc = uiox_tpwd_if_config(&sys->tif, hw,
                                   debounce_ms, hold_ms, timeout_ms);
     if (rc < 0) return rc;
 
     rc = uiox_tpwd_gesture_init(&sys->gesture, &sys->gesture_rb, gcfg);
     if (rc < 0) return rc;
 
     rc = uiox_tpwd_sec_init(&sys->sec, rng_seed);
     if (rc < 0) return rc;
 
     sys->state = UIOX_TPWD_SUBSYS_IDLE;
     return 0;
 }
 
 void uiox_tpwd_subsys_set_cb(uiox_tpwd_subsys_t *sys,
                               uiox_tpwd_evt_cb_t  cb, void *ctx)
 {
     if (!sys) return;
     sys->evt_cb  = cb;
     sys->evt_ctx = ctx;
 }
 
 int uiox_tpwd_subsys_enrol_start(uiox_tpwd_subsys_t *sys,
                                   const char *id,
                                   uint32_t now_ms)
 {
     if (!sys || !id) return -EINVAL;
     strncpy(sys->active_id, id, UIOX_TPWD_SEC_ID_LEN - 1);
     uiox_tpwd_hw_set_backlight(sys->tif.hw, 200u);
     uiox_tpwd_gesture_start(&sys->gesture, now_ms);
     sys->state = UIOX_TPWD_SUBSYS_ENROLLING;
     return 0;
 }
 
 int uiox_tpwd_subsys_verify_start(uiox_tpwd_subsys_t *sys,
                                    const char *id,
                                    uint32_t now_ms)
 {
     if (!sys || !id) return -EINVAL;
 
     if (uiox_tpwd_sec_is_locked(&sys->sec, id, sys->now_s)) {
         sys->state = UIOX_TPWD_SUBSYS_LOCKED;
         fire(sys, UIOX_TPWD_EVT_LOCKED, 0u);
         return -EPERM;
     }
 
     strncpy(sys->active_id, id, UIOX_TPWD_SEC_ID_LEN - 1);
     uiox_tpwd_hw_set_backlight(sys->tif.hw, 180u);
     uiox_tpwd_gesture_start(&sys->gesture, now_ms);
     sys->state = UIOX_TPWD_SUBSYS_VERIFYING;
     return 0;
 }
 
 void uiox_tpwd_subsys_tick(uiox_tpwd_subsys_t *sys,
                             uint32_t now_ms, uint32_t now_s)
 {
     if (!sys) return;
     sys->now_s = now_s;
 
     if (sys->state == UIOX_TPWD_SUBSYS_IDLE ||
         sys->state == UIOX_TPWD_SUBSYS_LOCKED ||
         sys->state == UIOX_TPWD_SUBSYS_AUTHENTICATED) return;
 
     /* 1. Scan hardware */
     uiox_tpwd_if_scan(&sys->tif, &sys->gesture_rb, now_ms);
 
     /* 2. Process gesture */
     int done = uiox_tpwd_gesture_process(&sys->gesture, now_ms);
     if (done <= 0) return;
 
     const uiox_tpwd_credential_t *cred = &sys->gesture.current;
 
     if (cred->cancelled) {
         uiox_tpwd_hw_set_backlight(sys->tif.hw, 0u);
         sys->state = UIOX_TPWD_SUBSYS_IDLE;
         fire(sys, UIOX_TPWD_EVT_CANCELLED, 0u);
         audit_push(sys, UIOX_TPWD_EVT_CANCELLED, sys->active_id);
         return;
     }
 
     if (!cred->complete) {
         uiox_tpwd_hw_set_backlight(sys->tif.hw, 0u);
         sys->state = UIOX_TPWD_SUBSYS_IDLE;
         fire(sys, UIOX_TPWD_EVT_TIMEOUT, 0u);
         audit_push(sys, UIOX_TPWD_EVT_TIMEOUT, sys->active_id);
         return;
     }
 
     /* 3. Serialise credential */
     uint8_t raw[UIOX_TPWD_MAX_PIN_LEN + UIOX_TPWD_MAX_PATTERN_PTS];
     int raw_len = uiox_tpwd_gesture_serialise(cred, raw, sizeof(raw));
     if (raw_len <= 0) {
         sys->state = UIOX_TPWD_SUBSYS_IDLE;
         uiox_tpwd_hw_set_backlight(sys->tif.hw, 0u);
         return;
     }
 
     if (sys->state == UIOX_TPWD_SUBSYS_ENROLLING) {
         /* 4a. Enrolment */
         int rc = uiox_tpwd_sec_enrol(&sys->sec, sys->active_id,
                                       raw, (uint16_t)raw_len);
         uiox_tpwd_sec_zero(raw, sizeof(raw));
         uiox_tpwd_hw_set_backlight(sys->tif.hw, 0u);
         sys->state = UIOX_TPWD_SUBSYS_IDLE;
         if (rc == 0) {
             fire(sys, UIOX_TPWD_EVT_ENROLLED, 0u);
             audit_push(sys, UIOX_TPWD_EVT_ENROLLED, sys->active_id);
         }
     } else {
         /* 4b. Verification */
         int rc = uiox_tpwd_sec_verify(&sys->sec, sys->active_id,
                                        raw, (uint16_t)raw_len, now_s);
         uiox_tpwd_sec_zero(raw, sizeof(raw));
         uiox_tpwd_hw_set_backlight(sys->tif.hw, 0u);
 
         if (rc == 0) {
             uiox_tpwd_sec_gen_token(&sys->sec, 3600u, now_s);
             sys->state = UIOX_TPWD_SUBSYS_AUTHENTICATED;
             fire(sys, UIOX_TPWD_EVT_AUTH_OK, 0u);
             audit_push(sys, UIOX_TPWD_EVT_AUTH_OK, sys->active_id);
         } else if (rc == -EPERM) {
             sys->state = UIOX_TPWD_SUBSYS_LOCKED;
             fire(sys, UIOX_TPWD_EVT_LOCKED, 0u);
             audit_push(sys, UIOX_TPWD_EVT_LOCKED, sys->active_id);
         } else {
             uint8_t left = UIOX_TPWD_SEC_MAX_ATTEMPTS;
             for (uint8_t i = 0; i < sys->sec.record_count; i++) {
                 if (sys->sec.records[i].valid &&
                     strncmp(sys->sec.records[i].id,
                             sys->active_id,
                             UIOX_TPWD_SEC_ID_LEN) == 0) {
                     uint8_t att = sys->sec.records[i].attempts;
                     left = (att < UIOX_TPWD_SEC_MAX_ATTEMPTS) ?
                            (uint8_t)(UIOX_TPWD_SEC_MAX_ATTEMPTS - att) : 0u;
                     break;
                 }
             }
             sys->state = UIOX_TPWD_SUBSYS_IDLE;
             fire(sys, UIOX_TPWD_EVT_AUTH_FAIL, left);
             audit_push(sys, UIOX_TPWD_EVT_AUTH_FAIL, sys->active_id);
         }
     }
 }
 
 void uiox_tpwd_subsys_logout(uiox_tpwd_subsys_t *sys)
 {
     if (!sys) return;
     uiox_tpwd_sec_logout(&sys->sec);
     sys->state = UIOX_TPWD_SUBSYS_IDLE;
     fire(sys, UIOX_TPWD_EVT_LOGOUT, 0u);
     audit_push(sys, UIOX_TPWD_EVT_LOGOUT, sys->active_id);
 }
 
 uiox_tpwd_subsys_state_t uiox_tpwd_subsys_state(
     const uiox_tpwd_subsys_t *sys)
 {
     return sys ? sys->state : UIOX_TPWD_SUBSYS_IDLE;
 }
 
uint8_t uiox_tpwd_subsys_attempts_left(const uiox_tpwd_subsys_t *sys,
                                        const char *id)
{
    if (!sys || !id) return 0u;
    for (uint8_t i = 0; i < sys->sec.record_count; i++) {
        const uiox_tpwd_sec_record_t *r = &sys->sec.records[i];
        if (r->valid &&
            strncmp(r->id, id, UIOX_TPWD_SEC_ID_LEN) == 0) {
            if (r->attempts >= UIOX_TPWD_SEC_MAX_ATTEMPTS) return 0u;
            return (uint8_t)(UIOX_TPWD_SEC_MAX_ATTEMPTS - r->attempts);
        }
    }
    return UIOX_TPWD_SEC_MAX_ATTEMPTS;
}

void uiox_tpwd_subsys_print_audit(const uiox_tpwd_subsys_t *sys)
{
    if (!sys) return;
    static const char *evt_names[] = {
        "AUTH_OK","AUTH_FAIL","LOCKED","TIMEOUT",
        "CANCELLED","ENROLLED","DIGIT","BACKSPACE","LOGOUT"
    };
    printf("  %-12s  %-10s  %s\n", "Event", "Time(s)", "ID");
    printf("  %-12s  %-10s  %s\n", "------------","----------","---");
    for (uint8_t i = 0; i < UIOX_TPWD_AUDIT_MAX; i++) {
        uint8_t idx = (uint8_t)((sys->audit_head - 1u - i) %
                                 UIOX_TPWD_AUDIT_MAX);
        const uiox_tpwd_audit_entry_t *e = &sys->audit[idx];
        if (!e->valid) continue;
        uint8_t en = (uint8_t)e->evt;
        printf("  %-12s  %-10u  %s\n",
               (en < 9u) ? evt_names[en] : "?",
               e->ts_s, e->id);
    }
}
uint8_t uiox_tpwd_subsys_attempts_left(const uiox_tpwd_subsys_t *sys,
                                        const char *id)
{
    if (!sys || !id) return 0u;
    for (uint8_t i = 0; i < sys->sec.record_count; i++) {
        const uiox_tpwd_sec_record_t *r = &sys->sec.records[i];
        if (r->valid &&
            strncmp(r->id, id, UIOX_TPWD_SEC_ID_LEN) == 0) {
            if (r->attempts >= UIOX_TPWD_SEC_MAX_ATTEMPTS) return 0u;
            return (uint8_t)(UIOX_TPWD_SEC_MAX_ATTEMPTS - r->attempts);
        }
    }
    return UIOX_TPWD_SEC_MAX_ATTEMPTS;
}

void uiox_tpwd_subsys_print_audit(const uiox_tpwd_subsys_t *sys)
{
    if (!sys) return;
    static const char *evt_names[] = {
        "AUTH_OK","AUTH_FAIL","LOCKED","TIMEOUT",
        "CANCELLED","ENROLLED","DIGIT","BACKSPACE","LOGOUT"
    };
    printf("  %-12s  %-10s  %s\n", "Event", "Time(s)", "ID");
    printf("  %-12s  %-10s  %s\n", "------------","----------","---");
    for (uint8_t i = 0; i < UIOX_TPWD_AUDIT_MAX; i++) {
        uint8_t idx = (uint8_t)((sys->audit_head - 1u - i) %
                                 UIOX_TPWD_AUDIT_MAX);
        const uiox_tpwd_audit_entry_t *e = &sys->audit[idx];
        if (!e->valid) continue;
        uint8_t en = (uint8_t)e->evt;
        printf("  %-12s  %-10u  %s\n",
               (en < 9u) ? evt_names[en] : "?",
               e->ts_s, e->id);
    }
}
uint8_t uiox_tpwd_subsys_attempts_left(const uiox_tpwd_subsys_t *sys,
                                        const char *id)
{
    if (!sys || !id) return 0u;
    for (uint8_t i = 0; i < sys->sec.record_count; i++) {
        const uiox_tpwd_sec_record_t *r = &sys->sec.records[i];
        if (r->valid &&
            strncmp(r->id, id, UIOX_TPWD_SEC_ID_LEN) == 0) {
            if (r->attempts >= UIOX_TPWD_SEC_MAX_ATTEMPTS) return 0u;
            return (uint8_t)(UIOX_TPWD_SEC_MAX_ATTEMPTS - r->attempts);
        }
    }
    return UIOX_TPWD_SEC_MAX_ATTEMPTS;
}

void uiox_tpwd_subsys_print_audit(const uiox_tpwd_subsys_t *sys)
{
    if (!sys) return;
    static const char *evt_names[] = {
        "AUTH_OK","AUTH_FAIL","LOCKED","TIMEOUT",
        "CANCELLED","ENROLLED","DIGIT","BACKSPACE","LOGOUT"
    };
    printf("  %-12s  %-10s  %s\n", "Event", "Time(s)", "ID");
    printf("  %-12s  %-10s  %s\n", "------------","----------","---");
    for (uint8_t i = 0; i < UIOX_TPWD_AUDIT_MAX; i++) {
        uint8_t idx = (uint8_t)((sys->audit_head - 1u - i) %
                                 UIOX_TPWD_AUDIT_MAX);
        const uiox_tpwd_audit_entry_t *e = &sys->audit[idx];
        if (!e->valid) continue;
        uint8_t en = (uint8_t)e->evt;
        printf("  %-12s  %-10u  %s\n",
               (en < 9u) ? evt_names[en] : "?",
               e->ts_s, e->id);
    }
}
uint8_t uiox_tpwd_subsys_attempts_left(const uiox_tpwd_subsys_t *sys,
                                        const char *id)
{
    if (!sys || !id) return 0u;
    for (uint8_t i = 0; i < sys->sec.record_count; i++) {
        const uiox_tpwd_sec_record_t *r = &sys->sec.records[i];
        if (r->valid &&
            strncmp(r->id, id, UIOX_TPWD_SEC_ID_LEN) == 0) {
            if (r->attempts >= UIOX_TPWD_SEC_MAX_ATTEMPTS) return 0u;
            return (uint8_t)(UIOX_TPWD_SEC_MAX_ATTEMPTS - r->attempts);
        }
    }
    return UIOX_TPWD_SEC_MAX_ATTEMPTS;
}

void uiox_tpwd_subsys_print_audit(const uiox_tpwd_subsys_t *sys)
{
    if (!sys) return;
    static const char *evt_names[] = {
        "AUTH_OK","AUTH_FAIL","LOCKED","TIMEOUT",
        "CANCELLED","ENROLLED","DIGIT","BACKSPACE","LOGOUT"
    };
    printf("  %-12s  %-10s  %s\n", "Event", "Time(s)", "ID");
    printf("  %-12s  %-10s  %s\n", "------------","----------","---");
    for (uint8_t i = 0; i < UIOX_TPWD_AUDIT_MAX; i++) {
        uint8_t idx = (uint8_t)((sys->audit_head - 1u - i) %
                                 UIOX_TPWD_AUDIT_MAX);
        const uiox_tpwd_audit_entry_t *e = &sys->audit[idx];
        if (!e->valid) continue;
        uint8_t en = (uint8_t)e->evt;
        printf("  %-12s  %-10u  %s\n",
               (en < 9u) ? evt_names[en] : "?",
               e->ts_s, e->id);
    }
}
uint8_t uiox_tpwd_subsys_attempts_left(const uiox_tpwd_subsys_t *sys,
    const char *id)
{
if (!sys || !id) return 0u;
for (uint8_t i = 0; i < sys->sec.record_count; i++) {
const uiox_tpwd_sec_record_t *r = &sys->sec.records[i];
if (r->valid &&
strncmp(r->id, id, UIOX_TPWD_SEC_ID_LEN) == 0) {
if (r->attempts >= UIOX_TPWD_SEC_MAX_ATTEMPTS) return 0u;
return (uint8_t)(UIOX_TPWD_SEC_MAX_ATTEMPTS - r->attempts);
}
}
return UIOX_TPWD_SEC_MAX_ATTEMPTS;
}
void uiox_tpwd_subsys_print_audit(const uiox_tpwd_subsys_t *sys)
{
if (!sys) return;
static const char *evt_names[] = {
"AUTH_OK","AUTH_FAIL","LOCKED","TIMEOUT",
"CANCELLED","ENROLLED","DIGIT","BACKSPACE","LOGOUT"
};
printf("  %-12s  %-10s  %s\n", "Event", "Time(s)", "ID");
printf("  %-12s  %-10s  %s\n", "------------","----------","---");
for (uint8_t i = 0; i < UIOX_TPWD_AUDIT_MAX; i++) {
uint8_t idx = (uint8_t)((sys->audit_head - 1u - i) %
UIOX_TPWD_AUDIT_MAX);
const uiox_tpwd_audit_entry_t *e = &sys->audit[idx];
if (!e->valid) continue;
uint8_t en = (uint8_t)e->evt;
printf("  %-12s  %-10u  %s\n",
(en < 9u) ? evt_names[en] : "?",
e->ts_s, e->id);
}
}
 