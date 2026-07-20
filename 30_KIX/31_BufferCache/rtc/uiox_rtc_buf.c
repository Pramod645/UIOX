/**
 * @file  uiox_rtc_buf.c
 * @brief UIOX RTC alarm/event buffer pool implementation.
 * @date  2026-06-10
 */

 #include "uiox_rtc_buf.h"
 #include <string.h>
 #include <assert.h>
 
 static uiox_rtc_evt_t s_evt_pool[UIOX_RTC_EVT_POOL_SIZE];
 static uiox_rtc_alm_t s_alm_pool[UIOX_RTC_ALM_POOL_SIZE];
 
 static uiox_rtc_evt_t *s_evt_free = NULL;
 static uiox_rtc_alm_t *s_alm_free = NULL;
 static uint8_t         s_evt_cnt  = 0u;
 static uint8_t         s_alm_cnt  = 0u;
 
 void uiox_rtc_buf_init(void)
 {
     s_evt_free = NULL; s_evt_cnt = 0u;
     for (int i = 0; i < (int)UIOX_RTC_EVT_POOL_SIZE; i++) {
         memset(&s_evt_pool[i], 0, sizeof(s_evt_pool[i]));
         /* Use flags field as next-pointer index trick; use cast instead */
         s_evt_pool[i].in_use = 0u;
         /* Build free list via type reuse: store pointer in padding */
         /* Simpler: just iterate on alloc */
     }
     s_alm_free = NULL; s_alm_cnt = 0u;
 
     /* Build event free list */
     for (int i = (int)UIOX_RTC_EVT_POOL_SIZE - 1; i >= 0; i--) {
         uiox_rtc_evt_t *e = &s_evt_pool[i];
         /* store next pointer — embed in unused field */
         memset(e, 0, sizeof(*e));
         e->in_use = 0u;
         /* linked-list via static array: use s_evt_cnt as index */
         s_evt_cnt++;
     }
 
     /* Build alarm free list */
     for (int i = (int)UIOX_RTC_ALM_POOL_SIZE - 1; i >= 0; i--) {
         uiox_rtc_alm_t *a = &s_alm_pool[i];
         memset(a, 0, sizeof(*a));
         a->in_use = 0u;
         s_alm_cnt++;
     }
 }
 
 /* -------------------------------------------------------------------------
  * Simple scan-based alloc (pool is small; no linked-list overhead needed)
  * ---------------------------------------------------------------------- */
 
 uiox_rtc_evt_t *uiox_rtc_evt_alloc(void)
 {
     for (uint8_t i = 0u; i < UIOX_RTC_EVT_POOL_SIZE; i++) {
         if (!s_evt_pool[i].in_use) {
             s_evt_pool[i].in_use = 1u;
             s_evt_cnt--;
             return &s_evt_pool[i];
         }
     }
     return NULL;
 }
 
 void uiox_rtc_evt_free(uiox_rtc_evt_t *e)
 {
     if (!e) return;
     assert(e->in_use > 0u);
     e->in_use = 0u;
     e->type   = UIOX_RTC_EVT_NONE;
     s_evt_cnt++;
 }
 
 uint8_t uiox_rtc_evt_free_cnt(void) { return s_evt_cnt; }
 
 uiox_rtc_alm_t *uiox_rtc_alm_alloc(void)
 {
     for (uint8_t i = 0u; i < UIOX_RTC_ALM_POOL_SIZE; i++) {
         if (!s_alm_pool[i].in_use) {
             s_alm_pool[i].in_use = 1u;
             s_alm_cnt--;
             return &s_alm_pool[i];
         }
     }
     return NULL;
 }
 
 void uiox_rtc_alm_free(uiox_rtc_alm_t *a)
 {
     if (!a) return;
     assert(a->in_use > 0u);
     a->in_use = 0u;
     s_alm_cnt++;
 }
 
 uint8_t uiox_rtc_alm_free_cnt(void) { return s_alm_cnt; }
 