/**
 * @file  uiox_pkg_subsys.h
 * @brief UIOX Package Manager subsystem — install/remove/upgrade/query.
 * @version 1.0.0
 * @date    2026-06-29
 */

 #ifndef UIOX_PKG_SUBSYS_H
 #define UIOX_PKG_SUBSYS_H
 
 #include "uiox_pkg_resolve.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * Subsystem events
  * ====================================================================== */
 
 typedef enum {
     UIOX_PKG_EV_INSTALL_START  = 0,
     UIOX_PKG_EV_INSTALL_DONE,
     UIOX_PKG_EV_REMOVE_START,
     UIOX_PKG_EV_REMOVE_DONE,
     UIOX_PKG_EV_UPGRADE_START,
     UIOX_PKG_EV_UPGRADE_DONE,
     UIOX_PKG_EV_DEP_INSTALL,
     UIOX_PKG_EV_CONFLICT,
     UIOX_PKG_EV_ERROR,
     UIOX_PKG_EV_INSTALL_FAIL,
     UIOX_PKG_EV_UPGRADE_FAIL,
 } uiox_pkg_ev_t;
 
 typedef void (*uiox_pkg_evt_cb_t)(uiox_pkg_ev_t ev,
                                    const char *pkg_name,
                                    uiox_pkg_err_t status,
                                    void *ctx);
 
 /* =========================================================================
  * Subsystem state
  * ====================================================================== */
 
 typedef enum {
     UIOX_PKG_SYS_OFF    = 0,
     UIOX_PKG_SYS_INIT,
     UIOX_PKG_SYS_READY,
     UIOX_PKG_SYS_BUSY,
     UIOX_PKG_SYS_ERROR,
 } uiox_pkg_sys_state_t;
 
 /* =========================================================================
  * Subsystem context
  * ====================================================================== */
 
 typedef struct {
     uiox_pkg_store_t     store;
     uiox_pkg_resolver_t  resolver;
     uiox_pkg_sys_state_t state;
     uiox_pkg_evt_cb_t    evt_cb;
     void                *evt_ctx;
     /* Statistics */
     uint32_t             installed_count;
     uint32_t             install_ops;
     uint32_t             remove_ops;
     uint32_t             upgrade_ops;
     uint32_t             error_count;
 } uiox_pkg_subsys_t;
 
 /* =========================================================================
  * Subsystem API
  * ====================================================================== */
 
 uiox_pkg_err_t uiox_pkg_subsys_init      (uiox_pkg_subsys_t *sys,
                                            uiox_pkg_repo_type_t repo,
                                            const char *repo_path);
 uiox_pkg_err_t uiox_pkg_subsys_start     (uiox_pkg_subsys_t *sys);
 void           uiox_pkg_subsys_stop      (uiox_pkg_subsys_t *sys);
 void           uiox_pkg_subsys_set_cb    (uiox_pkg_subsys_t *sys,
                                            uiox_pkg_evt_cb_t cb,
                                            void *ctx);
 
 /* Package operations */
 uiox_pkg_err_t uiox_pkg_subsys_install   (uiox_pkg_subsys_t *sys,
                                            const char *name,
                                            uint32_t version);
 uiox_pkg_err_t uiox_pkg_subsys_remove    (uiox_pkg_subsys_t *sys,
                                            const char *name);
 uiox_pkg_err_t uiox_pkg_subsys_upgrade   (uiox_pkg_subsys_t *sys,
                                            const char *name,
                                            uint32_t new_version);
 uiox_pkg_err_t uiox_pkg_subsys_upgrade_all(uiox_pkg_subsys_t *sys);
 
 /* Query */
 uiox_pkg_err_t uiox_pkg_subsys_query     (uiox_pkg_subsys_t *sys,
                                            const char *name,
                                            uiox_pkg_rec_t *out);
 bool           uiox_pkg_subsys_is_installed(uiox_pkg_subsys_t *sys,
                                            const char *name);
 void           uiox_pkg_subsys_list      (uiox_pkg_subsys_t *sys,
                                            uiox_pkg_state_t filter);
 
 /* Info */
 void           uiox_pkg_subsys_print_info(const uiox_pkg_subsys_t *sys);
 void           uiox_pkg_subsys_print_stats(uiox_pkg_subsys_t *sys);
 
 /* Name helpers */
 const char    *uiox_pkg_state_name       (uiox_pkg_state_t s);
 const char    *uiox_pkg_ev_name          (uiox_pkg_ev_t ev);
 const char    *uiox_pkg_err_str          (uiox_pkg_err_t e);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_PKG_SUBSYS_H */
 