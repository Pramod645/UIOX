/**
 * @file  uiox_chg_buf.c
 * @brief UIOX Charger event / fault buffer pool.
 * @date  2026-06-11
 */

 #include "uiox_chg_buf.h"

 /* Freestanding assert — no libc assert.h available under -nostdinc */
 #ifndef UIOX_ASSERT
 #  define UIOX_ASSERT(cond)  do { if (!(cond)) __builtin_trap(); } while (0)
 #endif
 
 static uiox_chg_evt_t   s_evt_pool  [UIOX_CHG_EVT_POOL_SIZE];
 static uiox_chg_fault_t s_fault_pool[UIOX_CHG_FAULT_POOL_SIZE];
 static uint8_t s_evt_cnt   = 0u;
 static uint8_t s_fault_cnt = 0u;
 
 void uiox_chg_buf_init(void)
 {
     memset(s_evt_pool,   0, sizeof(s_evt_pool));
     memset(s_fault_pool, 0, sizeof(s_fault_pool));
     s_evt_cnt   = UIOX_CHG_EVT_POOL_SIZE;
     s_fault_cnt = UIOX_CHG_FAULT_POOL_SIZE;
 }
 
 uiox_chg_evt_t *uiox_chg_evt_alloc(void)
 {
     for (uint8_t i = 0u; i < UIOX_CHG_EVT_POOL_SIZE; i++) {
         if (!s_evt_pool[i].in_use) {
             memset(&s_evt_pool[i], 0, sizeof(s_evt_pool[i]));
             s_evt_pool[i].in_use = 1u; s_evt_cnt--;
             return &s_evt_pool[i];
         }
     }
     return NULL;
 }
 
 void uiox_chg_evt_free(uiox_chg_evt_t *e)
 {
     if (!e) return;
     UIOX_ASSERT(e->in_use > 0u);
     e->in_use = 0u; s_evt_cnt++;
 }
 
 uint8_t uiox_chg_evt_free_cnt(void) { return s_evt_cnt; }
 
 uiox_chg_fault_t *uiox_chg_fault_alloc(void)
 {
     for (uint8_t i = 0u; i < UIOX_CHG_FAULT_POOL_SIZE; i++) {
         if (!s_fault_pool[i].in_use) {
             memset(&s_fault_pool[i], 0, sizeof(s_fault_pool[i]));
             s_fault_pool[i].in_use = 1u; s_fault_cnt--;
             return &s_fault_pool[i];
         }
     }
     return NULL;
 }
 
 void uiox_chg_fault_free(uiox_chg_fault_t *f)
 {
     if (!f) return;
     UIOX_ASSERT(f->in_use > 0u);
     f->in_use = 0u; s_fault_cnt++;
 }
 
 uint8_t uiox_chg_fault_free_cnt(void) { return s_fault_cnt; }
 