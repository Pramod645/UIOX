/**
 * @file  uiox_pkg_buf.c
 * @brief UIOX Package Manager — pool implementation.
 * @date  2026-06-29
 */

 #include "../include/uiox_pkg_device.h"
 #include <string.h>
 #include <assert.h>
 
 static uiox_pkg_rec_t s_rec_pool[UIOX_PKG_REC_POOL_SIZE];
 static uiox_pkg_evt_t s_evt_pool[UIOX_PKG_EVT_POOL_SIZE];
 static uint8_t        s_rec_cnt = 0u;
 static uint8_t        s_evt_cnt = 0u;
 
 void uiox_pkg_buf_init(void)
 {
     memset(s_rec_pool, 0, sizeof(s_rec_pool));
     memset(s_evt_pool, 0, sizeof(s_evt_pool));
     s_rec_cnt = UIOX_PKG_REC_POOL_SIZE;
     s_evt_cnt = UIOX_PKG_EVT_POOL_SIZE;
 }
 
 uiox_pkg_rec_t *uiox_pkg_rec_alloc(void)
 {
     for (uint8_t i = 0u; i < UIOX_PKG_REC_POOL_SIZE; i++) {
         if (!s_rec_pool[i].in_use) {
             memset(&s_rec_pool[i], 0, sizeof(s_rec_pool[i]));
             s_rec_pool[i].in_use = 1u;
             s_rec_cnt--;
             return &s_rec_pool[i];
         }
     }
     return NULL;
 }
 
 void uiox_pkg_rec_free(uiox_pkg_rec_t *r)
 {
     if (!r) return;
     assert(r->in_use > 0u);
     r->in_use = 0u;
     s_rec_cnt++;
 }
 
 uint8_t uiox_pkg_rec_free_cnt(void) { return s_rec_cnt; }
 
 uiox_pkg_evt_t *uiox_pkg_evt_alloc(void)
 {
     for (uint8_t i = 0u; i < UIOX_PKG_EVT_POOL_SIZE; i++) {
         if (!s_evt_pool[i].in_use) {
             memset(&s_evt_pool[i], 0, sizeof(s_evt_pool[i]));
             s_evt_pool[i].in_use = 1u;
             s_evt_cnt--;
             return &s_evt_pool[i];
         }
     }
     return NULL;
 }
 
 void uiox_pkg_evt_free(uiox_pkg_evt_t *e)
 {
     if (!e) return;
     assert(e->in_use > 0u);
     e->in_use = 0u;
     s_evt_cnt++;
 }
 
 uint8_t uiox_pkg_evt_free_cnt(void) { return s_evt_cnt; }
 