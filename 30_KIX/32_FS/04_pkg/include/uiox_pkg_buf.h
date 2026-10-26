/**
 * @file  uiox_pkg_buf.h
 * @brief UIOX Package Manager — package record pool.
 *
 * PATH  32_FS/04_pkg/include/uiox_pkg_buf.h
 *
 * This pool IS the package registry.  uiox_pkg_store_load_pkg() returns a
 * pointer into it, and the store index holds that pointer rather than a copy,
 * so a caller in possession of a uiox_pkg_rec_t * owns a pool slot until it
 * calls uiox_pkg_rec_free().
 *
 * There is no event pool.  Events are delivered by callback from
 * uiox_pkg_subsys.c (fire() → evt_cb) on the same call that produced them;
 * nothing queues them and nothing polls, so a pool would have no reader.
 *
 * @version 1.0.0
 * @date    2026-10-26
 */

 #ifndef UIOX_PKG_BUF_H
 #define UIOX_PKG_BUF_H
 
 #include "uiox_pkg_types.h"
 #include "uiox_base_types.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * Pool size — max packages in the registry at once
  * ====================================================================== */
 
 #define UIOX_PKG_REC_POOL_SIZE    64u
 
 /* =========================================================================
  * Pool API
  * ====================================================================== */
 
 void            uiox_pkg_buf_init      (void);
 
 uiox_pkg_rec_t *uiox_pkg_rec_alloc     (void);
 void            uiox_pkg_rec_free      (uiox_pkg_rec_t *r);
 uint8_t         uiox_pkg_rec_free_cnt  (void);
 
 /* compare, so pkg keeps one.  Bounded by n, NUL-terminating-safe: stops at
 * the first difference, at either terminator, or after n bytes. */
static inline int uiox_pkg_strncmp(const char *a, const char *b, uiox_size_t n)
{
    while (n-- > 0u) {
        if (*a != *b) return (int)(unsigned char)*a - (int)(unsigned char)*b;
        if (*a == '\0') return 0;
        a++; b++;
    }
    return 0;
}

 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_PKG_BUF_H */
 