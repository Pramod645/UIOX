/**
 * @file  uiox_pkg_device.c
 * @brief UIOX Package Manager — application-facing device API.
 * @date  2026-06-29
 */

 #include "../include/uiox_pkg_device.h"
 #include <string.h>
 #include <errno.h>
 #include <stdio.h>
 
 uiox_pkg_err_t uiox_pkg_open(uiox_pkg_device_t *dev,
                                const uiox_pkg_open_params_t *p)
 {
     if (!dev || !p || !p->repo_path) return UIOX_PKG_ERR_INVAL;
     memset(dev, 0, sizeof(*dev));
     uiox_pkg_err_t rc = uiox_pkg_subsys_init(&dev->subsys,
                                                p->repo,
                                                p->repo_path);
     if (rc != UIOX_PKG_OK) return rc;
     if (p->evt_cb)
         uiox_pkg_subsys_set_cb(&dev->subsys, p->evt_cb, p->evt_ctx);
     dev->open = true;
     return UIOX_PKG_OK;
 }
 
 uiox_pkg_err_t uiox_pkg_start(uiox_pkg_device_t *dev)
 { if (!dev || !dev->open) return UIOX_PKG_ERR_INVAL;
   return uiox_pkg_subsys_start(&dev->subsys); }
 
 void uiox_pkg_stop(uiox_pkg_device_t *dev)
 { if (!dev || !dev->open) return; uiox_pkg_subsys_stop(&dev->subsys); }
 
 void uiox_pkg_close(uiox_pkg_device_t *dev)
 { if (!dev || !dev->open) return; uiox_pkg_stop(dev); dev->open = false; }
 
 uiox_pkg_err_t uiox_pkg_install(uiox_pkg_device_t *dev,
                                   const char *name, uint32_t version)
 { if (!dev || !dev->open) return UIOX_PKG_ERR_INVAL;
   return uiox_pkg_subsys_install(&dev->subsys, name, version); }
 
 uiox_pkg_err_t uiox_pkg_remove(uiox_pkg_device_t *dev, const char *name)
 { if (!dev || !dev->open) return UIOX_PKG_ERR_INVAL;
   return uiox_pkg_subsys_remove(&dev->subsys, name); }
 
 uiox_pkg_err_t uiox_pkg_upgrade(uiox_pkg_device_t *dev,
                                   const char *name, uint32_t new_ver)
 { if (!dev || !dev->open) return UIOX_PKG_ERR_INVAL;
   return uiox_pkg_subsys_upgrade(&dev->subsys, name, new_ver); }
 
 uiox_pkg_err_t uiox_pkg_upgrade_all(uiox_pkg_device_t *dev)
 { if (!dev || !dev->open) return UIOX_PKG_ERR_INVAL;
   return uiox_pkg_subsys_upgrade_all(&dev->subsys); }
 
 uiox_pkg_err_t uiox_pkg_query(uiox_pkg_device_t *dev,
                                 const char *name, uiox_pkg_rec_t *out)
 { if (!dev || !dev->open) return UIOX_PKG_ERR_INVAL;
   return uiox_pkg_subsys_query(&dev->subsys, name, out); }
 
 bool uiox_pkg_installed(uiox_pkg_device_t *dev, const char *name)
 { if (!dev || !dev->open) return false;
   return uiox_pkg_subsys_is_installed(&dev->subsys, name); }
 
 void uiox_pkg_list(uiox_pkg_device_t *dev, uiox_pkg_state_t filter)
 { if (dev && dev->open) uiox_pkg_subsys_list(&dev->subsys, filter); }
 
 void uiox_pkg_print_info(const uiox_pkg_device_t *dev)
 { if (dev && dev->open) uiox_pkg_subsys_print_info(&dev->subsys); }
 
 void uiox_pkg_print_stats(uiox_pkg_device_t *dev)
 { if (dev && dev->open) uiox_pkg_subsys_print_stats(&dev->subsys); }
 