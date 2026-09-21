/*
 *  30_KIX/32_FS/10_scfs/src/write.c  — freestanding fix v1.2
 *    FIXED: balloc() returns uint32_t blkno, not buf_t*
 *           Use balloc() for block number, then getblk() for buffer.
 */
#include "../include/fs.h"
#include "../include/inode.h"
#include "../include/file.h"
#include "../include/buf.h"
#include "uiox_klibc.h"

/*
 * Algorithm write
 * input : user file descriptor, buffer, byte count
 * output: bytes written
 */
int fs_write(int fd, const char *buf, uint32_t count)
{
    file_t   *fp;
    inode_t  *ip;
    uint32_t  done = 0;

    if (fd < 0 || fd >= NOFILE) return FS_EBADF;
    fp = u.u_ofile.ufd_file[fd];
    if (!fp) return FS_EBADF;
    if (!(fp->f_flag & O_WRONLY)) return FS_EBADF;

    ip = fp->f_inode;
    if (!ip) return FS_EBADF;

    /* Pipe write — discard in sim */
    if ((ip->i_mode & IFMT) == IFIFO)
        return (int)count;

    if (fp->f_flag & O_APPEND)
        fp->f_offset = ip->i_size;

    while (done < count) {
        uint32_t blk_idx = fp->f_offset / BLOCK_SIZE;
        uint32_t blkoff  = fp->f_offset % BLOCK_SIZE;
        uint32_t avail   = BLOCK_SIZE - blkoff;
        uint32_t n       = (avail < (count - done)) ? avail : (count - done);
        uint32_t blkno;
        buf_t   *bp;

        if (blk_idx >= (uint32_t)(NBLOCK_DIRECT + NBLOCK_INDIRECT)) break;

        if (!ip->i_addr[blk_idx]) {
            /* Allocate a new block — balloc returns block number */
            blkno = balloc((uint16_t)ip->i_dev);
            if (!blkno) break;
            ip->i_addr[blk_idx] = blkno;
            ip->i_flag |= IUPD;
            bp = getblk((uint16_t)ip->i_dev, blkno);
        } else {
            blkno = ip->i_addr[blk_idx];
            bp = bread((uint16_t)ip->i_dev, blkno);
        }
        if (!bp) break;

        memcpy(bp->b_data + blkoff, buf + done, n);
        bp->b_flags |= B_DIRTY;
        bwrite(bp);
        brelse(bp);

        done         += n;
        fp->f_offset += n;
        if (fp->f_offset > ip->i_size)
            ip->i_size = fp->f_offset;
    }

    ip->i_flag |= IUPD;
    return (int)done;
}
