/**
 * @file    uiox_bms_buf.c
 * @brief   UIOX BMS buffer pool implementation.
 * @date    2026-06-04
 */

 #include "uiox_bms_buf.h"
 #include <string.h>
 #include <assert.h>
 
 static uiox_bms_event_t s_evlog[UIOX_BMS_EVENT_LOG_SIZE];
 static uint8_t           s_ev_head = 0;
 static uint8_t           s_ev_tail = 0;
 static uint8_t           s_ev_cnt  = 0;
 
 static uiox_bms_telem_t  s_telem[UIOX_BMS_TELEM_POOL_SIZE];
 static uiox_bms_telem_t *s_telem_free = NULL;
 
 void uiox_bms_buf_init(void)
 {
     memset(s_evlog, 0, sizeof(s_evlog));
     s_ev_head = s_ev_tail = s_ev_cnt = 0;
     s_telem_free = NULL;
     for (int i = 0; i < UIOX_BMS_TELEM_POOL_SIZE; i++) {
         memset(&s_telem[i], 0, sizeof(s_telem[i]));
         s_telem[i].next = s_telem_free;
         s_telem_free    = &s_telem[i];
     }
 }
 
 void uiox_bms_event_push(const uiox_bms_event_t *ev)
 {
     if (!ev) return;
     s_evlog[s_ev_head % UIOX_BMS_EVENT_LOG_SIZE] = *ev;
     s_evlog[s_ev_head % UIOX_BMS_EVENT_LOG_SIZE].valid = true;
     s_ev_head++;
     if (s_ev_cnt < UIOX_BMS_EVENT_LOG_SIZE) s_ev_cnt++;
     else s_ev_tail++;
 }
 
 bool uiox_bms_event_pop(uiox_bms_event_t *ev)
 {
     if (!ev || s_ev_cnt == 0) return false;
     *ev = s_evlog[s_ev_tail % UIOX_BMS_EVENT_LOG_SIZE];
     s_ev_tail++;
     s_ev_cnt--;
     return true;
 }
 
 bool    uiox_bms_event_empty(void) { return s_ev_cnt == 0; }
 uint8_t uiox_bms_event_count(void) { return s_ev_cnt; }
 
 uiox_bms_telem_t *uiox_bms_telem_alloc(void)
 {
     if (!s_telem_free) return NULL;
     uiox_bms_telem_t *t = s_telem_free;
     s_telem_free = t->next;
     t->next      = NULL;
     t->in_use    = 1;
     memset(&t->ts_ms, 0,
            sizeof(*t) - offsetof(uiox_bms_telem_t, ts_ms));
     return t;
 }
 
 void uiox_bms_telem_free(uiox_bms_telem_t *t)
 {
     if (!t) return;
     assert(t->in_use > 0);
     if (--t->in_use == 0) {
         t->next      = s_telem_free;
         s_telem_free = t;
     }
 }
 