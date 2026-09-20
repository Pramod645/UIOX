/**
 * @file  uiox_nfs_cache.h
 * @brief UIOX NFS — attribute cache and write-back page cache.
 * @version 1.0.0
 */

 #ifndef UIOX_NFS_CACHE_H
 #define UIOX_NFS_CACHE_H
 
 #include "uiox_nfs_types.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 #define UIOX_NFS_ACACHE_ENTRIES  64u    /**< Attribute cache slots      */
 #define UIOX_NFS_ACACHE_TTL_MS   30000u /**< 30 second TTL              */
 #define UIOX_NFS_PCACHE_ENTRIES  32u    /**< Page cache slots           */
 #define UIOX_NFS_PAGE_SIZE       4096u  /**< Cache page size            */
 
 /* =========================================================================
  * Attribute cache entry
  * ====================================================================== */
 
 typedef struct {
     uint64_t        key;         /**< Hashed FH or nodeid              */
     uiox_nfs_attr_t attr;
     uint64_t        expire_ms;   /**< Expiry uptime_ms                 */
     bool            valid;
 } uiox_nfs_acache_entry_t;
 
 /* =========================================================================
  * Page cache entry (write-back)
  * ====================================================================== */
 
 typedef struct {
     uint64_t key;            /**< Hashed FH + page_index              */
     uint64_t page_offset;    /**< Byte offset = page_index * PAGE_SIZE */
     uint8_t  data[UIOX_NFS_PAGE_SIZE];
     bool     dirty;          /**< Needs write-back                    */
     bool     valid;
 } uiox_nfs_pcache_entry_t;
 
 /* =========================================================================
  * Cache context
  * ====================================================================== */
 
 typedef struct {
     uiox_nfs_acache_entry_t acache[UIOX_NFS_ACACHE_ENTRIES];
     uiox_nfs_pcache_entry_t pcache[UIOX_NFS_PCACHE_ENTRIES];
     uint64_t (*get_uptime_ms)(void);  /**< Wired to timer uptime      */
     uint32_t acache_hits;
     uint32_t acache_misses;
     uint32_t pcache_hits;
     uint32_t pcache_misses;
     uint32_t writeback_count;
 } uiox_nfs_cache_t;
 
 /* =========================================================================
  * Cache API
  * ====================================================================== */
 
 void              uiox_nfs_cache_init      (uiox_nfs_cache_t *c,
                                               uint64_t (*get_uptime_ms)(void));
 
 /* Attribute cache */
 uiox_nfs_attr_t  *uiox_nfs_acache_get     (uiox_nfs_cache_t *c, uint64_t key);
 void              uiox_nfs_acache_put     (uiox_nfs_cache_t *c, uint64_t key,
                                               const uiox_nfs_attr_t *attr);
 void              uiox_nfs_acache_inval   (uiox_nfs_cache_t *c, uint64_t key);
 
 /* Page cache */
 uiox_nfs_pcache_entry_t *uiox_nfs_pcache_get(uiox_nfs_cache_t *c,
                                                uint64_t key,
                                                uint64_t offset);
 uiox_nfs_pcache_entry_t *uiox_nfs_pcache_alloc(uiox_nfs_cache_t *c,
                                                 uint64_t key,
                                                 uint64_t offset);
 void              uiox_nfs_pcache_mark_dirty(uiox_nfs_cache_t *c,
                                                uiox_nfs_pcache_entry_t *e);
 void              uiox_nfs_pcache_inval   (uiox_nfs_cache_t *c, uint64_t key);
 
 /** Flush all dirty pages — calls @writeback for each dirty entry. */
 typedef uiox_nfs_err_t (*uiox_nfs_writeback_fn_t)(
     uint64_t key, uint64_t offset,
     const uint8_t *data, uint32_t len, void *priv);
 
 uiox_nfs_err_t    uiox_nfs_cache_flush   (uiox_nfs_cache_t *c,
                                               uiox_nfs_writeback_fn_t wb,
                                               void *priv);
 void              uiox_nfs_cache_print   (const uiox_nfs_cache_t *c);
 
 /* FH → uint64_t hash */
 uint64_t          uiox_nfs_fh_hash       (const uiox_nfs_fh_t *fh);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_NFS_CACHE_H */
 