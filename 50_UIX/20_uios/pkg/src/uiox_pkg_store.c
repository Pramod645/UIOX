/**
 * @file  uiox_pkg_store.c
 * @brief UIOX Package Manager — store layer (package index + archive I/O).
 *
 * In simulation mode (no real FS) the store keeps the index in RAM
 * and uses an in-memory archive table populated by uiox_pkg_demo.c.
 * On real UIOX the store calls buf_read()/buf_write() from 31_BufferCache.
 *
 * @date  2026-06-29
 */

 #include "../include/uiox_pkg_device.h"
 #include <string.h>
 #include <errno.h>
 #include <stdio.h>
 
 /* =========================================================================
  * Simulation: in-memory package archive table
  * ====================================================================== */
 
 #define STORE_SIM_MAX  32u
 
 typedef struct {
     char           name[UIOX_PKG_NAME_MAX];
     uiox_pkg_hdr_t hdr;
     uiox_pkg_dep_t deps[UIOX_PKG_MAX_DEPS];
     uint8_t        dep_count;
 } sim_archive_t;
 
 static sim_archive_t s_sim_archives[STORE_SIM_MAX];
 static uint32_t      s_sim_count = 0u;
 
 /* Register a simulated package archive (called from demo / test code) */
 void uiox_pkg_store_sim_register(const sim_archive_t *a)
 {
     if (s_sim_count < STORE_SIM_MAX)
         s_sim_archives[s_sim_count++] = *a;
 }
 
 static sim_archive_t *sim_find(const char *name)
 {
     for (uint32_t i = 0u; i < s_sim_count; i++)
         if (strncmp(s_sim_archives[i].name, name,
                     UIOX_PKG_NAME_MAX) == 0)
             return &s_sim_archives[i];
     return NULL;
 }
 
 /* =========================================================================
  * String helpers (no libc strlen in bare kernel context)
  * ====================================================================== */
 
 static size_t pkg_strlen(const char *s)
 { size_t n = 0u; while (s && *s++) n++; return n; }
 
 static void pkg_strncpy(char *dst, const char *src, size_t n)
 {
     size_t i = 0u;
     while (i < n - 1u && src[i]) { dst[i] = src[i]; i++; }
     dst[i] = '\0';
 }
 
 static int pkg_strncmp(const char *a, const char *b, size_t n)
 {
     while (n-- && *a && *b) {
         if (*a != *b) return (int)(unsigned char)*a - (int)(unsigned char)*b;
         a++; b++;
     }
     return 0;
 }
 
 /* =========================================================================
  * Store init / mount
  * ====================================================================== */
 
 uiox_pkg_err_t uiox_pkg_store_init(uiox_pkg_store_t *store,
                                      uiox_pkg_repo_type_t type,
                                      const char *path)
 {
     if (!store || !path) return UIOX_PKG_ERR_INVAL;
     memset(store, 0, sizeof(*store));
     store->repo_type = type;
     pkg_strncpy(store->repo_path, path, UIOX_PKG_URL_MAX);
     return UIOX_PKG_OK;
 }
 
 uiox_pkg_err_t uiox_pkg_store_mount(uiox_pkg_store_t *store)
 {
     if (!store) return UIOX_PKG_ERR_INVAL;
     /*
      * Real UIOX: open /pkg directory via namei() → iget() → bread().
      * Simulation: just mark mounted.
      */
     store->mounted = true;
     uiox_pkg_store_index_load(store);
     printf("  [store] mounted  repo=%s  type=%u  entries=%u\n",
            store->repo_path, store->repo_type, store->index_count);
     return UIOX_PKG_OK;
 }
 
 uiox_pkg_err_t uiox_pkg_store_sync(uiox_pkg_store_t *store)
 {
     if (!store || !store->mounted) return UIOX_PKG_ERR_INVAL;
     if (store->index_dirty) return uiox_pkg_store_index_save(store);
     return UIOX_PKG_OK;
 }
 
 /* =========================================================================
  * Index operations
  * ====================================================================== */
 
 uiox_pkg_err_t uiox_pkg_store_index_load(uiox_pkg_store_t *store)
 {
     if (!store) return UIOX_PKG_ERR_INVAL;
     /*
      * Real UIOX: bread(dev, INDEX_LBA) → parse uiox_pkg_index_hdr_t.
      * Simulation: index starts empty; packages added as installed.
      */
     store->index_count = 0u;
     store->index_dirty = false;
     return UIOX_PKG_OK;
 }
 
 uiox_pkg_err_t uiox_pkg_store_index_save(uiox_pkg_store_t *store)
 {
     if (!store) return UIOX_PKG_ERR_INVAL;
     /*
      * Real UIOX: bwrite(buf) to /pkg/index.upix sector.
      * Simulation: mark clean.
      */
     store->index_dirty = false;
     printf("  [store] index saved (%u entries)\n", store->index_count);
     return UIOX_PKG_OK;
 }
 
 uiox_pkg_index_entry_t *uiox_pkg_store_index_find(uiox_pkg_store_t *store,
                                                     const char *name)
 {
     if (!store || !name) return NULL;
     for (uint32_t i = 0u; i < store->index_count; i++) {
         if (pkg_strncmp(store->index[i].name, name,
                         UIOX_PKG_NAME_MAX) == 0)
             return &store->index[i];
     }
     return NULL;
 }
 
 uiox_pkg_err_t uiox_pkg_store_index_add(uiox_pkg_store_t *store,
                                           const uiox_pkg_rec_t *rec)
 {
     if (!store || !rec) return UIOX_PKG_ERR_INVAL;
     if (store->index_count >= UIOX_PKG_INDEX_MAX_ENTRIES)
         return UIOX_PKG_ERR_OVERFLOW;
     if (uiox_pkg_store_index_find(store, rec->hdr.name))
         return UIOX_PKG_ERR_ALREADY;
     uiox_pkg_index_entry_t *e = &store->index[store->index_count++];
     pkg_strncpy(e->name, rec->hdr.name, UIOX_PKG_NAME_MAX);
     e->version      = rec->hdr.version;
     e->state        = (uint32_t)rec->state;
     e->archive_size = rec->hdr.archive_size;
     memcpy(e->sha256, rec->hdr.sha256, UIOX_PKG_SHA256_LEN);
     store->index_dirty = true;
     return UIOX_PKG_OK;
 }
 
 uiox_pkg_err_t uiox_pkg_store_index_remove(uiox_pkg_store_t *store,
                                              const char *name)
 {
     if (!store || !name) return UIOX_PKG_ERR_INVAL;
     for (uint32_t i = 0u; i < store->index_count; i++) {
         if (pkg_strncmp(store->index[i].name, name,
                         UIOX_PKG_NAME_MAX) == 0) {
             store->index[i] = store->index[--store->index_count];
             store->index_dirty = true;
             return UIOX_PKG_OK;
         }
     }
     return UIOX_PKG_ERR_NOTFOUND;
 }
 
 uiox_pkg_err_t uiox_pkg_store_index_update(uiox_pkg_store_t *store,
                                              const char *name,
                                              uiox_pkg_state_t new_state)
 {
     uiox_pkg_index_entry_t *e = uiox_pkg_store_index_find(store, name);
     if (!e) return UIOX_PKG_ERR_NOTFOUND;
     e->state = (uint32_t)new_state;
     store->index_dirty = true;
     return UIOX_PKG_OK;
 }
 
 /* =========================================================================
  * Archive I/O
  * ====================================================================== */
 
 uiox_pkg_err_t uiox_pkg_store_read_hdr(uiox_pkg_store_t *store,
                                          const char *name,
                                          uiox_pkg_hdr_t *hdr)
 {
     if (!store || !name || !hdr) return UIOX_PKG_ERR_INVAL;
     sim_archive_t *a = sim_find(name);
     if (!a) return UIOX_PKG_ERR_NOTFOUND;
     *hdr = a->hdr;
     store->stats.archive_reads++;
     store->stats.bytes_read += sizeof(*hdr);
     return UIOX_PKG_OK;
 }
 
 uiox_pkg_err_t uiox_pkg_store_load_pkg(uiox_pkg_store_t *store,
                                          const char *name,
                                          uiox_pkg_rec_t *out)
 {
     if (!store || !name || !out) return UIOX_PKG_ERR_INVAL;
     sim_archive_t *a = sim_find(name);
     if (!a) return UIOX_PKG_ERR_NOTFOUND;
 
     memset(out, 0, sizeof(*out));
     out->hdr = a->hdr;
     out->dep_count = a->dep_count;
     for (uint32_t i = 0u; i < a->dep_count; i++)
         out->deps[i] = a->deps[i];
     out->state = UIOX_PKG_STATE_AVAILABLE;
 
     store->stats.archive_reads++;
     store->stats.bytes_read += sizeof(uiox_pkg_hdr_t);
     return UIOX_PKG_OK;
 }
 
 uiox_pkg_err_t uiox_pkg_store_extract(uiox_pkg_store_t *store,
                                         const uiox_pkg_rec_t *pkg,
                                         const char *dest_root)
 {
     if (!store || !pkg || !dest_root) return UIOX_PKG_ERR_INVAL;
     /*
      * Real UIOX: decompress archive data via buf_read() chain,
      * create files via creat()/write() through the FS layer.
      * Simulation: print what would be extracted.
      */
     printf("  [store] extract %-32s → %s  (%u files)\n",
            pkg->hdr.name, dest_root, pkg->file_count);
     for (uint32_t i = 0u; i < pkg->file_count; i++) {
         printf("    %s  (%u B)\n",
                pkg->files[i].path, pkg->files[i].size);
         store->stats.bytes_written += pkg->files[i].size;
     }
     store->stats.archive_reads++;
     return UIOX_PKG_OK;
 }
 
 uiox_pkg_err_t uiox_pkg_store_remove_files(uiox_pkg_store_t *store,
                                              const uiox_pkg_rec_t *pkg)
 {
     if (!store || !pkg) return UIOX_PKG_ERR_INVAL;
     /*
      * Real UIOX: unlink() each file path via the FS namei() layer.
      * Simulation: print.
      */
     printf("  [store] remove files of %-32s (%u files)\n",
            pkg->hdr.name, pkg->file_count);
     return UIOX_PKG_OK;
 }
 
 void uiox_pkg_store_stats(const uiox_pkg_store_t *store,
                             uiox_pkg_store_stats_t *out)
 { if (store && out) memcpy(out, &store->stats, sizeof(*out)); }
 
 void uiox_pkg_store_print(const uiox_pkg_store_t *store)
 {
     if (!store) return;
     printf("  Store: %s  mounted=%d  entries=%u  dirty=%d\n",
            store->repo_path, (int)store->mounted,
            store->index_count, (int)store->index_dirty);
     printf("  Stats: reads=%llu  writes=%llu  errs=%u\n",
            (unsigned long long)store->stats.bytes_read,
            (unsigned long long)store->stats.bytes_written,
            store->stats.errors);
     for (uint32_t i = 0u; i < store->index_count; i++) {
         const uiox_pkg_index_entry_t *e = &store->index[i];
         printf("  [%2u] %-32s  v%u.%u.%u  %s\n",
                i, e->name,
                UIOX_PKG_VER_MAJOR(e->version),
                UIOX_PKG_VER_MINOR(e->version),
                UIOX_PKG_VER_PATCH(e->version),
                uiox_pkg_state_name((uiox_pkg_state_t)e->state));
     }
 }
 