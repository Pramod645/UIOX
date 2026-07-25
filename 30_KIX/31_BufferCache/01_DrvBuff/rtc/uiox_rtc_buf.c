/**
 * @file  uiox_rtc_buf.c
 * @brief UIOX RTC alarm/event buffer pool implementation.
 * @date  2026-06-10
 */

 #include "uiox_rtc_buf.h"

 /* Freestanding assert — no libc assert.h available under -nostdinc */
 #ifndef UIOX_ASSERT
 #  define UIOX_ASSERT(cond)  do { if (!(cond)) __builtin_trap(); } while (0)
 #endif
 
 static uiox_rtc_evt_t s_evt_pool[UIOX_RTC_EVT_POOL_SIZE];
 static uiox_rtc_alm_t s_alm_pool[UIOX_RTC_ALM_POOL_SIZE];
 static uint8_t s_evt_cnt = 0u, s_alm_cnt = 0u;
 
 void uiox_rtc_buf_init(void)
 {
     for (int i = 0; i < (int)UIOX_RTC_EVT_POOL_SIZE; i++) {
         memset(&s_evt_pool[i], 0, sizeof(s_evt_pool[i]));
         s_evt_cnt++;
     }
     for (int i = 0; i < (int)UIOX_RTC_ALM_POOL_SIZE; i++) {
         memset(&s_alm_pool[i], 0, sizeof(s_alm_pool[i]));
         s_alm_cnt++;
     }
 }
 
 uiox_rtc_evt_t *uiox_rtc_evt_alloc(void)
 {
     for (uint8_t i = 0u; i < UIOX_RTC_EVT_POOL_SIZE; i++) {
         if (!s_evt_pool[i].in_use) {
             s_evt_pool[i].in_use = 1u; s_evt_cnt--;
             return &s_evt_pool[i];
         }
     }
     return NULL;
 }
 
 void uiox_rtc_evt_free(uiox_rtc_evt_t *e)
 {
     if (!e) return;
     UIOX_ASSERT(e->in_use > 0u);
     e->in_use = 0u; e->type = UIOX_RTC_EVT_NONE; s_evt_cnt++;
 }
 
 uint8_t uiox_rtc_evt_free_cnt(void) { return s_evt_cnt; }
 
 uiox_rtc_alm_t *uiox_rtc_alm_alloc(void)
 {
     for (uint8_t i = 0u; i < UIOX_RTC_ALM_POOL_SIZE; i++) {
         if (!s_alm_pool[i].in_use) {
             s_alm_pool[i].in_use = 1u; s_alm_cnt--;
             return &s_alm_pool[i];
         }
     }
     return NULL;
 }
 
 void uiox_rtc_alm_free(uiox_rtc_alm_t *a)
 {
     if (!a) return;
     UIOX_ASSERT(a->in_use > 0u);
     a->in_use = 0u; s_alm_cnt++;
 }
 
 uint8_t uiox_rtc_alm_free_cnt(void) { return s_alm_cnt; }
 