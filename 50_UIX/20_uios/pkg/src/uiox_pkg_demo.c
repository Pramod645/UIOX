/**
 * @file  uiox_pkg_demo.c
 * @brief UIOX Package Manager demo — simulated package registry + 12 scenarios.
 * @date  2026-06-29
 */

 #include "../include/uiox_pkg_device.h"
 #include <stdio.h>
 #include <string.h>
 
 /* =========================================================================
  * Simulated archive table (replaces real FS/network access)
  * ====================================================================== */
 
 extern void uiox_pkg_store_sim_register(const void *a);
 
 typedef struct {
     char           name[UIOX_PKG_NAME_MAX];
     uiox_pkg_hdr_t hdr;
     uiox_pkg_dep_t deps[UIOX_PKG_MAX_DEPS];
     uint8_t        dep_count;
 } sim_archive_t;
 
 static void make_hdr(uiox_pkg_hdr_t *h, const char *name,
                       uint32_t ver, uint32_t isz)
 {
     memset(h, 0, sizeof(*h));
     h->magic          = UIOX_PKG_MAGIC;
     h->format_version = UIOX_PKG_VERSION;
     h->version        = ver;
     h->installed_size = isz;
     h->archive_size   = isz / 2u;
     strncpy(h->name, name, UIOX_PKG_NAME_MAX - 1u);
     snprintf(h->desc, UIOX_PKG_DESC_MAX, "UIOX package: %s", name);
     strncpy(h->arch, "any", UIOX_PKG_ARCH_MAX - 1u);
 }
 
 static sim_archive_t s_archives[] = {
     /* libuiox-base — no dependencies */
     { "libuiox-base",
       .hdr = {0}, .deps = {0}, .dep_count = 0 },
 
     /* libuiox-net — depends on libuiox-base */
     { "libuiox-net",
       .hdr = {0},
       .deps = {{ "libuiox-base",
                  UIOX_PKG_VER(1,0,0), 0,
                  UIOX_PKG_DEP_REQUIRED }},
       .dep_count = 1 },
 
     /* uiox-shell — depends on libuiox-base + libuiox-net */
     { "uiox-shell",
       .hdr = {0},
       .deps = {
           { "libuiox-base", UIOX_PKG_VER(1,0,0), 0, UIOX_PKG_DEP_REQUIRED },
           { "libuiox-net",  UIOX_PKG_VER(1,0,0), 0, UIOX_PKG_DEP_REQUIRED },
       },
       .dep_count = 2 },
 
     /* uiox-editor — depends on libuiox-base only */
     { "uiox-editor",
       .hdr = {0},
       .deps = {{ "libuiox-base",
                  UIOX_PKG_VER(1,0,0), 0, UIOX_PKG_DEP_REQUIRED }},
       .dep_count = 1 },
 
     /* uiox-devtools — optional dep on uiox-editor */
     { "uiox-devtools",
       .hdr = {0},
       .deps = {
           { "libuiox-base", UIOX_PKG_VER(1,0,0), 0, UIOX_PKG_DEP_REQUIRED },
           { "uiox-editor",  UIOX_PKG_VER(1,0,0), 0, UIOX_PKG_DEP_OPTIONAL },
       },
       .dep_count = 2 },
 
     /* uiox-conflicting — conflicts with uiox-editor */
     { "uiox-conflicting",
       .hdr = {0},
       .deps = {{ "uiox-editor", 0, 0, UIOX_PKG_DEP_CONFLICT }},
       .dep_count = 1 },
 };
 
 static void register_archives(void)
 {
     static const uint32_t sizes[] = {
         512*1024u,   /* libuiox-base   512 KB */
         256*1024u,   /* libuiox-net    256 KB */
         1024*1024u,  /* uiox-shell       1 MB */
         768*1024u,   /* uiox-editor    768 KB */
         2*1024*1024u,/* uiox-devtools    2 MB */
         128*1024u,   /* uiox-conflicting */
     };
     static const uint32_t vers[] = {
         UIOX_PKG_VER(1,0,0),
         UIOX_PKG_VER(1,0,0),
         UIOX_PKG_VER(2,1,0),
         UIOX_PKG_VER(1,3,0),
         UIOX_PKG_VER(1,0,0),
         UIOX_PKG_VER(1,0,0),
     };
     for (uint32_t i = 0u;
          i < sizeof(s_archives)/sizeof(s_archives[0]); i++) {
         make_hdr(&s_archives[i].hdr, s_archives[i].name,
                   vers[i], sizes[i]);
         uiox_pkg_store_sim_register(&s_archives[i]);
     }
 }
 
 /* =========================================================================
  * Event callback
  * ====================================================================== */
 
 static void on_pkg_event(uiox_pkg_ev_t ev, const char *name,
                           uiox_pkg_err_t status, void *ctx)
 {
     (void)ctx;
     printf("  [event] %-18s  pkg=%-32s  status=%s\n",
            uiox_pkg_ev_name(ev), name, uiox_pkg_err_str(status));
 }
 
 /* =========================================================================
  * main
  * ====================================================================== */
 
 int main(void)
 {
     printf("=== UIOX Package Manager Demo ===\n\n");
 
     /* Register simulated archives */
     register_archives();
 
     /* ------------------------------------------------------------------ */
     printf("--- Open ---\n");
     uiox_pkg_device_t     dev;
     uiox_pkg_open_params_t p = {
         .repo      = UIOX_PKG_REPO_RAMFS,
         .repo_path = "/pkg",
         .evt_cb    = on_pkg_event,
     };
     uiox_pkg_err_t rc = uiox_pkg_open(&dev, &p);
     printf("  open rc=%s\n", uiox_pkg_err_str(rc));
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Start ---\n");
     rc = uiox_pkg_start(&dev);
     printf("  start rc=%s\n", uiox_pkg_err_str(rc));
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Device info ---\n");
     uiox_pkg_print_info(&dev);
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Install libuiox-base (no deps) ---\n");
     rc = uiox_pkg_install(&dev, "libuiox-base", UIOX_PKG_VER(1,0,0));
     printf("  install rc=%s\n", uiox_pkg_err_str(rc));
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Install libuiox-net (deps: libuiox-base already done) ---\n");
     rc = uiox_pkg_install(&dev, "libuiox-net", UIOX_PKG_VER(1,0,0));
     printf("  install rc=%s\n", uiox_pkg_err_str(rc));
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Install uiox-shell (deps: base + net) ---\n");
     rc = uiox_pkg_install(&dev, "uiox-shell", UIOX_PKG_VER(2,1,0));
     printf("  install rc=%s\n", uiox_pkg_err_str(rc));
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Install uiox-editor ---\n");
     rc = uiox_pkg_install(&dev, "uiox-editor", UIOX_PKG_VER(1,3,0));
     printf("  install rc=%s\n", uiox_pkg_err_str(rc));
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Install uiox-devtools (optional dep: uiox-editor) ---\n");
     rc = uiox_pkg_install(&dev, "uiox-devtools", UIOX_PKG_VER(1,0,0));
     printf("  install rc=%s\n", uiox_pkg_err_str(rc));
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Re-install libuiox-base (should return ALREADY) ---\n");
     rc = uiox_pkg_install(&dev, "libuiox-base", UIOX_PKG_VER(1,0,0));
     printf("  install rc=%s  (expected ALREADY_INSTALLED)\n",
            uiox_pkg_err_str(rc));
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Query uiox-shell ---\n");
     uiox_pkg_rec_t rec;
     rc = uiox_pkg_query(&dev, "uiox-shell", &rec);
     if (rc == UIOX_PKG_OK) {
         printf("  name=%s  v%u.%u.%u  deps=%u\n",
                rec.hdr.name,
                UIOX_PKG_VER_MAJOR(rec.hdr.version),
                UIOX_PKG_VER_MINOR(rec.hdr.version),
                UIOX_PKG_VER_PATCH(rec.hdr.version),
                rec.dep_count);
     }
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Is uiox-editor installed? ---\n");
     printf("  installed=%s\n",
            uiox_pkg_installed(&dev, "uiox-editor") ? "YES" : "NO");
     printf("  installed(nonexistent)=%s\n",
            uiox_pkg_installed(&dev, "uiox-ghost") ? "YES" : "NO");
 
     /* ------------------------------------------------------------------ */
     printf("\n--- List all installed packages ---\n");
     uiox_pkg_list(&dev, UIOX_PKG_STATE_INSTALLED);
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Upgrade uiox-editor to v1.4.0 ---\n");
     rc = uiox_pkg_upgrade(&dev, "uiox-editor", UIOX_PKG_VER(1,4,0));
     printf("  upgrade rc=%s\n", uiox_pkg_err_str(rc));
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Upgrade-all ---\n");
     rc = uiox_pkg_upgrade_all(&dev);
     printf("  upgrade_all rc=%s\n", uiox_pkg_err_str(rc));
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Remove uiox-devtools (not depended on by others) ---\n");
     rc = uiox_pkg_remove(&dev, "uiox-devtools");
     printf("  remove rc=%s\n", uiox_pkg_err_str(rc));
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Try to remove libuiox-base (uiox-shell depends on it) ---\n");
     rc = uiox_pkg_remove(&dev, "libuiox-base");
     printf("  remove rc=%s  (expected CONFLICT)\n",
            uiox_pkg_err_str(rc));
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Remove uiox-shell first, then libuiox-base ---\n");
     rc = uiox_pkg_remove(&dev, "uiox-shell");
     printf("  remove uiox-shell rc=%s\n", uiox_pkg_err_str(rc));
     rc = uiox_pkg_remove(&dev, "libuiox-base");
     printf("  remove libuiox-base rc=%s\n", uiox_pkg_err_str(rc));
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Final package list ---\n");
     uiox_pkg_list(&dev, UIOX_PKG_STATE_NONE);  /* list all */
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Statistics ---\n");
     uiox_pkg_print_stats(&dev);
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Final device info ---\n");
     uiox_pkg_print_info(&dev);
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Store contents ---\n");
     uiox_pkg_store_print(&dev.subsys.store);
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Stop and close ---\n");
     uiox_pkg_stop(&dev);
     uiox_pkg_close(&dev);
     printf("  Device: CLOSED\n");
 
     printf("\n=== UIOX Package Manager Demo complete ===\n");
     return 0;
 }
 