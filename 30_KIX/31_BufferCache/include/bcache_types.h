#ifndef UIOX_BCACHE_TYPES_H
#define UIOX_BCACHE_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* ─────────────────────────────────────────────────────────────
 * Filesystem / disk geometry
 * ───────────────────────────────────────────────────────────── */
#define BLOCK_SIZE       512     /* bytes per disk block             */
#define NUM_BUFFERS      16      /* total buffers in the pool        */
#define NUM_HASH_QUEUES  4       /* hash queues (blkno % 4)          */
#define NUM_DISK_BLOCKS  64      /* simulated disk capacity          */
#define MAX_DEVICES      4       /* logical filesystem/device count  */

/* ─────────────────────────────────────────────────────────────
 * Buffer status flags
 *
 * These correspond exactly to the five conditions
 * 
 * ───────────────────────────────────────────────────────────── */
#define BUF_LOCKED       (1u << 0)  /* buffer busy / locked          */
#define BUF_VALID        (1u << 1)  /* contains valid data           */
#define BUF_DELWRITE     (1u << 2)  /* delayed-write: flush before reuse */
#define BUF_IOBUSY       (1u << 3)  /* I/O in progress               */
#define BUF_WANTED       (1u << 4)  /* a process is waiting for it   */
#define BUF_OLD          (1u << 5)  /* marked old: goes to head of freelist */
#define BUF_ASYNC        (1u << 6)  /* asynchronous I/O in flight    */
#define BUF_ERROR        (1u << 7)  /* I/O error occurred            */

/* ─────────────────────────────────────────────────────────────
 * Cache statistics counters
 * ───────────────────────────────────────────────────────────── */
typedef struct {
    uint64_t hits;           /* getblk found block in cache          */
    uint64_t misses;         /* getblk did not find block in cache   */
    uint64_t delayed_writes; /* scenario 3: delayed-write flushes    */
    uint64_t free_waits;     /* scenario 4: slept waiting free buf   */
    uint64_t busy_waits;     /* scenario 5: slept on locked buffer   */
    uint64_t reads;          /* bread calls                          */
    uint64_t writes;         /* bwrite calls                         */
    uint64_t readaheads;     /* breada second-block initiations      */
} BufStats;

#endif /* UIOX_BCACHE_TYPES_H */
