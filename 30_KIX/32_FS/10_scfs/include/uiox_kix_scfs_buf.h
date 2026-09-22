/*
 * 30_KIX/32_FS/10_scfs/include/uiox_kix_scfs_buf.h   — modernised
 *
 * UIOX buffer cache (Bach Ch.3).  Owns BLOCK_SIZE, includes inode.h for the
 * bmap() prototype, adds the modern i-node-aware entry points.
 *
 * @version 2.0.0  @date 2026-09-21
 */
#ifndef UIOX_KIX_SCFS_BUF_H
#define UIOX_KIX_SCFS_BUF_H

#include "uiox_klibc.h"
#include "uiox_kix_scfs_inode.h"

#define NBUF        30
/* BLOCK_SIZE — SINGLE OWNER.  buf.h only; inode.h no longer defines it. */
#define BLOCK_SIZE  512

/* Buffer flags (Bach Ch.3) */
#define B_BUSY  0x01    /* buffer is in use                     */
#define B_VALID 0x02    /* buffer has valid data                */
#define B_DIRTY 0x04    /* buffer has been written to           */
#define B_READ  0x08    /* read operation                       */
#define B_WRITE 0x10    /* write operation                      */

/* Special block numbers */
#define B_NOBLOCK   ((uint32_t)-1)   /* no such block — a hole */

typedef struct buf {
    uint16_t    b_flags;
    uint16_t    b_dev;
    uint32_t    b_blkno;
    int         b_error;
    int         b_resid;
    char        b_data[BLOCK_SIZE];
    struct buf *b_next;          /* hash chain / free-list forward      */
    struct buf *b_prev;          /* hash chain / free-list back         */
    uint32_t    b_refcnt;        /* modern: shared readers              */
} buf_t;

extern buf_t buf_pool[NBUF];

/* Buffer cache algorithms (Bach Ch.3) */
buf_t   *bread (uint16_t dev, uint32_t blkno);
buf_t   *breada(uint16_t dev, uint32_t blkno, uint32_t rablkno);
buf_t   *breadn(uint16_t dev, uint32_t blkno, uint32_t nblocks);
void     bwrite(buf_t *bp);
void     brelse(buf_t *bp);
buf_t   *getblk(uint16_t dev, uint32_t blkno);
void     bflush(uint16_t dev);          /* write back all dirty bufs/dev */
void     bsync(void);                   /* Bach update/pdflush           */

/* Allocation (Bach Ch.4 superblock free lists) */
uint32_t balloc (uint16_t dev);
void     bfree  (uint16_t dev, uint32_t blkno);

/* Block mapping (Bach Ch.4 bmap — direct + single/double/triple indirect). */
uint32_t bmap(inode_t *ip, uint32_t offset);

#endif /* UIOX_KIX_SCFS_BUF_H */
