#include "../include/fs.h"
#include "../include/inode.h"
#include "../include/file.h"
#include "../include/buf.h"
#include <string.h>

/*
 * Algorithm read
 * input : user file descriptor, buffer address, byte count
 * output: bytes copied into user space
 */
int fs_read(int fd, char *ubuf, uint32_t count)
{
    if (fd < 0 || fd >= NOFILE) return FS_EBADF;

    file_t  *fp = u.u_ofile.ufd_file[fd];
    if (!fp || !(fp->f_flag & O_RDONLY)) return FS_EBADF;

    inode_t *ip = fp->f_inode;
    if (!ip) return FS_EBADF;

    ilock(ip);

    /* Set u area I/O parameters */
    u.u_base   = ubuf;
    u.u_count  = count;
    u.u_offset = fp->f_offset;

    uint32_t total_read = 0;

    while (u.u_count > 0) {
        /* Convert file offset to disk block (algorithm bmap) */
        uint32_t blkno = bmap(ip, u.u_offset);

        uint32_t blk_offset = u.u_offset % BLOCK_SIZE;
        uint32_t bytes_avail = BLOCK_SIZE - blk_offset;

        /* End of file? */
        if (u.u_offset >= ip->i_size) break;

        uint32_t to_read = u.u_count < bytes_avail
                         ? u.u_count : bytes_avail;
        if (u.u_offset + to_read > ip->i_size)
            to_read = ip->i_size - u.u_offset;
        if (to_read == 0) break;

        /* Read block (breada if read-ahead available) */
        buf_t *bp = bread(ip->i_dev, blkno);
        if (!bp) break;

        /* Copy data from system buffer to user address */
        memcpy(u.u_base, bp->b_data + blk_offset, to_read);

        /* Update u area fields */
        u.u_base   += to_read;
        u.u_offset += to_read;
        u.u_count  -= to_read;
        total_read += to_read;

        brelse(bp);     /* release buffer locked in bread */
    }

    iunlock(ip);

    /* Update file table offset for next read */
    fp->f_offset = u.u_offset;

    ip->i_flag |= IACC;
    return (int)total_read;
}
