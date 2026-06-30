/**
 * @file  uiox_pkg_store.h
 * @brief UIOX Package Manager — store layer: FAT32/ramfs index + archive I/O.
 *
 * The store reads/writes through the UIOX FS layer (32_FileSystem)
 * buf_read() / buf_write() calls, exactly like any other kernel driver.
 *
 * @version 1.0.0
 * @date    2026-06-29
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
     UIOX_PKG_REPO_RAMFS  = 1,  /**< In-memory ramfs (embedded packages)  */
     UIOX_PKG_REPO_NET    = 2,  /**< Network (future — stub for now)      */
 } uiox_pkg_repo_type_t;
 
 /* =========================================================================
  * Package index (on-disk format)
  *
  * Stored as a flat binary file at /pkg/index.upix
  * Each entry = fixed-size slot: name + version + state + LBA offset.
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
     uint64_t archive_lba;               /**< First LBA of .upkg file      */
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
     /* Index cache (in memory) */
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
                                         const uiox_pkg_rec_t *rec);
 uiox_pkg_err_t uiox_pkg_store_index_remove(uiox_pkg_store_t *store,
                                         const char *name);
 uiox_pkg_err_t uiox_pkg_store_index_update(uiox_pkg_store_t *store,
                                         const char *name,
                                         uiox_pkg_state_t new_state);
 
 /* Archive I/O */
 uiox_pkg_err_t uiox_pkg_store_read_hdr(uiox_pkg_store_t *store,
                                         const char *name,
                                         uiox_pkg_hdr_t *hdr);
 uiox_pkg_err_t uiox_pkg_store_load_pkg(uiox_pkg_store_t *store,
                                         const char *name,
                                         uiox_pkg_rec_t *out);
 uiox_pkg_err_t uiox_pkg_store_extract (uiox_pkg_store_t *store,
                                         const uiox_pkg_rec_t *pkg,
                                         const char *dest_root);
 uiox_pkg_err_t uiox_pkg_store_remove_files(uiox_pkg_store_t *store,
                                         const uiox_pkg_rec_t *pkg);
 
 /* Stats */
 void           uiox_pkg_store_stats   (const uiox_pkg_store_t *store,
                                         uiox_pkg_store_stats_t *out);
 void           uiox_pkg_store_print   (const uiox_pkg_store_t *store);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_PKG_STORE_H */
 