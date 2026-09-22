/*
 * 30_KIX/32_FS/10_scfs/include/uiox_kix_scfs_buf.h   — v3.0.0
 *
 * UIOX buffer cache — Bach Ch.3 algorithm, scaled for a modern device.
 *
 * GAP FIXES APPLIED (#2 groups, #3 table size, #4 concurrency, #6 TRIM)
 * @version 3.0.0  @date 2026-09-21
 */
#ifndef UIOX_KIX_SCFS_BUF_H
#define UIOX_KIX_SCFS_BUF_H

#include "uiox_klibc.h"
#include "uiox_kix_scfs_inode.h"

#ifndef NBUF
#define NBUF        4096          /* GAP #3: tunable, was 30 */
#endif

#define BLOCK_SIZE  512           /* SINGLE OWNER */

#define B_BUSY    0x01
#define B_VALID   0x02
#define B_DIRTY   0x04
#define B_READ    0x08
#define B_WRITE   0x10
#define B_SHARED  0x20    /* held by >1 reader             */
#define B_DISCARD 0x40    /* queued for TRIM on free       */
#define B_ORDERED 0x80    /* write precedes its metadata   */

#define B_NOBLOCK   ((uint32_t)-1)

/* GAP #4: shared/exclusive buffer lock */
typedef struct buf_lock {
    int16_t   l_excl;
    int16_t   l_readers;
    uint16_t  l_want;
    void     *l_waitq;
} buf_lock_t;

typedef struct buf {
    uint16_t    b_flags;
    uint16_t    b_dev;
    uint32_t    b_blkno;
    int         b_error;
    int         b_resid;
    char        b_data[BLOCK_SIZE];
    struct buf *b_next;
    struct buf *b_prev;
    uint32_t    b_refcnt;      /* GAP #4 */
    buf_lock_t  b_lock;        /* GAP #4 */
    uint32_t    b_lru_seq;     /* clock sweep instead of FIFO */
} buf_t;

extern buf_t buf_pool[NBUF];

buf_t   *bread (uint16_t dev, uint32_t blkno);
buf_t   *breada(uint16_t dev, uint32_t blkno, uint32_t rablkno);
buf_t   *breadn(uint16_t dev, uint32_t blkno, uint32_t nblocks);
void     bwrite(buf_t *bp);
void     brelse(buf_t *bp);
buf_t   *getblk(uint16_t dev, uint32_t blkno);
void     bflush(uint16_t dev);
void     bsync(void);
buf_t   *bread_shared   (uint16_t dev, uint32_t blkno);   /* GAP #4 */
buf_t   *bread_exclusive(uint16_t dev, uint32_t blkno);   /* GAP #4 */

/* GAP #2: group-aware allocation */
#define B_ANYGRP   ((uint32_t)-1)
uint32_t balloc (uint16_t dev, uint32_t grp);
void     bfree  (uint16_t dev, uint32_t blkno);

/* GAP #6: TRIM */
void     bdiscard_queue(uint16_t dev, uint32_t blkno);
int      bdiscard_drain(uint16_t dev, uint32_t max);

uint32_t bmap(inode_t *ip, uint32_t offset);

/* GAP #5: ordered metadata write */
void     bwrite_ordered(buf_t *bp);

#endif /* UIOX_KIX_SCFS_BUF_H */
