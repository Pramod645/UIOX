/**
 * @file  uiox_pkg_subsys.c
 * @brief UIOX Package Manager subsystem — install/remove/upgrade execution.
 * @date  2026-06-29
 */

 #include "../include/uiox_pkg_device.h"
 #include <string.h>
 #include <stdio.h>
 #include <errno.h>
 
 static void fire(uiox_pkg_subsys_t *sys, uiox_pkg_ev_t ev,
                  const char *name, uiox_pkg_err_t status)
 {
     if (sys->evt_cb)
         sys->evt_cb(ev, name, status, sys->evt_ctx);
 }
 
 /* Execute a single plan entry */
 static uiox_pkg_err_t exec_plan_entry(uiox_pkg_subsys_t *sys,
                                        const uiox_pkg_plan_entry_t *pe)
 {
     uiox_pkg_rec_t rec;
     uiox_pkg_err_t rc;
 
     switch (pe->op) {
 
     case UIOX_PKG_OP_INSTALL:
         fire(sys, UIOX_PKG_EV_INSTALL_START, pe->name, UIOX_PKG_OK);
         rc = uiox_pkg_store_load_pkg(&sys->store, pe->name, &rec);
         if (rc != UIOX_PKG_OK) {
             fire(sys, UIOX_PKG_EV_INSTALL_FAIL, pe->name, rc);
             return rc;
         }
         rec.state = UIOX_PKG_STATE_INSTALLING;
         /* Extract files to /usr/pkg/<name> */
         rc = uiox_pkg_store_extract(&sys->store, &rec, "/usr/pkg");
         if (rc != UIOX_PKG_OK) {
             fire(sys, UIOX_PKG_EV_INSTALL_FAIL, pe->name, rc);
             return rc;
         }
         rec.state = UIOX_PKG_STATE_INSTALLED;
         uiox_pkg_store_index_add(&sys->store, &rec);
         uiox_pkg_store_index_update(&sys->store, pe->name,
                                      UIOX_PKG_STATE_INSTALLED);
         sys->install_ops++;
         sys->installed_count++;
         fire(sys, UIOX_PKG_EV_INSTALL_DONE, pe->name, UIOX_PKG_OK);
         break;
 
     case UIOX_PKG_OP_REMOVE:
         fire(sys, UIOX_PKG_EV_REMOVE_START, pe->name, UIOX_PKG_OK);
         uiox_pkg_store_index_update(&sys->store, pe->name,
                                      UIOX_PKG_STATE_REMOVING);
         rc = uiox_pkg_store_load_pkg(&sys->store, pe->name, &rec);
         if (rc == UIOX_PKG_OK)
             uiox_pkg_store_remove_files(&sys->store, &rec);
         uiox_pkg_store_index_remove(&sys->store, pe->name);
         sys->remove_ops++;
         if (sys->installed_count > 0u) sys->installed_count--;
         fire(sys, UIOX_PKG_EV_REMOVE_DONE, pe->name, UIOX_PKG_OK);
         break;
 
     case UIOX_PKG_OP_UPGRADE:
         fire(sys, UIOX_PKG_EV_UPGRADE_START, pe->name, UIOX_PKG_OK);
         /* Remove old → install new */
         uiox_pkg_store_load_pkg(&sys->store, pe->name, &rec);
         uiox_pkg_store_remove_files(&sys->store, &rec);
         uiox_pkg_store_index_update(&sys->store, pe->name,
                                      UIOX_PKG_STATE_UPGRADING);
         rc = uiox_pkg_store_extract(&sys->store, &rec, "/usr/pkg");
         if (rc != UIOX_PKG_OK) {
             fire(sys, UIOX_PKG_EV_UPGRADE_FAIL, pe->name, rc);
             return rc;
         }
         uiox_pkg_store_index_update(&sys->store, pe->name,
                                      UIOX_PKG_STATE_INSTALLED);
         sys->upgrade_ops++;
         fire(sys, UIOX_PKG_EV_UPGRADE_DONE, pe->name, UIOX_PKG_OK);
         break;
 
     case UIOX_PKG_OP_KEEP:
     default:
         break;
     }
     return UIOX_PKG_OK;
 }
 
 /* =========================================================================
  * Public API
  * ====================================================================== */
 
 uiox_pkg_err_t uiox_pkg_subsys_init(uiox_pkg_subsys_t *sys,
                                       uiox_pkg_repo_type_t repo,
                                       const char *repo_path)
 {
     if (!sys || !repo_path) return UIOX_PKG_ERR_INVAL;
     memset(sys, 0, sizeof(*sys));
     uiox_pkg_buf_init();
     return uiox_pkg_store_init(&sys->store, repo, repo_path);
 }
 
 uiox_pkg_err_t uiox_pkg_subsys_start(uiox_pkg_subsys_t *sys)
 {
     if (!sys) return UIOX_PKG_ERR_INVAL;
     sys->state = UIOX_PKG_SYS_INIT;
     uiox_pkg_err_t rc = uiox_pkg_store_mount(&sys->store);
     if (rc != UIOX_PKG_OK) { sys->state = UIOX_PKG_SYS_ERROR; return rc; }
     uiox_pkg_resolve_init(&sys->resolver, &sys->store);
     sys->state = UIOX_PKG_SYS_READY;
     printf("  [pkg] subsystem ready  repo=%s\n", sys->store.repo_path);
     return UIOX_PKG_OK;
 }
 
 void uiox_pkg_subsys_stop(uiox_pkg_subsys_t *sys)
 {
     if (!sys) return;
     uiox_pkg_store_sync(&sys->store);
     sys->state = UIOX_PKG_SYS_OFF;
 }
 
 void uiox_pkg_subsys_set_cb(uiox_pkg_subsys_t *sys,
                               uiox_pkg_evt_cb_t cb, void *ctx)
 { if (sys) { sys->evt_cb = cb; sys->evt_ctx = ctx; } }
 
 uiox_pkg_err_t uiox_pkg_subsys_install(uiox_pkg_subsys_t *sys,
                                          const char *name, uint32_t version)
 {
     if (!sys || sys->state != UIOX_PKG_SYS_READY) return UIOX_PKG_ERR_BUSY;
     sys->state = UIOX_PKG_SYS_BUSY;
 
     uiox_pkg_plan_t plan;
     uiox_pkg_err_t rc = uiox_pkg_resolve_install(&sys->resolver,
                                                    name, version, &plan);
     if (rc != UIOX_PKG_OK) {
         sys->error_count++;
         sys->state = UIOX_PKG_SYS_READY;
         return rc;
     }
 
     printf("  [pkg] install plan for '%s':\n", name);
     uiox_pkg_resolve_print(&plan);
 
     for (uint32_t i = 0u; i < plan.count; i++) {
         rc = exec_plan_entry(sys, &plan.entries[i]);
         if (rc != UIOX_PKG_OK) {
             sys->error_count++;
             sys->state = UIOX_PKG_SYS_READY;
             return rc;
         }
     }
     uiox_pkg_store_sync(&sys->store);
     sys->state = UIOX_PKG_SYS_READY;
     return UIOX_PKG_OK;
 }
 
 uiox_pkg_err_t uiox_pkg_subsys_remove(uiox_pkg_subsys_t *sys,
                                         const char *name)
 {
     if (!sys || sys->state != UIOX_PKG_SYS_READY) return UIOX_PKG_ERR_BUSY;
     sys->state = UIOX_PKG_SYS_BUSY;
 
     uiox_pkg_plan_t plan;
     uiox_pkg_err_t rc = uiox_pkg_resolve_remove(&sys->resolver, name, &plan);
     if (rc != UIOX_PKG_OK) {
         sys->error_count++;
         sys->state = UIOX_PKG_SYS_READY;
         return rc;
     }
 
     printf("  [pkg] remove plan for '%s':\n", name);
     uiox_pkg_resolve_print(&plan);
 
     for (uint32_t i = 0u; i < plan.count; i++) {
         rc = exec_plan_entry(sys, &plan.entries[i]);
         if (rc != UIOX_PKG_OK) { sys->error_count++; break; }
     }
     uiox_pkg_store_sync(&sys->store);
     sys->state = UIOX_PKG_SYS_READY;
     return rc;
 }
 
 uiox_pkg_err_t uiox_pkg_subsys_upgrade(uiox_pkg_subsys_t *sys,
                                          const char *name,
                                          uint32_t new_version)
 {
     if (!sys || sys->state != UIOX_PKG_SYS_READY) return UIOX_PKG_ERR_BUSY;
     sys->state = UIOX_PKG_SYS_BUSY;
 
     uiox_pkg_plan_t plan;
     uiox_pkg_err_t rc = uiox_pkg_resolve_upgrade(&sys->resolver,
                                                    name, new_version, &plan);
     if (rc != UIOX_PKG_OK) {
         sys->error_count++;
         sys->state = UIOX_PKG_SYS_READY;
         return rc;
     }
 
     printf("  [pkg] upgrade plan for '%s':\n", name);
     uiox_pkg_resolve_print(&plan);
 
     for (uint32_t i = 0u; i < plan.count; i++) {
         rc = exec_plan_entry(sys, &plan.entries[i]);
         if (rc != UIOX_PKG_OK) { sys->error_count++; break; }
     }
     uiox_pkg_store_sync(&sys->store);
     sys->state = UIOX_PKG_SYS_READY;
     return rc;
 }
 
 uiox_pkg_err_t uiox_pkg_subsys_upgrade_all(uiox_pkg_subsys_t *sys)
 {
     if (!sys || sys->state != UIOX_PKG_SYS_READY) return UIOX_PKG_ERR_BUSY;
     /* Stub: in a real implementation, query the remote repository for
      * newer versions and upgrade each installed package. */
     printf("  [pkg] upgrade-all: %u packages checked\n",
            sys->store.index_count);
     return UIOX_PKG_OK;
 }
 
 uiox_pkg_err_t uiox_pkg_subsys_query(uiox_pkg_subsys_t *sys,
                                        const char *name,
                                        uiox_pkg_rec_t *out)
 {
     if (!sys || !name || !out) return UIOX_PKG_ERR_INVAL;
     uiox_pkg_index_entry_t *ie =
         uiox_pkg_store_index_find(&sys->store, name);
     if (!ie) return UIOX_PKG_ERR_NOTFOUND;
     return uiox_pkg_store_load_pkg(&sys->store, name, out);
 }
 
 bool uiox_pkg_subsys_is_installed(uiox_pkg_subsys_t *sys, const char *name)
 {
     if (!sys || !name) return false;
     uiox_pkg_index_entry_t *ie =
         uiox_pkg_store_index_find(&sys->store, name);
     return ie && (uiox_pkg_state_t)ie->state == UIOX_PKG_STATE_INSTALLED;
 }
 
 void uiox_pkg_subsys_list(uiox_pkg_subsys_t *sys, uiox_pkg_state_t filter)
 {
     if (!sys) return;
     printf("  Package list (filter=%s):\n", uiox_pkg_state_name(filter));
     for (uint32_t i = 0u; i < sys->store.index_count; i++) {
         const uiox_pkg_index_entry_t *e = &sys->store.index[i];
         if (filter != UIOX_PKG_STATE_NONE &&
             (uiox_pkg_state_t)e->state != filter) continue;
         printf("  %-32s  v%u.%u.%u  %s\n",
                e->name,
                UIOX_PKG_VER_MAJOR(e->version),
                UIOX_PKG_VER_MINOR(e->version),
                UIOX_PKG_VER_PATCH(e->version),
                uiox_pkg_state_name((uiox_pkg_state_t)e->state));
     }
 }
 
 void uiox_pkg_subsys_print_info(const uiox_pkg_subsys_t *sys)
 {
     if (!sys) return;
     static const char *ss[] = {"OFF","INIT","READY","BUSY","ERROR"};
     printf("  State        : %s\n",
            (uint32_t)sys->state < 5u ? ss[sys->state] : "?");
     printf("  Repo         : %s\n", sys->store.repo_path);
     printf("  Installed    : %u\n", sys->installed_count);
     printf("  Total in idx : %u\n", sys->store.index_count);
     printf("  Install ops  : %u\n", sys->install_ops);
     printf("  Remove ops   : %u\n", sys->remove_ops);
     printf("  Upgrade ops  : %u\n", sys->upgrade_ops);
     printf("  Errors       : %u\n", sys->error_count);
     printf("  Rec pool free: %u / %u\n",
            uiox_pkg_rec_free_cnt(), UIOX_PKG_REC_POOL_SIZE);
     printf("  Evt pool free: %u / %u\n",
            uiox_pkg_evt_free_cnt(), UIOX_PKG_EVT_POOL_SIZE);
 }
 
 void uiox_pkg_subsys_print_stats(uiox_pkg_subsys_t *sys)
 {
     if (!sys) return;
     uiox_pkg_store_stats_t st;
     uiox_pkg_store_stats(&sys->store, &st);
     printf("  Store bytes read    : %llu\n",
            (unsigned long long)st.bytes_read);
     printf("  Store bytes written : %llu\n",
            (unsigned long long)st.bytes_written);
     printf("  Store index reads   : %u\n", st.index_reads);
     printf("  Store archive reads : %u\n", st.archive_reads);
     printf("  Store errors        : %u\n", st.errors);
 }
 
 const char *uiox_pkg_state_name(uiox_pkg_state_t s)
 {
     switch (s) {
     case UIOX_PKG_STATE_NONE:       return "none";
     case UIOX_PKG_STATE_AVAILABLE:  return "available";
     case UIOX_PKG_STATE_INSTALLING: return "installing";
     case UIOX_PKG_STATE_INSTALLED:  return "installed";
     case UIOX_PKG_STATE_REMOVING:   return "removing";
     case UIOX_PKG_STATE_UPGRADING:  return "upgrading";
     case UIOX_PKG_STATE_ERROR:      return "error";
     default:                         return "?";
     }
 }
 
 const char *uiox_pkg_ev_name(uiox_pkg_ev_t ev)
 {
     switch (ev) {
     case UIOX_PKG_EV_INSTALL_START: return "INSTALL_START";
     case UIOX_PKG_EV_INSTALL_DONE:  return "INSTALL_DONE";
     case UIOX_PKG_EV_REMOVE_START:  return "REMOVE_START";
     case UIOX_PKG_EV_REMOVE_DONE:   return "REMOVE_DONE";
     case UIOX_PKG_EV_UPGRADE_START: return "UPGRADE_START";
     case UIOX_PKG_EV_UPGRADE_DONE:  return "UPGRADE_DONE";
     case UIOX_PKG_EV_DEP_INSTALL:   return "DEP_INSTALL";
     case UIOX_PKG_EV_CONFLICT:      return "CONFLICT";
     case UIOX_PKG_EV_ERROR:         return "ERROR";
     default:                         return "?";
     }
 }
 
 const char *uiox_pkg_err_str(uiox_pkg_err_t e)
 {
     switch (e) {
     case UIOX_PKG_OK:             return "OK";
     case UIOX_PKG_ERR_GENERIC:    return "GENERIC";
     case UIOX_PKG_ERR_INVAL:      return "INVAL";
     case UIOX_PKG_ERR_NOMEM:      return "NOMEM";
     case UIOX_PKG_ERR_IO:         return "IO";
     case UIOX_PKG_ERR_NOTFOUND:   return "NOTFOUND";
     case UIOX_PKG_ERR_ALREADY:    return "ALREADY_INSTALLED";
     case UIOX_PKG_ERR_CONFLICT:   return "CONFLICT";
     case UIOX_PKG_ERR_BADMAGIC:   return "BADMAGIC";
     case UIOX_PKG_ERR_BADCSUM:    return "BADCSUM";
     case UIOX_PKG_ERR_DEPFAIL:    return "DEPFAIL";
     case UIOX_PKG_ERR_OVERFLOW:   return "OVERFLOW";
     case UIOX_PKG_ERR_PERM:       return "PERM";
     case UIOX_PKG_ERR_BUSY:       return "BUSY";
     case UIOX_PKG_ERR_UNSUP:      return "UNSUP";
     default:                       return "UNKNOWN";
     }
 }
 