/**
 * @file  uiox_pkg_exec.c
 * @brief UIOX Package Manager — plan executor.
 *
 * PATH  32_FS/04_pkg/src/uiox_pkg_exec.c
 *
 * run_plan() walks plan->entries[] and exec_plan_entry() performs each one.
 * Records are taken from the pool for the duration of an entry and released
 * before the next runs, so a single install never holds more than one slot.
 *
 * @version 1.0.0
 * @date    2026-10-26
 */

 #include "uiox_pkg_exec.h"

 /* Kernel console — uiox_soc_stdio.h supplies uiox_printf/uiox_snprintf.
  * The if(0) keeps every argument referenced so variables computed only for
  * a log line do not trip -Werror=unused-but-set-variable. */
 #include "uiox_soc_stdio.h"
 #define PKG_LOG(...)   do { if (0) { uiox_printf(__VA_ARGS__); } } while (0)
 
 static void fire(uiox_pkg_exec_t *ex, uiox_pkg_ev_t ev,
                  const char *name, uiox_pkg_err_t status)
 {
     if (ex->evt_cb)
         ex->evt_cb(ev, name, status, ex->evt_ctx);
 }
 
 /* Execute a single plan entry */
 static uiox_pkg_err_t exec_plan_entry(uiox_pkg_exec_t *ex,
                                        const uiox_pkg_plan_entry_t *pe)
 {
     uiox_pkg_rec_t *rec = NULL;
     uiox_pkg_err_t  rc;
 
     switch (pe->op) {
 
     case UIOX_PKG_OP_INSTALL:
         fire(ex, UIOX_PKG_EV_INSTALL_START, pe->name, UIOX_PKG_OK);
         rec = uiox_pkg_store_load_pkg(&ex->store, pe->name);
         if (!rec) {
             fire(ex, UIOX_PKG_EV_INSTALL_FAIL, pe->name,
                  UIOX_PKG_ERR_NOTFOUND);
             return UIOX_PKG_ERR_NOTFOUND;
         }
         rec->state = UIOX_PKG_STATE_INSTALLING;
         /* Extract files to /usr/pkg/<name> */
         rc = uiox_pkg_store_extract(&ex->store, rec, "/usr/pkg");
         if (rc != UIOX_PKG_OK) {
             fire(ex, UIOX_PKG_EV_INSTALL_FAIL, pe->name, rc);
             uiox_pkg_rec_free(rec);
             return rc;
         }
         rec->state = UIOX_PKG_STATE_INSTALLED;
         uiox_pkg_store_index_add(&ex->store, rec);
         uiox_pkg_store_index_update(&ex->store, pe->name,
                                     UIOX_PKG_STATE_INSTALLED);
         ex->installed_count++;
         ex->install_ops++;
         uiox_pkg_rec_free(rec);
         fire(ex, UIOX_PKG_EV_INSTALL_DONE, pe->name, UIOX_PKG_OK);
         break;
 
     case UIOX_PKG_OP_REMOVE:
         fire(ex, UIOX_PKG_EV_REMOVE_START, pe->name, UIOX_PKG_OK);
         rec = uiox_pkg_store_load_pkg(&ex->store, pe->name);
         if (rec) {
             uiox_pkg_store_remove_files(&ex->store, rec);
             uiox_pkg_rec_free(rec);
         }
         uiox_pkg_store_index_remove(&ex->store, pe->name);
         if (ex->installed_count > 0u) ex->installed_count--;
         ex->remove_ops++;
         fire(ex, UIOX_PKG_EV_REMOVE_DONE, pe->name, UIOX_PKG_OK);
         break;
 
     case UIOX_PKG_OP_UPGRADE:
         fire(ex, UIOX_PKG_EV_UPGRADE_START, pe->name, UIOX_PKG_OK);
         rec = uiox_pkg_store_load_pkg(&ex->store, pe->name);
         if (rec) {
             uiox_pkg_store_remove_files(&ex->store, rec);
             uiox_pkg_rec_free(rec);
         }
         uiox_pkg_store_index_update(&ex->store, pe->name,
                                     UIOX_PKG_STATE_UPGRADING);
         rec = uiox_pkg_store_load_pkg(&ex->store, pe->name);
         if (!rec) {
             ex->error_count++;
             fire(ex, UIOX_PKG_EV_UPGRADE_FAIL, pe->name,
                  UIOX_PKG_ERR_NOTFOUND);
             return UIOX_PKG_ERR_NOTFOUND;
         }
         rc = uiox_pkg_store_extract(&ex->store, rec, "/usr/pkg");
         uiox_pkg_rec_free(rec);
         if (rc != UIOX_PKG_OK) {
             ex->error_count++;
             fire(ex, UIOX_PKG_EV_UPGRADE_FAIL, pe->name, rc);
             return rc;
         }
         uiox_pkg_store_index_update(&ex->store, pe->name,
                                     UIOX_PKG_STATE_INSTALLED);
         ex->upgrade_ops++;
         fire(ex, UIOX_PKG_EV_UPGRADE_DONE, pe->name, UIOX_PKG_OK);
         break;
 
     case UIOX_PKG_OP_KEEP:
     default:
         break;
     }
     return UIOX_PKG_OK;
 }
 
 /* =========================================================================
  * Lifecycle
  * ====================================================================== */
 
 uiox_pkg_err_t uiox_pkg_exec_init(uiox_pkg_exec_t *ex,
                                     uiox_pkg_repo_type_t repo,
                                     const char *repo_path)
 {
     if (!ex || !repo_path) return UIOX_PKG_ERR_INVAL;
     memset(ex, 0, sizeof(*ex));
     ex->state = UIOX_PKG_EXEC_INIT;
     return uiox_pkg_store_init(&ex->store, repo, repo_path);
 }
 
 uiox_pkg_err_t uiox_pkg_exec_start(uiox_pkg_exec_t *ex)
 {
     if (!ex) return UIOX_PKG_ERR_INVAL;
 
     uiox_pkg_buf_init();
 
     uiox_pkg_err_t rc = uiox_pkg_store_mount(&ex->store);
     if (rc != UIOX_PKG_OK) {
         ex->state = UIOX_PKG_EXEC_ERROR;
         return rc;
     }
     uiox_pkg_resolve_init(&ex->resolver, &ex->store);
     ex->state = UIOX_PKG_EXEC_READY;
 
     PKG_LOG("  [pkg] executor ready  repo=%s\n", ex->store.repo_path);
     return UIOX_PKG_OK;
 }
 
 void uiox_pkg_exec_stop(uiox_pkg_exec_t *ex)
 {
     if (!ex) return;
     uiox_pkg_store_sync(&ex->store);
     ex->state = UIOX_PKG_EXEC_OFF;
 }
 
 void uiox_pkg_exec_set_cb(uiox_pkg_exec_t *ex,
                            uiox_pkg_evt_cb_t cb, void *ctx)
 {
     if (!ex) return;
     ex->evt_cb  = cb;
     ex->evt_ctx = ctx;
 }
 
 /* =========================================================================
  * Plan execution
  * ====================================================================== */
 
 static uiox_pkg_err_t run_plan(uiox_pkg_exec_t *ex,
                                 const uiox_pkg_plan_t *plan)
 {
     if (plan->has_conflicts) {
         ex->error_count++;
         return UIOX_PKG_ERR_CONFLICT;
     }
     for (uint32_t i = 0u; i < plan->count; i++) {
         uiox_pkg_err_t rc = exec_plan_entry(ex, &plan->entries[i]);
         if (rc != UIOX_PKG_OK) return rc;
     }
     return UIOX_PKG_OK;
 }
 
 uiox_pkg_err_t uiox_pkg_exec_install(uiox_pkg_exec_t *ex,
                                        const char *name,
                                        uint32_t version)
 {
     if (!ex || !name) return UIOX_PKG_ERR_INVAL;
     if (ex->state != UIOX_PKG_EXEC_READY) return UIOX_PKG_ERR_INVAL;
 
     uiox_pkg_plan_t plan;
     uiox_pkg_err_t rc = uiox_pkg_resolve_install(&ex->resolver,
                                                   name, version, &plan);
     if (rc != UIOX_PKG_OK) {
         if (rc == UIOX_PKG_ERR_CONFLICT) ex->error_count++;
         return rc;
     }
     PKG_LOG("  [pkg] install plan for '%s':\n", name);
     uiox_pkg_resolve_print(&plan);
 
     ex->state = UIOX_PKG_EXEC_BUSY;
     rc = run_plan(ex, &plan);
     ex->state = UIOX_PKG_EXEC_READY;
 
     uiox_pkg_store_sync(&ex->store);
     return rc;
 }
 
 uiox_pkg_err_t uiox_pkg_exec_remove(uiox_pkg_exec_t *ex,
                                       const char *name)
 {
     if (!ex || !name) return UIOX_PKG_ERR_INVAL;
     if (ex->state != UIOX_PKG_EXEC_READY) return UIOX_PKG_ERR_INVAL;
 
     uiox_pkg_plan_t plan;
     uiox_pkg_err_t rc = uiox_pkg_resolve_remove(&ex->resolver, name, &plan);
     if (rc != UIOX_PKG_OK) {
         if (rc == UIOX_PKG_ERR_CONFLICT) ex->error_count++;
         return rc;
     }
     PKG_LOG("  [pkg] remove plan for '%s':\n", name);
     uiox_pkg_resolve_print(&plan);
 
     ex->state = UIOX_PKG_EXEC_BUSY;
     rc = run_plan(ex, &plan);
     ex->state = UIOX_PKG_EXEC_READY;
 
     uiox_pkg_store_sync(&ex->store);
     return rc;
 }
 
 uiox_pkg_err_t uiox_pkg_exec_upgrade(uiox_pkg_exec_t *ex,
                                        const char *name,
                                        uint32_t new_version)
 {
     if (!ex || !name) return UIOX_PKG_ERR_INVAL;
     if (ex->state != UIOX_PKG_EXEC_READY) return UIOX_PKG_ERR_INVAL;
 
     uiox_pkg_plan_t plan;
     uiox_pkg_err_t rc = uiox_pkg_resolve_upgrade(&ex->resolver,
                                                   name, new_version, &plan);
     if (rc != UIOX_PKG_OK) return rc;
     PKG_LOG("  [pkg] upgrade plan for '%s':\n", name);
     uiox_pkg_resolve_print(&plan);
 
     ex->state = UIOX_PKG_EXEC_BUSY;
     rc = run_plan(ex, &plan);
     ex->state = UIOX_PKG_EXEC_READY;
 
     uiox_pkg_store_sync(&ex->store);
     return rc;
 }
 
 uiox_pkg_err_t uiox_pkg_exec_upgrade_all(uiox_pkg_exec_t *ex)
 {
     if (!ex) return UIOX_PKG_ERR_INVAL;
     if (ex->state != UIOX_PKG_EXEC_READY) return UIOX_PKG_ERR_INVAL;
 
     uint32_t checked = 0u;
     for (uint32_t i = 0u; i < ex->store.index_count; i++) {
         const uiox_pkg_index_entry_t *e = &ex->store.index[i];
         if ((uiox_pkg_state_t)e->state != UIOX_PKG_STATE_INSTALLED) continue;
         /* An upgrade needs a candidate version; the store supplies none yet,
          * so the entry is counted and left alone. */
         checked++;
     }
     PKG_LOG("  [pkg] upgrade-all: %u packages checked\n", checked);
     return UIOX_PKG_OK;
 }
 
 /* =========================================================================
  * Query
  * ====================================================================== */
 
 uiox_pkg_err_t uiox_pkg_exec_query(uiox_pkg_exec_t *ex,
                                      const char *name,
                                      uiox_pkg_rec_t **out)
 {
     if (!ex || !name || !out) return UIOX_PKG_ERR_INVAL;
     *out = NULL;
 
     uiox_pkg_index_entry_t *ie = uiox_pkg_store_index_find(&ex->store, name);
     if (!ie) return UIOX_PKG_ERR_NOTFOUND;
 
     uiox_pkg_rec_t *rec = uiox_pkg_store_load_pkg(&ex->store, name);
     if (!rec) return UIOX_PKG_ERR_NOTFOUND;
     rec->state = (uiox_pkg_state_t)ie->state;
     *out = rec;                 /* caller frees */
     return UIOX_PKG_OK;
 }
 
 bool uiox_pkg_exec_is_installed(uiox_pkg_exec_t *ex, const char *name)
 {
     uiox_pkg_rec_t *rec = NULL;
     if (!ex || !name) return false;
     if (uiox_pkg_exec_query(ex, name, &rec) != UIOX_PKG_OK) return false;
     bool yes = (rec->state == UIOX_PKG_STATE_INSTALLED);
     uiox_pkg_rec_free(rec);
     return yes;
 }
 
 void uiox_pkg_exec_list(uiox_pkg_exec_t *ex, uiox_pkg_state_t filter)
 {
     if (!ex) return;
     PKG_LOG("  Package list (filter=%s):\n", uiox_pkg_state_name(filter));
     for (uint32_t i = 0u; i < ex->store.index_count; i++) {
         const uiox_pkg_index_entry_t *e = &ex->store.index[i];
         if (filter != UIOX_PKG_STATE_NONE &&
             (uiox_pkg_state_t)e->state != filter) continue;
         PKG_LOG("  %-32s  v%u.%u.%u  %s\n",
                 e->name,
                 UIOX_PKG_VER_MAJOR(e->version),
                 UIOX_PKG_VER_MINOR(e->version),
                 UIOX_PKG_VER_PATCH(e->version),
                 uiox_pkg_state_name((uiox_pkg_state_t)e->state));
     }
 }
 
 /* =========================================================================
  * Info
  * ====================================================================== */
 
 void uiox_pkg_exec_print_info(const uiox_pkg_exec_t *ex)
 {
     if (!ex) return;
     PKG_LOG("  State        : %s\n", "READY");
     PKG_LOG("  Repo         : %s\n", ex->store.repo_path);
     PKG_LOG("  Installed    : %u\n", ex->installed_count);
     PKG_LOG("  Total in idx : %u\n", ex->store.index_count);
     PKG_LOG("  Install ops  : %u\n", ex->install_ops);
     PKG_LOG("  Remove ops   : %u\n", ex->remove_ops);
     PKG_LOG("  Upgrade ops  : %u\n", ex->upgrade_ops);
     PKG_LOG("  Errors       : %u\n", ex->error_count);
     PKG_LOG("  Rec pool free: %u / %u\n",
             uiox_pkg_rec_free_cnt(), UIOX_PKG_REC_POOL_SIZE);
 }
 
 void uiox_pkg_exec_print_stats(uiox_pkg_exec_t *ex)
 {
     if (!ex) return;
     uiox_pkg_store_stats_t st;
     uiox_pkg_store_stats(&ex->store, &st);
     PKG_LOG("  Store bytes read    : %llu\n",
             (unsigned long long)st.bytes_read);
     PKG_LOG("  Store bytes written : %llu\n",
             (unsigned long long)st.bytes_written);
     PKG_LOG("  Store index reads   : %u\n", st.index_reads);
     PKG_LOG("  Store archive reads : %u\n", st.archive_reads);
     PKG_LOG("  Store errors        : %u\n", st.errors);
 }
 
 /* =========================================================================
  * Name helpers
  * ====================================================================== */
 
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
     default:                        return "?";
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
     case UIOX_PKG_EV_INSTALL_FAIL:  return "INSTALL_FAIL";
     case UIOX_PKG_EV_UPGRADE_FAIL:  return "UPGRADE_FAIL";
     default:                        return "?";
     }
 }
 
 const char *uiox_pkg_err_str(uiox_pkg_err_t e)
 {
     switch (e) {
     case UIOX_PKG_OK:           return "OK";
     case UIOX_PKG_ERR_GENERIC:  return "GENERIC";
     case UIOX_PKG_ERR_INVAL:    return "INVAL";
     case UIOX_PKG_ERR_NOMEM:    return "NOMEM";
     case UIOX_PKG_ERR_IO:       return "IO";
     case UIOX_PKG_ERR_NOTFOUND: return "NOTFOUND";
     case UIOX_PKG_ERR_ALREADY:  return "ALREADY_INSTALLED";
     case UIOX_PKG_ERR_CONFLICT: return "CONFLICT";
     case UIOX_PKG_ERR_BADMAGIC: return "BADMAGIC";
     case UIOX_PKG_ERR_BADCSUM:  return "BADCSUM";
     case UIOX_PKG_ERR_DEPFAIL:  return "DEPFAIL";
     case UIOX_PKG_ERR_OVERFLOW: return "OVERFLOW";
     case UIOX_PKG_ERR_PERM:     return "PERM";
     case UIOX_PKG_ERR_BUSY:     return "BUSY";
     case UIOX_PKG_ERR_UNSUP:    return "UNSUP";
     default:                    return "?";
     }
 }
 