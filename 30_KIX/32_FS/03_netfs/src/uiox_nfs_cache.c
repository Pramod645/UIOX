/**
 * @file  uiox_nfs_cache.c
 * @brief UIOX NFS — attribute + page cache. No libc.
 * @date  2026-07-07
 */

 #include "../include/uiox_nfs_cache.h"

 extern void uiox_fw_printf(const char *fmt, ...);
 
 static void c_memset(void *d,int v,size_t n)
 { uint8_t *p=(uint8_t*)d;while(n--)*p++=(uint8_t)v; }
 static void c_memcpy(void *d,const void *s,size_t n)
 { uint8_t *dp=(uint8_t*)d;const uint8_t *sp=(const uint8_t*)s;while(n--)*dp++=*sp++; }
 
 /* Simple FNV-1a hash of FH bytes */
 uint64_t uiox_nfs_fh_hash(const uiox_nfs_fh_t *fh)
 {
     uint64_t h = 14695981039346656037ULL;
     for (uint32_t i=0u;i<fh->len;i++)
         h = (h ^ fh->data[i]) * 1099511628211ULL;
     return h;
 }
 
 void uiox_nfs_cache_init(uiox_nfs_cache_t *c,
                            uint64_t (*get_uptime_ms)(void))
 {
     c_memset(c, 0, sizeof(*c));
     c->get_uptime_ms = get_uptime_ms;
 }
 
 /* ── Attribute cache ─────────────────────────────────────────── */
 
 uiox_nfs_attr_t *uiox_nfs_acache_get(uiox_nfs_cache_t *c, uint64_t key)
 {
     uint64_t now = c->get_uptime_ms ? c->get_uptime_ms() : 0u;
     uint32_t idx = (uint32_t)(key % UIOX_NFS_ACACHE_ENTRIES);
     uiox_nfs_acache_entry_t *e = &c->acache[idx];
     if (e->valid && e->key == key && now < e->expire_ms) {
         c->acache_hits++;
         return &e->attr;
     }
     c->acache_misses++;
     return NULL;
 }
 
 void uiox_nfs_acache_put(uiox_nfs_cache_t *c, uint64_t key,
                            const uiox_nfs_attr_t *attr)
 {
     uint64_t now = c->get_uptime_ms ? c->get_uptime_ms() : 0u;
     uint32_t idx = (uint32_t)(key % UIOX_NFS_ACACHE_ENTRIES);
     uiox_nfs_acache_entry_t *e = &c->acache[idx];
     e->key       = key;
     e->attr      = *attr;
     e->expire_ms = now + UIOX_NFS_ACACHE_TTL_MS;
     e->valid     = true;
 }
 
 void uiox_nfs_acache_inval(uiox_nfs_cache_t *c, uint64_t key)
 {
     uint32_t idx = (uint32_t)(key % UIOX_NFS_ACACHE_ENTRIES);
     if (c->acache[idx].key == key) c->acache[idx].valid = false;
 }
 
 /* ── Page cache ──────────────────────────────────────────────── */
 
 static uint64_t page_key(uint64_t fh_key, uint64_t offset)
 { return fh_key ^ (offset >> 12u) * 6364136223846793005ULL; }
 
 uiox_nfs_pcache_entry_t *uiox_nfs_pcache_get(uiox_nfs_cache_t *c,
                                                uint64_t fh_key,
                                                uint64_t offset)
 {
     uint64_t k = page_key(fh_key, offset & ~(uint64_t)(UIOX_NFS_PAGE_SIZE-1u));
     uint32_t idx = (uint32_t)(k % UIOX_NFS_PCACHE_ENTRIES);
     uiox_nfs_pcache_entry_t *e = &c->pcache[idx];
     if (e->valid && e->key == k && e->page_offset == offset) {
         c->pcache_hits++;
         return e;
     }
     c->pcache_misses++;
     return NULL;
 }
 
 uiox_nfs_pcache_entry_t *uiox_nfs_pcache_alloc(uiox_nfs_cache_t *c,
                                                  uint64_t fh_key,
                                                  uint64_t offset)
 {
     uint64_t pg_off = offset & ~(uint64_t)(UIOX_NFS_PAGE_SIZE-1u);
     uint64_t k = page_key(fh_key, pg_off);
     uint32_t idx = (uint32_t)(k % UIOX_NFS_PCACHE_ENTRIES);
     uiox_nfs_pcache_entry_t *e = &c->pcache[idx];
     /* Evict if dirty (caller should have flushed first) */
     c_memset(e->data, 0, UIOX_NFS_PAGE_SIZE);
     e->key         = k;
     e->page_offset = pg_off;
     e->dirty       = false;
     e->valid       = true;
     return e;
 }
 
 void uiox_nfs_pcache_mark_dirty(uiox_nfs_cache_t *c,
                                   uiox_nfs_pcache_entry_t *e)
 { (void)c; e->dirty = true; }
 
 void uiox_nfs_pcache_inval(uiox_nfs_cache_t *c, uint64_t fh_key)
 {
     for (uint32_t i=0u;i<UIOX_NFS_PCACHE_ENTRIES;i++) {
         uiox_nfs_pcache_entry_t *e = &c->pcache[i];
         if (e->valid && (e->key >> 32u) == (fh_key >> 32u))
             e->valid = false;
     }
 }
 
 uiox_nfs_err_t uiox_nfs_cache_flush(uiox_nfs_cache_t *c,
                                        uiox_nfs_writeback_fn_t wb,
                                        void *priv)
 {
     for (uint32_t i=0u;i<UIOX_NFS_PCACHE_ENTRIES;i++) {
         uiox_nfs_pcache_entry_t *e = &c->pcache[i];
         if (!e->valid || !e->dirty) continue;
         uiox_nfs_err_t rc = wb(e->key, e->page_offset,
                                  e->data, UIOX_NFS_PAGE_SIZE, priv);
         if (rc != UIOX_NFS_OK) return rc;
         e->dirty = false;
         c->writeback_count++;
     }
     return UIOX_NFS_OK;
 }
 
 void uiox_nfs_cache_print(const uiox_nfs_cache_t *c)
 {
     uiox_fw_printf("[NFS cache] attr: hits=%u misses=%u\n",
                     c->acache_hits, c->acache_misses);
     uiox_fw_printf("[NFS cache] page: hits=%u misses=%u writebacks=%u\n",
                     c->pcache_hits, c->pcache_misses, c->writeback_count);
 }
 