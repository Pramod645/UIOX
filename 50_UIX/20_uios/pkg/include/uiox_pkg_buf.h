/**
 * @file  uiox_pkg_buf.h
 * @brief UIOX Package Manager — package record and event queue pool.
 * @version 1.0.0
 * @date    2026-06-29
 */

 #ifndef UIOX_PKG_BUF_H
 #define UIOX_PKG_BUF_H
 
 #include "uiox_pkg_types.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * Pool sizes
  * ====================================================================== */
 
 #define UIOX_PKG_REC_POOL_SIZE    64u  /**< Max packages in registry      */
 #define UIOX_PKG_EVT_POOL_SIZE    32u  /**< Event queue depth             */
 
 /* =========================================================================
  * Event record
  * ====================================================================== */
 
 typedef enum {
     UIOX_PKG_EVT_NONE        = 0,
     UIOX_PKG_EVT_INSTALL_OK,
     UIOX_PKG_EVT_INSTALL_FAIL,
     UIOX_PKG_EVT_REMOVE_OK,
     UIOX_PKG_EVT_REMOVE_FAIL,
     UIOX_PKG_EVT_UPGRADE_OK,
     UIOX_PKG_EVT_UPGRADE_FAIL,
     UIOX_PKG_EVT_DEP_MISSING,
     UIOX_PKG_EVT_CONFLICT,
     UIOX_PKG_EVT_DOWNLOAD_START,
     UIOX_PKG_EVT_DOWNLOAD_DONE,
     UIOX_PKG_EVT_VERIFY_FAIL,
 } uiox_pkg_evt_type_t;
 
 typedef struct {
     uiox_pkg_evt_type_t type;
     char                pkg_name[UIOX_PKG_NAME_MAX];
     uint32_t            pkg_version;
     uiox_pkg_err_t      status;
     uint32_t            timestamp;
     uint8_t             in_use;
 } uiox_pkg_evt_t;
 
 /* =========================================================================
  * Pool API
  * ====================================================================== */
 
 void            uiox_pkg_buf_init      (void);
 
 uiox_pkg_rec_t *uiox_pkg_rec_alloc     (void);
 void            uiox_pkg_rec_free      (uiox_pkg_rec_t *r);
 uint8_t         uiox_pkg_rec_free_cnt  (void);
 
 uiox_pkg_evt_t *uiox_pkg_evt_alloc     (void);
 void            uiox_pkg_evt_free      (uiox_pkg_evt_t *e);
 uint8_t         uiox_pkg_evt_free_cnt  (void);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_PKG_BUF_H */
 