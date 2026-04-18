#ifndef BUF_H
#define BUF_H

#include <stdint.h>

#define NBUF        30
#define BLOCK_SIZE  512

/* Buffer flags */
#define B_BUSY  0x01    /* buffer is in use */
#define B_VALID 0x02    /* buffer has valid data */
#define B_DIRTY 0x04    /* buffer has been written to */
#define B_READ  0x08    /* read operation */
#define B_WRITE 0x10    /* write operation */

typedef struct buf {
    uint16_t  b_flags;
    uint16_t  b_dev;
    uint32_t  b_blkno;
    int       b_error;
    int       b_resid;
    char      b_data[BLOCK_SIZE];
    struct buf *b_next;
    struct buf *b_prev;
} buf_t;

extern buf_t buf_pool[NBUF];

/* Buffer cache algorithms */
buf_t *bread(uint16_t dev, uint32_t blkno);
buf_t *breada(uint16_t dev, uint32_t blkno, uint32_t rablkno);
void   bwrite(buf_t *bp);
void   brelse(buf_t *bp);
buf_t *getblk(uint16_t dev, uint32_t blkno);
uint32_t balloc(uint16_t dev);
void     bfree(uint16_t dev, uint32_t blkno);
uint32_t bmap(inode_t *ip, uint32_t offset);

#endif /* BUF_H */
