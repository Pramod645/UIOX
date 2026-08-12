/*
 * 31_BufferCache/00_FileBuff/include/bcache_types.h  — v1.1.0
 *
 * Updated: BLOCK_SIZE split into sector size + page size.
 *          NUM_BUFFERS increased. NUM_DISK_BLOCKS made a platform stub.
 */

 #ifndef UIOX_BCACHE_TYPES_H
 #define UIOX_BCACHE_TYPES_H
 
 #include "uiox_base_types.h"
 
 /* ── Geometry ──────────────────────────────────────────────────────── */
 #define BCACHE_SECTOR_SIZE       512u     /* physical sector — fixed       */
 #define BCACHE_PAGE_SIZE         4096u    /* logical page  = 8 sectors     */
 #define BCACHE_BLOCKS_PER_PAGE   (BCACHE_PAGE_SIZE / BCACHE_SECTOR_SIZE) /* 8 */
 
 /* Keep BLOCK_SIZE as alias for existing code */
 #define BLOCK_SIZE               BCACHE_SECTOR_SIZE
 
 #define NUM_BUFFERS              256u     /* block buffer pool size        */
 #define NUM_HASH_QUEUES          64u      /* hash table size (power of 2)  */
 #define MAX_DEVICES              8u       /* logical device count          */
 
 /* Platform hook — override from 10_BSP for real device size */
 #define NUM_DISK_BLOCKS_DEFAULT  4096u
 __attribute__((weak))
 static inline uint32_t bcache_plat_num_blocks(uint8_t dev)
 {
     (void)dev;
     return NUM_DISK_BLOCKS_DEFAULT;
 }
 
 /* ── Buffer status flags ────────────────────────────────────────────── */
 #define BUF_LOCKED   (1u << 0)   /* buffer is locked — in use            */
 #define BUF_VALID    (1u << 1)   /* contains valid data                  */
 #define BUF_DELWRITE (1u << 2)   /* delayed write — flush before reuse   */
 #define BUF_IOBUSY   (1u << 3)   /* I/O in progress                      */
 #define BUF_WANTED   (1u << 4)   /* a process is waiting for it          */
 #define BUF_OLD      (1u << 5)   /* goes to head of free list            */
 #define BUF_ASYNC    (1u << 6)   /* asynchronous I/O in flight           */
 #define BUF_ERROR    (1u << 7)   /* I/O error occurred                   */
 #define BUF_DIRTY    (1u << 8)   /* page cache dirty (page cache use)    */
 
 /* ── Statistics ─────────────────────────────────────────────────────── */
 typedef struct {
     uint64_t hits;
     uint64_t misses;
     uint64_t delayed_writes;
     uint64_t free_waits;
     uint64_t busy_waits;
     uint64_t reads;
     uint64_t writes;
     uint64_t readaheads;
 } BufStats;
 
 #endif /* UIOX_BCACHE_TYPES_H */
 