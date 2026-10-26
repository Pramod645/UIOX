/**
 * @file  uiox_pkg_exec.h
 * @brief UIOX Package Manager — plan executor: install/remove/upgrade/query.
 *
 * PATH  32_FS/04_pkg/include/uiox_pkg_exec.h
 *
 * This is not a subsystem; it is the half that RUNS a resolved plan.  The
 * resolver decides (Kahn's topological sort, conflict detection); this file
 * walks plan->entries[] and drives the store, raising a progress event on
 * the call that produced it.
 *
 * Events are delivered by callback (fire() → evt_cb); nothing queues them
 * and nothing polls.  The caller of an install/remove/upgrade already learns
 * the outcome from the return value, so the callbacks are progress
 * notifications during that call, not a delivery channel for later.
 *
 * @version 1.0.0
 * @date    2026-10-26
 */

 #ifndef UIOX_PKG_EXEC_H
 #define UIOX_PKG_EXEC_H
 
 #include "uiox_pkg_resolve.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * Executor events
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
  * Executor state
  * ====================================================================== */
 
 typedef enum {
     UIOX_PKG_EXEC_OFF    = 0,
     UIOX_PKG_EXEC_INIT,
     UIOX_PKG_EXEC_READY,
     UIOX_PKG_EXEC_BUSY,      /**< a plan is running                        */
     UIOX_PKG_EXEC_ERROR,
 } uiox_pkg_exec_state_t;
 
 /* =========================================================================
  * Executor context
  * ====================================================================== */
 
 typedef struct {
     uiox_pkg_store_t        store;
     uiox_pkg_resolver_t     resolver;
     uiox_pkg_exec_state_t   state;
     uiox_pkg_evt_cb_t       evt_cb;
     void                   *evt_ctx;
     /* Statistics */
     uint32_t                installed_count;
     uint32_t                install_ops;
     uint32_t                remove_ops;
     uint32_t                upgrade_ops;
     uint32_t                error_count;
 } uiox_pkg_exec_t;
 
 /* =========================================================================
  * Executor API
  * ====================================================================== */
 
 uiox_pkg_err_t uiox_pkg_exec_init      (uiox_pkg_exec_t *ex,
                                         uiox_pkg_repo_type_t repo,
                                         const char *repo_path);
 uiox_pkg_err_t uiox_pkg_exec_start     (uiox_pkg_exec_t *ex);
 void           uiox_pkg_exec_stop      (uiox_pkg_exec_t *ex);
 void           uiox_pkg_exec_set_cb    (uiox_pkg_exec_t *ex,
                                         uiox_pkg_evt_cb_t cb,
                                         void *ctx);
 
 /* Plan execution — each resolves a plan, then runs it entry by entry */
 uiox_pkg_err_t uiox_pkg_exec_install   (uiox_pkg_exec_t *ex,
                                         const char *name,
                                         uint32_t version);
 uiox_pkg_err_t uiox_pkg_exec_remove    (uiox_pkg_exec_t *ex,
                                         const char *name);
 uiox_pkg_err_t uiox_pkg_exec_upgrade   (uiox_pkg_exec_t *ex,
                                         const char *name,
                                         uint32_t new_version);
 uiox_pkg_err_t uiox_pkg_exec_upgrade_all(uiox_pkg_exec_t *ex);
 
 /* Query.
  *
  * exec_query returns a pointer into the record pool; the caller owns that
  * slot and must release it with uiox_pkg_rec_free() when done. */
 uiox_pkg_err_t  uiox_pkg_exec_query     (uiox_pkg_exec_t *ex,
                                          const char *name,
                                          uiox_pkg_rec_t **out);
 bool            uiox_pkg_exec_is_installed(uiox_pkg_exec_t *ex,
                                          const char *name);
 void            uiox_pkg_exec_list      (uiox_pkg_exec_t *ex,
                                          uiox_pkg_state_t filter);
 
 /* Info */
 void           uiox_pkg_exec_print_info (const uiox_pkg_exec_t *ex);
 void           uiox_pkg_exec_print_stats(uiox_pkg_exec_t *ex);
 
 /* Name helpers: uiox_pkg_state_name and uiox_pkg_err_str are declared in
  * uiox_pkg_types.h (store.c needs the first and cannot reach exec.h);
  * uiox_pkg_ev_name stays here because uiox_pkg_ev_t is defined here. */
 const char    *uiox_pkg_ev_name         (uiox_pkg_ev_t ev);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_PKG_EXEC_H */
 