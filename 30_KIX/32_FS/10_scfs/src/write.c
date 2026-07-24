#include "../include/fs.h"
#include "../include/inode.h"
#include "../include/file.h"
#include "../include/buf.h"
#include <string.h>

/*
 * Algorithm write (similar to read)
 * input : user file descriptor, buffer, byte count
 * output: bytes written
 */
int fs_write(int fd, const char *ubuf, uint32_t count)
{
    if (fd < 0 || fd >= NOFILE) return FS_EBADF;

    file_t  *fp = u.u_ofile.ufd_file[fd];
    if (!fp || !(fp->f_flag & O_WRONLY)) return FS_EBADF;

    inode_t *ip = fp->f_inode;
    if (!ip) return FS_EBADF;

    ilock(ip);

    u.u_base   = (char *)ubuf;
    u.u_count  = count;
    u.u_offset = (fp->f_flag & O_APPEND) ? ip->i_size : fp->f_offset;

    uint32_t total_written = 0;

    while (u.u_count > 0) {
        uint32_t blk_offset = u.u_offset % BLOCK_SIZE;
        uint32_t space       = BLOCK_SIZE - blk_offset;
        uint32_t to_write    = u.u_count < space ? u.u_count : space;

        /* Get block number; allocate new block if needed */
        uint32_t blkno = bmap(ip, u.u_offset);
        if (blkno == 0) {
            /* Allocate new block (algorithm alloc) */
            blkno = balloc(ip->i_dev);
            if (!blkno) break;
            /* Assign to correct position in inode block table */
            uint32_t idx = u.u_offset / BLOCK_SIZE;
            if (idx < NBLOCK_DIRECT + NBLOCK_INDIRECT)
                ip->i_addr[idx] = blkno;
        }

        buf_t *bp = bread(ip->i_dev, blkno);
        if (!bp) break;

        memcpy(bp->b_data + blk_offset, u.u_base, to_write);
        bp->b_flags |= B_DIRTY;
        bwrite(bp);

        u.u_base        += to_write;
        u.u_offset      += to_write;
        u.u_count       -= to_write;
        total_written   += to_write;

        if (u.u_offset > ip->i_size)
            ip->i_size = u.u_offset;
    }

    ip->i_flag |= IUPD;
    iunlock(ip);
    fp->f_offset = u.u_offset;

    return (int)total_written;
}

/*
 * Algorithm lseek
 * input : file descriptor, offset, whence
 * output: new file offset
 */
int fs_lseek(int fd, int32_t offset, int whence)
{
    if (fd < 0 || fd >= NOFILE) return FS_EBADF;

    file_t *fp = u.u_ofile.ufd_file[fd];
    if (!fp) return FS_EBADF;

    int32_t new_offset;
    switch (whence) {
        case SEEK_SET: new_offset = offset;                       break;
        case SEEK_CUR: new_offset = (int32_t)fp->f_offset + offset; break;
        case SEEK_END: new_offset = (int32_t)fp->f_inode->i_size
                                    + offset;                     break;
        default: return FS_EINVAL;
    }

    if (new_offset < 0) return FS_EINVAL;

    fp->f_offset = (uint32_t)new_offset;
    return (int)fp->f_offset;
}

/*
 * Algorithm close
 * input : file descriptor
 * output: none (0 on success)
 */
int fs_close(int fd)
{
    if (fd < 0 || fd >= NOFILE) return FS_EBADF;

    file_t *fp = u.u_ofile.ufd_file[fd];
    if (!fp) return FS_EBADF;

    u.u_ofile.ufd_file[fd] = NULL;
    f_close(fp);
    return FS_OK;
}
