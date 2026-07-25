/**
 * @file  uiox_als_buf.c
 * @brief UIOX ALS sample and event buffer pool.
 * @date  2026-06-11
 */

 #include "uiox_als_buf.h"

 /* Freestanding assert — no libc assert.h available under -nostdinc */
 #ifndef UIOX_ASSERT
 #  define UIOX_ASSERT(cond)  do { if (!(cond)) __builtin_trap(); } while (0)
 #endif
 
 static uiox_als_sample_t s_sample_pool[UIOX_ALS_SAMPLE_POOL_SIZE];
 static uiox_als_evt_t    s_evt_pool   [UIOX_ALS_EVT_POOL_SIZE];
 
 static uint8_t s_sample_cnt = 0u;
 static uint8_t s_evt_cnt    = 0u;
 
 void uiox_als_buf_init(void)
 {
     memset(s_sample_pool, 0, sizeof(s_sample_pool));
     memset(s_evt_pool,    0, sizeof(s_evt_pool));
     s_sample_cnt = UIOX_ALS_SAMPLE_POOL_SIZE;
     s_evt_cnt    = UIOX_ALS_EVT_POOL_SIZE;
 }
 
 uiox_als_sample_t *uiox_als_sample_alloc(void)
 {
     for (uint8_t i = 0u; i < UIOX_ALS_SAMPLE_POOL_SIZE; i++) {
         if (!s_sample_pool[i].in_use) {
             memset(&s_sample_pool[i], 0, sizeof(s_sample_pool[i]));
             s_sample_pool[i].in_use = 1u;
             s_sample_cnt--;
             return &s_sample_pool[i];
         }
     }
     return NULL;
 }
 
 void uiox_als_sample_free(uiox_als_sample_t *s)
 {
     if (!s) return;
     /* FIX: was 'f->in_use' — parameter is 's' in this function */
     UIOX_ASSERT(s->in_use > 0);
     s->in_use = 0u;
     s_sample_cnt++;
 }
 
 uint8_t uiox_als_sample_free_cnt(void) { return s_sample_cnt; }
 
 uiox_als_evt_t *uiox_als_evt_alloc(void)
 {
     for (uint8_t i = 0u; i < UIOX_ALS_EVT_POOL_SIZE; i++) {
         if (!s_evt_pool[i].in_use) {
             memset(&s_evt_pool[i], 0, sizeof(s_evt_pool[i]));
             s_evt_pool[i].in_use = 1u;
             s_evt_cnt--;
             return &s_evt_pool[i];
         }
     }
     return NULL;
 }
 
 void uiox_als_evt_free(uiox_als_evt_t *e)
 {
     if (!e) return;
     /* FIX: was 'f->in_use' — parameter is 'e' in this function */
     UIOX_ASSERT(e->in_use > 0);
     e->in_use = 0u;
     s_evt_cnt++;
 }
 
 uint8_t uiox_als_evt_free_cnt(void) { return s_evt_cnt; }
 