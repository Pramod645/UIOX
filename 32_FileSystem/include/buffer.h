#ifndef UIOX_BUFFER_H
#define UIOX_BUFFER_H

#include "fs_types.h"

/* ─────────────────────────────────────────────────────────────
 * Buffer cache entry
 *
 * The buffer cache sits between the filesystem algorithms and
 * the simulated disk.  Each entry caches one disk block.
 * ───────────────────────────────────────────────────────────── */
typedef struct BufEntry {
    uint32_t       blkno;          /* disk block this entry caches    */
    uint8_t        data[BLOCK_SIZE];
    bool           valid;          /* data matches disk               */
    bool           dirty;          /* written, not yet flushed        */
    bool           locked;         /* I/O in progress                 */
    int            refcount;       /* number of holders               */
    struct BufEntry *next_hash;    /* hash chain                      */
    struct BufEntry *prev_free;    /* free list links                 */
    struct BufEntry *next_free;
} BufEntry;

/* ─────────────────────────────────────────────────────────────
 * Buffer cache API
 * ───────────────────────────────────────────────────────────── */
void      buf_init(void);

/*
 * getblk — find or allocate a cache entry for 'blkno'.
 * Returns a locked BufEntry; caller must call brelse when done.
 */
BufEntry *getblk(uint32_t blkno);

/*
 * bread — read block 'blkno' from disk (via cache).
 */
BufEntry *bread(uint32_t blkno);

/*
 * bwrite — write a dirty buffer back to the simulated disk.
 */
void      bwrite(BufEntry *buf);

/*
 * brelse — release a buffer (decrement refcount; unlock).
 */
void      brelse(BufEntry *buf);

/* Flush all dirty buffers to disk (fsync equivalent) */
void      buf_sync(void);

/* Debug: print cache state */
void      buf_print(void);

#endif /* UIOX_BUFFER_H */
