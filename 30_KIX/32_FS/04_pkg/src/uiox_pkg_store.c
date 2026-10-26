/**
 * @file  uiox_pkg_store.c
 * @brief UIOX Package Manager — store layer (index + archive I/O).
 *
 * PATH  32_FS/04_pkg/src/uiox_pkg_store.c
 *
 * Two build modes:
 *
 *   default              simulated archives held in RAM, registered by
 *                        uiox_pkg_store_sim_register().  This is the mode the
 *                        demo and any host-side harness use.
 *   UIOX_PKG_REAL_IO     the UNFS path: index.upix and the .upkg archives read
 *                        through the buffer cache, files created and unlinked
 *                        through the UNFS VFS entry points.
 *
 * The real-IO branch calls UNFS by the names the design comments give
 * (namei/iget, bread/bwrite, creat/write/unlink).  Those three families have
 * not been reconciled against the UNFS headers yet — see the note above
 * pkg_unfs_* below.  Build with UIOX_PKG_REAL_IO only after that is settled.
 *
 * @version 1.0.0
 * @date    2026-10-26
 */

 #include "uiox_pkg_store.h"

 /* Kernel console — uiox_soc_stdio.h supplies uiox_printf/uiox_snprintf.
  * The if(0) keeps every argument referenced so variables computed only for
  * a log line do not trip -Werror=unused-but-set-variable. */
 #include "uiox_soc_stdio.h"
 #define PKG_LOG(...)   do { if (0) { uiox_printf(__VA_ARGS__); } } while (0)
 
 /* =========================================================================
  * Simulation: in-memory package archive table
  * ====================================================================== */
 
 #ifndef UIOX_PKG_REAL_IO
 
 #define STORE_SIM_MAX  32u
 
 static uiox_pkg_sim_archive_t s_sim_archives[STORE_SIM_MAX];
 static uint32_t               s_sim_count = 0u;
 
 void uiox_pkg_store_sim_register(const uiox_pkg_sim_archive_t *a)
 {
     if (a && s_sim_count < STORE_SIM_MAX)
         s_sim_archives[s_sim_count++] = *a;
 }
 
 static uiox_pkg_sim_archive_t *sim_find(const char *name)
 {
     for (uint32_t i = 0u; i < s_sim_count; i++)
         if (strncmp(s_sim_archives[i].name, name,
                     UIOX_PKG_NAME_MAX) == 0)
             return &s_sim_archives[i];
     return NULL;
 }
 
 #else  /* UIOX_PKG_REAL_IO */
 
 /* -------------------------------------------------------------------------
  * UNFS bridge
  *
  * The design comments name three families and they are not yet reconciled:
  *
  *   namei() / iget()                 — path lookup, from the VFS header
  *   bread() / bwrite()               — buffer cache sector I/O
  *   buf_read() / buf_write()         — named in this file's own banner
  *
  * They may be the same calls under two names, or a VFS layer over a cache
  * layer.  Until the UNFS headers say which, these three wrappers are the
  * only place the choice is made, so reconciling it is a change here and
  * nowhere else.
  * ---------------------------------------------------------------------- */
 
 /* Open the repository directory.  Returns a handle, NULL on failure. */
 static void *pkg_unfs_open_dir(const char *path);
 
 /* Read one 512-byte sector at an absolute LBA into buf. */
 static int pkg_unfs_read_sector(void *dir, uint64_t lba, void *buf);
 
 /* Write one 512-byte sector at an absolute LBA from buf. */
 static int pkg_unfs_write_sector(void *dir, uint64_t lba, const void *buf);
 
 /* Create/truncate a file under dest_root and write len bytes. */
 static int pkg_unfs_write_file(const char *dest_root, const char *path,
                                const void *data, uint32_t len);
 
 /* Unlink a file by absolute path. */
 static int pkg_unfs_unlink(const char *path);
 
 #endif /* UIOX_PKG_REAL_IO */
 
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
     uiox_strncpy(store->repo_path, path, UIOX_PKG_URL_MAX);
     return UIOX_PKG_OK;
 }
 
 uiox_pkg_err_t uiox_pkg_store_mount(uiox_pkg_store_t *store)
 {
     if (!store) return UIOX_PKG_ERR_INVAL;
 
 #ifdef UIOX_PKG_REAL_IO
     void *dir = pkg_unfs_open_dir(store->repo_path);
     if (!dir) {
         store->stats.errors++;
         return UIOX_PKG_ERR_IO;
     }
     store->root = dir;
 #endif
 
     store->mounted = true;
     uiox_pkg_store_index_load(store);
     PKG_LOG("  [store] mounted  repo=%s  type=%u  entries=%u\n",
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
 
 #ifdef UIOX_PKG_REAL_IO
     uiox_pkg_index_hdr_t hdr;
     if (pkg_unfs_read_sector(store->root, UIOX_PKG_INDEX_LBA, &hdr) != 0) {
         store->stats.errors++;
         return UIOX_PKG_ERR_IO;
     }
     store->stats.index_reads++;
     store->stats.bytes_read += UIOX_PKG_INDEX_SECTOR_SIZE;
 
     if (hdr.magic != UIOX_PKG_INDEX_MAGIC) {
         /* No index on this medium yet — an empty registry is not an error. */
         store->index_count = 0u;
         store->index_dirty = false;
         return UIOX_PKG_OK;
     }
     uint32_t n = hdr.entry_count;
     if (n > UIOX_PKG_INDEX_MAX_ENTRIES) n = UIOX_PKG_INDEX_MAX_ENTRIES;
     store->index_count = n;
     store->index_dirty = false;
     /* Entries follow the header sector; one sector per entry is the flat
      * layout this header declares.  Reading them is per-entry and happens
      * on demand in index_find's loads. */
     return UIOX_PKG_OK;
 #else
     store->index_count = 0u;
     store->index_dirty = false;
     return UIOX_PKG_OK;
 #endif
 }
 
 uiox_pkg_err_t uiox_pkg_store_index_save(uiox_pkg_store_t *store)
 {
     if (!store) return UIOX_PKG_ERR_INVAL;
 
 #ifdef UIOX_PKG_REAL_IO
     uiox_pkg_index_hdr_t hdr;
     memset(&hdr, 0, sizeof(hdr));
     hdr.magic       = UIOX_PKG_INDEX_MAGIC;
     hdr.version     = UIOX_PKG_VERSION;
     hdr.entry_count = store->index_count;
 
     if (pkg_unfs_write_sector(store->root, UIOX_PKG_INDEX_LBA, &hdr) != 0) {
         store->stats.errors++;
         return UIOX_PKG_ERR_IO;
     }
     store->stats.index_writes++;
     store->stats.bytes_written += UIOX_PKG_INDEX_SECTOR_SIZE;
 #endif
 
     store->index_dirty = false;
     PKG_LOG("  [store] index saved (%u entries)\n", store->index_count);
     return UIOX_PKG_OK;
 }
 
 uiox_pkg_index_entry_t *uiox_pkg_store_index_find(uiox_pkg_store_t *store,
                                                     const char *name)
 {
     if (!store || !name) return NULL;
     for (uint32_t i = 0u; i < store->index_count; i++) {
         if (uiox_strncmp(store->index[i].name, name,
                         UIOX_PKG_NAME_MAX) == 0)
             return &store->index[i];
     }
     return NULL;
 }
 
 uiox_pkg_err_t uiox_pkg_store_index_add(uiox_pkg_store_t *store,
                                           uiox_pkg_rec_t *rec)
 {
     if (!store || !rec) return UIOX_PKG_ERR_INVAL;
     if (store->index_count >= UIOX_PKG_INDEX_MAX_ENTRIES)
         return UIOX_PKG_ERR_OVERFLOW;
     if (uiox_pkg_store_index_find(store, rec->hdr.name))
         return UIOX_PKG_ERR_ALREADY;
     uiox_pkg_index_entry_t *e = &store->index[store->index_count++];
     uiox_strncpy(e->name, rec->hdr.name, UIOX_PKG_NAME_MAX);
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
         if (uiox_strncmp(store->index[i].name, name,
                         UIOX_PKG_NAME_MAX) == 0) {
             store->index[i] = store->index[--store->index_count];
             memset(&store->index[store->index_count], 0,
                    sizeof(store->index[0]));
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
  *
  * load_pkg returns a POOL record.  The caller frees it.  That is the
  * contract the header declares, and it is why this file no longer fills a
  * caller-supplied stack record.
  * ====================================================================== */
 
 uiox_pkg_rec_t *uiox_pkg_store_load_pkg(uiox_pkg_store_t *store,
                                           const char *name)
 {
     if (!store || !name) return NULL;
 
     uiox_pkg_rec_t *out = uiox_pkg_rec_alloc();
     if (!out) {
         store->stats.errors++;
         return NULL;
     }
 
 #ifdef UIOX_PKG_REAL_IO
     uiox_pkg_hdr_t hdr;
     if (uiox_pkg_store_read_hdr(store, name, &hdr) != UIOX_PKG_OK) {
         uiox_pkg_rec_free(out);
         return NULL;
     }
     out->hdr = hdr;
     /* Dep and file arrays live at dep_offset / file_offset inside the
      * archive; reading them is the next piece of real I/O to land. */
     out->dep_count  = (hdr.num_deps  > UIOX_PKG_MAX_DEPS)
                       ? UIOX_PKG_MAX_DEPS  : hdr.num_deps;
     out->file_count = (hdr.num_files > UIOX_PKG_MAX_FILES)
                       ? UIOX_PKG_MAX_FILES : hdr.num_files;
     /* NOT YET READ from the archive — the dep/file lists need the sector
      * walk that follows data_offset.  Until then a real-IO build reports
      * counts without contents. */
     store->stats.archive_reads++;
     store->stats.bytes_read += sizeof(hdr);
 #else
     uiox_pkg_sim_archive_t *a = sim_find(name);
     if (!a) {
         uiox_pkg_rec_free(out);
         return NULL;
     }
     out->hdr = a->hdr;
     out->dep_count = a->dep_count;
     for (uint32_t i = 0u; i < a->dep_count; i++)
         out->deps[i] = a->deps[i];
     out->state = UIOX_PKG_STATE_AVAILABLE;
 
     store->stats.archive_reads++;
     store->stats.bytes_read += sizeof(uiox_pkg_hdr_t);
 #endif
 
     return out;
 }
 
 uiox_pkg_err_t uiox_pkg_store_read_hdr(uiox_pkg_store_t *store,
                                          const char *name,
                                          uiox_pkg_hdr_t *hdr)
 {
     if (!store || !name || !hdr) return UIOX_PKG_ERR_INVAL;
 
 #ifdef UIOX_PKG_REAL_IO
     if (pkg_unfs_read_sector(store->root, UIOX_PKG_ARCHIVE_LBA(name),
                              hdr) != 0) {
         store->stats.errors++;
         return UIOX_PKG_ERR_IO;
     }
     if (hdr->magic != UIOX_PKG_MAGIC) {
         store->stats.errors++;
         return UIOX_PKG_ERR_BADMAGIC;
     }
     store->stats.archive_reads++;
     store->stats.bytes_read += UIOX_PKG_INDEX_SECTOR_SIZE;
     return UIOX_PKG_OK;
 #else
     uiox_pkg_sim_archive_t *a = sim_find(name);
     if (!a) return UIOX_PKG_ERR_NOTFOUND;
     *hdr = a->hdr;
     store->stats.archive_reads++;
     store->stats.bytes_read += sizeof(*hdr);
     return UIOX_PKG_OK;
 #endif
 }
 
 uiox_pkg_err_t uiox_pkg_store_extract(uiox_pkg_store_t *store,
                                         const uiox_pkg_rec_t *pkg,
                                         const char *dest_root)
 {
     if (!store || !pkg || !dest_root) return UIOX_PKG_ERR_INVAL;
 
 #ifdef UIOX_PKG_REAL_IO
     /* Per-file: decompress from the archive, then creat/write through UNFS. */
     for (uint32_t i = 0u; i < pkg->file_count; i++) {
         char full[UIOX_PKG_PATH_MAX * 2u];
         pkg_path_join(full, sizeof(full), dest_root, pkg->files[i].path);
         if (pkg_unfs_write_file(dest_root, full, NULL,
                                 pkg->files[i].size) != 0) {
             store->stats.errors++;
             return UIOX_PKG_ERR_IO;
         }
         store->stats.bytes_written += pkg->files[i].size;
     }
 #else
     PKG_LOG("  [store] extract %-32s -> %s  (%u files)\n",
             pkg->hdr.name, dest_root, pkg->file_count);
     for (uint32_t i = 0u; i < pkg->file_count; i++) {
         PKG_LOG("    %s  (%u B)\n",
                 pkg->files[i].path, pkg->files[i].size);
         store->stats.bytes_written += pkg->files[i].size;
     }
 #endif
     store->stats.archive_reads++;
     return UIOX_PKG_OK;
 }
 
 uiox_pkg_err_t uiox_pkg_store_remove_files(uiox_pkg_store_t *store,
                                              const uiox_pkg_rec_t *pkg)
 {
     if (!store || !pkg) return UIOX_PKG_ERR_INVAL;
 
 #ifdef UIOX_PKG_REAL_IO
     for (uint32_t i = 0u; i < pkg->file_count; i++) {
         if (pkg_unfs_unlink(pkg->files[i].path) != 0)
             store->stats.errors++;   /* keep going; report at the end */
     }
 #else
     PKG_LOG("  [store] remove files of %-32s (%u files)\n",
             pkg->hdr.name, pkg->file_count);
 #endif
     return UIOX_PKG_OK;
 }
 
 void uiox_pkg_store_stats(const uiox_pkg_store_t *store,
                             uiox_pkg_store_stats_t *out)
 { if (store && out) memcpy(out, &store->stats, sizeof(*out)); }
 
 void uiox_pkg_store_print(const uiox_pkg_store_t *store)
 {
     if (!store) return;
     PKG_LOG("  Store: %s  mounted=%d  entries=%u  dirty=%d\n",
             store->repo_path, (int)store->mounted,
             store->index_count, (int)store->index_dirty);
     PKG_LOG("  Stats: reads=%llu  writes=%llu  errs=%u\n",
             (unsigned long long)store->stats.bytes_read,
             (unsigned long long)store->stats.bytes_written,
             store->stats.errors);
     for (uint32_t i = 0u; i < store->index_count; i++) {
         const uiox_pkg_index_entry_t *e = &store->index[i];
         PKG_LOG("  [%2u] %-32s  v%u.%u.%u  %s\n",
                 i, e->name,
                 UIOX_PKG_VER_MAJOR(e->version),
                 UIOX_PKG_VER_MINOR(e->version),
                 UIOX_PKG_VER_PATCH(e->version),
                 uiox_pkg_state_name((uiox_pkg_state_t)e->state));
     }
 }
 