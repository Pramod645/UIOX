/**
 * @file  uiox_pkg_store.h
 * @brief UIOX Package Manager — store layer: UNFS index + archive I/O.
 *
 * PATH  32_FS/04_pkg/include/uiox_pkg_store.h
 *
 * The store reads and writes through the UIOX filesystem, the same way any
 * other module in 32_FS does.  Two access shapes, deliberately different in
 * kind — see the store context below for why:
 *
 *   index.upix and the .upkg archives   addressed by BLOCK NUMBER, read and
 *                                       written directly (no path lookup).
 *                                       unfs_bdev_read/write, 4096-byte
 *                                       UNFS blocks.
 *   extracted files under /usr/pkg      addressed by PATH, so they need the
 *                                       UNFS VFS.  NOT YET AVAILABLE —
 *                                       extract/remove return UNSUP until
 *                                       01_fsa provides file creation above
 *                                       bmap_alloc().
 *
 * RECORDS COME FROM THE POOL.  uiox_pkg_store_load_pkg() returns a pointer
 * into the record pool (uiox_pkg_buf.h), not a copy.  The caller owns that
 * slot and must call uiox_pkg_rec_free() when done.  uiox_pkg_store_index_add()
 * stores the pointer, so the index and the pool share one lifetime.
 *
 * @version 1.1.0
 * @date    2026-10-27
 */

 #ifndef UIOX_PKG_STORE_H
 #define UIOX_PKG_STORE_H
 
 #include "uiox_pkg_buf.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * Repository URL schemes
  * ====================================================================== */
 
 typedef enum {
     UIOX_PKG_REPO_LOCAL  = 0,  /**< /pkg directory on rootfs            */
     UIOX_PKG_REPO_RAMFS  = 1,  /**< In-memory ramfs (embedded packages) */
     UIOX_PKG_REPO_NET    = 2,  /**< Network (not implemented)           */
 } uiox_pkg_repo_type_t;
 
 /* =========================================================================
  * Package index (on-disk format)
  *
  * Stored at /pkg/index.upix — one UNFS block (4096 B) holding the header,
  * with one block per index entry.  UIOX_PKG_INDEX_SECTOR_SIZE below is kept
  * for the header's own 512-byte field alignment; the I/O unit is
  * UNFS_BLOCK_SIZE, not this.
  * ====================================================================== */
 
 #define UIOX_PKG_INDEX_MAX_ENTRIES  256u
 #define UIOX_PKG_INDEX_SECTOR_SIZE  512u
 
 typedef struct __attribute__((packed)) {
     uint32_t magic;                     /**< UIOX_PKG_INDEX_MAGIC         */
     uint32_t version;
     uint32_t entry_count;
     uint32_t _pad;
 } uiox_pkg_index_hdr_t;
 
 typedef struct __attribute__((packed)) {
     char     name   [UIOX_PKG_NAME_MAX];
     uint32_t version;
     uint32_t state;                     /**< uiox_pkg_state_t             */
     uint64_t archive_lba;               /**< UNFS block of the .upkg      */
     uint32_t archive_size;
     uint8_t  sha256[UIOX_PKG_SHA256_LEN];
     uint8_t  _pad[8];
 } uiox_pkg_index_entry_t;
 
 /* =========================================================================
  * Store statistics
  * ====================================================================== */
 
 typedef struct {
     uint64_t bytes_read;
     uint64_t bytes_written;
     uint32_t index_reads;
     uint32_t index_writes;
     uint32_t archive_reads;
     uint32_t errors;
 } uiox_pkg_store_stats_t;
 
 /* =========================================================================
  * Store context
  * ====================================================================== */
 
 typedef struct {
     uiox_pkg_repo_type_t   repo_type;
     char                   repo_path[UIOX_PKG_URL_MAX]; /**< e.g. "/pkg" */
     uiox_pkg_store_stats_t stats;
     bool                   mounted;
 
     /* ── real-IO addressing (UIOX_PKG_REAL_IO only) ───────────────────
      * The repository is a region of a block device, not a directory walk,
      * so the store addresses it by number.  'dev' is UNFS's device id —
      * uiox_uint32_t, the type every unfs_bdev_* call takes (unfs_io.h:38).
      * 'index_blk' is the UNFS block holding index.upix.
      *
      * UNFS blocks are 4096 bytes (unfs_format.h:129), NOT 512: one block
      * spans 8 bcache sectors, and unfs_bdev_* does that conversion
      * internally.  Pass BLOCK numbers here, never sectors.
      *
      * Use the uiox_ spelling throughout — uiox_fs.h warns that
      * uiox_uint32_t and uint32_t are DIFFERENT types in C, and mixing
      * them is a hard error under -Werror.
      *
      * BOTH MUST BE SET BY THE CALLER before uiox_pkg_store_mount():
      * store_init() does not take them, and mount() validates index_blk
      * rather than defaulting it — an unset location is a caller error
      * (UIOX_PKG_ERR_INVAL), not a failed mount. */
     uiox_uint32_t          dev;
     uiox_uint32_t          index_blk;
 
     /* Index cache (in memory), backed by the record pool */
     uiox_pkg_index_entry_t index[UIOX_PKG_INDEX_MAX_ENTRIES];
     uint32_t               index_count;
     bool                   index_dirty;
 } uiox_pkg_store_t;
 
 /* =========================================================================
  * Store API
  * ====================================================================== */
 
 uiox_pkg_err_t uiox_pkg_store_init    (uiox_pkg_store_t *store,
                                         uiox_pkg_repo_type_t type,
                                         const char *path);
 uiox_pkg_err_t uiox_pkg_store_mount   (uiox_pkg_store_t *store);
 uiox_pkg_err_t uiox_pkg_store_sync    (uiox_pkg_store_t *store);
 
 /* Index operations */
 uiox_pkg_err_t uiox_pkg_store_index_load  (uiox_pkg_store_t *store);
 uiox_pkg_err_t uiox_pkg_store_index_save  (uiox_pkg_store_t *store);
 uiox_pkg_index_entry_t *uiox_pkg_store_index_find(
                                         uiox_pkg_store_t *store,
                                         const char *name);
 uiox_pkg_err_t uiox_pkg_store_index_add   (uiox_pkg_store_t *store,
                                         uiox_pkg_rec_t *rec);
 uiox_pkg_err_t uiox_pkg_store_index_remove(uiox_pkg_store_t *store,
                                         const char *name);
 uiox_pkg_err_t uiox_pkg_store_index_update(uiox_pkg_store_t *store,
                                         const char *name,
                                         uiox_pkg_state_t new_state);
 
 /* Archive I/O
  *
  * load_pkg returns a record from the pool and the caller frees it; the
  * default path is the simulated archive table.  Set UIOX_PKG_REAL_IO to
  * build the UNFS/bcache path instead. */
 uiox_pkg_rec_t *uiox_pkg_store_load_pkg (uiox_pkg_store_t *store,
                                          const char *name);
 uiox_pkg_err_t uiox_pkg_store_read_hdr  (uiox_pkg_store_t *store,
                                          const char *name,
                                          uiox_pkg_hdr_t *hdr);
 uiox_pkg_err_t uiox_pkg_store_extract   (uiox_pkg_store_t *store,
                                          const uiox_pkg_rec_t *pkg,
                                          const char *dest_root);
 uiox_pkg_err_t uiox_pkg_store_remove_files(uiox_pkg_store_t *store,
                                          const uiox_pkg_rec_t *pkg);
 
 /* Simulation hook — registers an in-memory archive.  Declared in the header
  * so the definition and every caller agree on the type (they did not before:
  * the demo declared it as taking const void *). */
 typedef struct uiox_pkg_sim_archive {
     char           name[UIOX_PKG_NAME_MAX];
     uiox_pkg_hdr_t hdr;
     uiox_pkg_dep_t deps[UIOX_PKG_MAX_DEPS];
     uint8_t        dep_count;
 } uiox_pkg_sim_archive_t;
 
 void uiox_pkg_store_sim_register(const uiox_pkg_sim_archive_t *a);
 
 /* Stats */
 void uiox_pkg_store_stats   (const uiox_pkg_store_t *store,
                              uiox_pkg_store_stats_t *out);
 void uiox_pkg_store_print   (const uiox_pkg_store_t *store);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_PKG_STORE_H */
 