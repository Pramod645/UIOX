/*
 * 31_BufferCache/00_FileBuff/include/uiox_page_cache.h
 *
 * UIOX Page Cache — sits above the block buffer cache.
 *
 * Unit: 4096-byte page, identified by (inode_no, file_offset).
 * Uses bread() from bcache.c to fill pages from the block device.
 * Exposes physical address of pages for zero-copy mmap().
 *
 * Relationship to 00_FileBuff block cache:
 *
 *   PAGE CACHE (this file)          BLOCK CACHE (bcache.c)
 *   ─────────────────────           ──────────────────────
 *   unit: 4096 bytes                unit: 512 bytes
 *   key: (inode, offset)            key: (dev, blkno)
 *   used by: VFS read/write/mmap    used by: page cache + FS metadata
 *   exposes: physical address       exposes: data pointer
 *
 * @version 1.0.0  @date 2026-07-29
 */

 #ifndef UIOX_PAGE_CACHE_H
 #define UIOX_PAGE_CACHE_H
 
 #include "uiox_base_types.h"
 #include "bcache_types.h"
 
 /* ── Limits ────────────────────────────────────────────────────────── */
 #define UIOX_PC_MAX_PAGES    128u   /* page cache pool size             */
 #define UIOX_PC_PAGE_SIZE    BCACHE_PAGE_SIZE   /* 4096                 */
 #define UIOX_PC_HASH_QUEUES  32u
 
 /* ── Page state flags ──────────────────────────────────────────────── */
 #define PC_UPTODATE   (1u << 0)   /* data is valid (read from device)  */
 #define PC_DIRTY      (1u << 1)   /* modified — needs writeback         */
 #define PC_LOCKED     (1u << 2)   /* I/O in progress                   */
 #define PC_REFERENCED (1u << 3)   /* recently accessed (clock eviction)*/
 
 /* ── Page cache entry ──────────────────────────────────────────────── */
 typedef struct uiox_page {
     uint32_t        ino;          /* inode number                      */
     uint64_t        offset;       /* page-aligned offset in file       */
     uint8_t         data[UIOX_PC_PAGE_SIZE]; /* page data (DRAM)        */
     uintptr_t       pa;           /* physical address of data[]         */
     uint32_t        flags;        /* PC_* flags                        */
     uint8_t         inuse;        /* reference count                   */
     struct uiox_page *hash_next;  /* hash chain                        */
     struct uiox_page *lru_next;   /* LRU list                          */
     struct uiox_page *lru_prev;
 } uiox_page_t;
 
 /* ── Inode-to-device translation callback ──────────────────────────── */
 /*
  * The page cache calls this to translate a file offset to a
  * block device (dev, blkno) for the bread() call.
  * Registered by the filesystem (SCFS, ext, etc.) at mount time.
  */
 typedef int (*uiox_pc_map_fn_t)(uint32_t  ino,
                                   uint64_t  file_offset,
                                   uint8_t  *dev_out,
                                   uint32_t *blkno_out);
 
 /* ── Public API ────────────────────────────────────────────────────── */
 void  uiox_pc_init(void);
 
 /* Register filesystem block-mapping callback */
 void  uiox_pc_register_map(uiox_pc_map_fn_t fn);
 
 /*
  * uiox_pc_read — fill kbuf with 'len' bytes from file at 'offset'.
  * Hits serve from DRAM page cache. Misses call bread() to fill page.
  * Returns bytes copied, negative on error.
  */
 ssize_t uiox_pc_read(uint32_t ino, uint64_t offset,
                       void *kbuf, size_t len);
 
 /*
  * uiox_pc_write — write 'len' bytes from kbuf into page cache at offset.
  * Marks page dirty — writeback happens on sync/fsync or pressure.
  * Returns bytes written, negative on error.
  */
 ssize_t uiox_pc_write(uint32_t ino, uint64_t offset,
                        const void *kbuf, size_t len);
 
 /*
  * uiox_pc_get_page_pa — returns physical address of the DRAM page
  * holding file data at offset. Used by sys_mmap() for zero-copy.
  * Returns 0 if page not cached or cannot be mapped.
  */
 uintptr_t uiox_pc_get_page_pa(uint32_t ino, uint64_t offset);
 
 /*
  * uiox_pc_writeback — write all dirty pages for inode to device.
  * Called by vfs_fsync() / SYS_FSYNC.
  */
 int  uiox_pc_writeback(uint32_t ino);
 
 /*
  * uiox_pc_writeback_all — write all dirty pages for all inodes.
  * Called by SYS_SYNC / uiox_jr_checkpoint().
  */
 int  uiox_pc_writeback_all(void);
 
 /*
  * uiox_pc_invalidate — evict all pages for inode.
  * Called on file truncation or close.
  */
 void uiox_pc_invalidate(uint32_t ino);




 /* Add to uiox_page_cache.h public API section: */

/*
 * uiox_pc_sync_all   — flush all dirty pages + journal commit + checkpoint.
 *                      Called by SYS_SYNC / SYS_SYNCFS.
 */
int uiox_pc_sync_all(void);

/*
 * uiox_pc_sync_inode — flush dirty pages for one inode + journal commit.
 *                      Called by SYS_FSYNC / SYS_FDATASYNC.
 */
int uiox_pc_sync_inode(uint32_t ino);

 
 #endif /* UIOX_PAGE_CACHE_H */
 