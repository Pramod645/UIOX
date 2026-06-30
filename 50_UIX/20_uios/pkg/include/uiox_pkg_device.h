/**
 * @file  uiox_pkg_device.h
 * @brief UIOX Package Manager — application-facing API (Layer 5).
 *
 * This is what syscall handlers in 40_SystemCallInterface call:
 *   sys_pkg_install(name, version)
 *   sys_pkg_remove(name)
 *   sys_pkg_query(name, buf)
 *
 * @version 1.0.0
 * @date    2026-06-29
 */

 #ifndef UIOX_PKG_DEVICE_H
 #define UIOX_PKG_DEVICE_H
 
 #include "uiox_pkg_subsys.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 typedef struct {
     uiox_pkg_repo_type_t  repo;
     const char           *repo_path;
     uiox_pkg_evt_cb_t     evt_cb;
     void                 *evt_ctx;
 } uiox_pkg_open_params_t;
 
 typedef struct {
     uiox_pkg_subsys_t  subsys;
     bool               open;
 } uiox_pkg_device_t;
 
 /* Lifecycle */
 uiox_pkg_err_t uiox_pkg_open      (uiox_pkg_device_t *dev,
                                     const uiox_pkg_open_params_t *p);
 uiox_pkg_err_t uiox_pkg_start     (uiox_pkg_device_t *dev);
 void           uiox_pkg_stop      (uiox_pkg_device_t *dev);
 void           uiox_pkg_close     (uiox_pkg_device_t *dev);
 
 /* Operations — syscall shims call these */
 uiox_pkg_err_t uiox_pkg_install   (uiox_pkg_device_t *dev,
                                     const char *name, uint32_t version);
 uiox_pkg_err_t uiox_pkg_remove    (uiox_pkg_device_t *dev,
                                     const char *name);
 uiox_pkg_err_t uiox_pkg_upgrade   (uiox_pkg_device_t *dev,
                                     const char *name, uint32_t new_ver);
 uiox_pkg_err_t uiox_pkg_upgrade_all(uiox_pkg_device_t *dev);
 uiox_pkg_err_t uiox_pkg_query     (uiox_pkg_device_t *dev,
                                     const char *name,
                                     uiox_pkg_rec_t *out);
 bool           uiox_pkg_installed (uiox_pkg_device_t *dev,
                                     const char *name);
 void           uiox_pkg_list      (uiox_pkg_device_t *dev,
                                     uiox_pkg_state_t filter);
 void           uiox_pkg_print_info(const uiox_pkg_device_t *dev);
 void           uiox_pkg_print_stats(uiox_pkg_device_t *dev);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_PKG_DEVICE_H */
 