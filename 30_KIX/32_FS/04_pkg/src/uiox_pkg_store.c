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
 *   UIOX_PKG_REAL_IO     the UNFS path.  Block I/O is SETTLED:
 *                        unfs_bdev_read/write(dev, blk, buf) — 4096-byte
 *                        UNFS blocks, sector conversion internal
 *                        (unfs_io.h:38-39).  File creation is NOT:
 *                        extract and unlink stay UIOX_PKG_ERR_UNSUP until
 *                        01_fsa grows a creat/write path above bmap_alloc().
 *
 * @version 1.0.0
 * @date    2026-10-26
 */

 #include "uiox_pkg_store.h"
 #include "unfs_io.h"       /* unfs_bdev_read / unfs_bdev_write — 4096 B blocks */
 #include "unfs_errno.h"    /* UNFS_OK */


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
 * The wrappers below are the SINGLE place the UNFS call choice is made.
 * Everything else in this file calls them, so reconciling the API is a
 * change here and nowhere else.
 *
 * Confirmed from the headers:
 *   unfs_bdev_read/write(dev, blk, buf)   unfs_io.h:38-39
 *     — takes a UNFS block number (4096-byte units) and does the sector
 *       conversion internally.  Returns UNFS_OK or UNFS_EIO.
 *
 * NOT YET AVAILABLE, and the reason extract/unlink stay simulated:
 *   a file-creation path above bmap_alloc().  bmap_alloc() itself is
 *   implemented (bmap.c, @version 3.1.0 — see its case 3/4 placement
 *   logic), but nothing turns "write these bytes to this path" into an
 *   inode plus an extent run.  When it exists it will be:
 *       namei(path, cwd, uid, gid)   namei.h:199
 *       dir_add(dir, name, len, ino, type)   namei.h:186
 *   Both already have definitions; they need a caller.
 * ---------------------------------------------------------------------- */

/* Read/write one UNFS BLOCK (4096 B) at an absolute block number.
 *
 * 'dir' is the store's device field boxed as void* — there is no directory
 * object, because unfs_mount.c keeps no mount table ("it does NOT register
 * in a mount table", :5).  Call sites pass &store->dev. */
static int   pkg_unfs_read_blk (void *dir, uiox_uint32_t blk, void *buf);
static int   pkg_unfs_write_blk(void *dir, uiox_uint32_t blk,
                                const void *buf);

/* BLOCKED — see the note above.  Return -1 until 01_fsa grows the path. */
static int   pkg_unfs_write_file(const char *dest_root, const char *path,
                                 const void *data, uiox_uint32_t len);
static int   pkg_unfs_unlink   (const char *path);

/* ── definitions for the three that are buildable now ──────────────── */

static int pkg_unfs_read_blk(void *dir, uiox_uint32_t blk, void *buf)
{
    if (!dir || !buf) return -1;
    return (unfs_bdev_read(*(uiox_uint32_t *)dir, blk, buf) == UNFS_OK)
           ? 0 : -1;
}

static int pkg_unfs_write_blk(void *dir, uiox_uint32_t blk, const void *buf)
{
    if (!dir || !buf) return -1;
    return (unfs_bdev_write(*(uiox_uint32_t *)dir, blk, buf) == UNFS_OK)
           ? 0 : -1;
}

static int pkg_unfs_write_file(const char *dest_root, const char *path,
                               const void *data, uiox_uint32_t len)
{
    (void)dest_root; (void)path; (void)data; (void)len;
    return -1;   /* 01_fsa has no creat/write above bmap_alloc yet */
}

static int pkg_unfs_unlink(const char *path)
{
    (void)path;
    return -1;   /* 01_fsa has no unlink above dir_remove yet */
}
 
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
    /* No directory handle: the repository is addressed by block number.
     * unfs_mount.c:5 is explicit that it keeps no mount table, so there is
     * nothing to look up here — 'dev' and 'index_blk' are set by the
     * caller before store_mount() runs. */
    if (store->index_blk == 0u) {
        store->stats.errors++;
        return UIOX_PKG_ERR_INVAL;   /* unset location — not an I/O fault */
    }
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
     if (pkg_unfs_read_blk(&store->dev, store->index_blk, &hdr) != 0) {
         store->stats.errors++;
         return UIOX_PKG_ERR_IO;
     }
     store->stats.index_reads++;
     store->stats.bytes_read += UNFS_BLOCK_SIZE;
 
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
 
     if (pkg_unfs_write_blk(&store->dev, store->index_blk, &hdr) != 0) {
         store->stats.errors++;
         return UIOX_PKG_ERR_IO;
     }
     store->stats.index_writes++;
     store->stats.bytes_written += UNFS_BLOCK_SIZE;
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
     /* The archive's block comes from the index entry — the struct already
      * declares archive_lba (uiox_pkg_store.h).  There is no name→LBA
      * function in the tree; the index IS that mapping. */
     uiox_pkg_index_entry_t *ie = uiox_pkg_store_index_find(store, name);
     if (!ie) return UIOX_PKG_ERR_NOTFOUND;

     if (pkg_unfs_read_blk(&store->dev, (uiox_uint32_t)ie->archive_lba,
                           hdr) != 0) {
         store->stats.errors++;
         return UIOX_PKG_ERR_IO;
     }
     if (hdr->magic != UIOX_PKG_MAGIC) {
         store->stats.errors++;
         return UIOX_PKG_ERR_BADMAGIC;
     }
     store->stats.archive_reads++;
     store->stats.bytes_read += UNFS_BLOCK_SIZE;
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
     /* BLOCKED: needs the 01_fsa creat/write path, which does not exist
      * above bmap_alloc() yet.  Do NOT fake this — a successful return
      * here would mark a package installed with no files on disk.
      *
      * When it lands: namei → unfs_alloc_inode_bit → unfs_write_inode_disk
      * → dir_add, then writei for the data.  See pkg_unfs_write_file. */
     store->stats.errors++;
     return UIOX_PKG_ERR_UNSUP;
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
     /* BLOCKED: same missing piece as extract — unlink needs namei plus
      * dir_remove, and removing the data blocks needs the extent free path.
      * Faking success would leave files on disk while the registry says
      * they are gone. */
     (void)pkg;
     store->stats.errors++;
     return UIOX_PKG_ERR_UNSUP;
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
 