/**
 * @file  uiox_pkg_buf.c
 * @brief UIOX Package Manager — package record pool.
 *
 * PATH  32_FS/04_pkg/src/uiox_pkg_buf.c
 *
 * Freestanding: no libc.  memset comes from uiox_klibc.h; the assert is the
 * local __builtin_trap() form used by the other pool files in this tree
 * (uiox_emmc_buf.c), because <assert.h> does not exist under -nostdinc.
 *
 * @version 1.0.0
 * @date    2026-10-26
 */

 #include "uiox_pkg_buf.h"


 #ifndef UIOX_ASSERT
 #  define UIOX_ASSERT(cond)  do { if (!(cond)) __builtin_trap(); } while (0)
 #endif
 
 static uiox_pkg_rec_t s_rec_pool[UIOX_PKG_REC_POOL_SIZE];
 static uint8_t        s_rec_cnt = 0u;
 
 void uiox_pkg_buf_init(void)
 {
     memset(s_rec_pool, 0, sizeof(s_rec_pool));
     s_rec_cnt = UIOX_PKG_REC_POOL_SIZE;
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
     UIOX_ASSERT(r->in_use > 0u);
     r->in_use = 0u;
     r->next   = NULL;
     s_rec_cnt++;
 }
 
 uint8_t uiox_pkg_rec_free_cnt(void) { return s_rec_cnt; }
 