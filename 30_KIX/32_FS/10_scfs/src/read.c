/*
 *  30_KIX/32_FS/10_scfs/src/read.c  — freestanding fix v1.1
 *    FIXED: ../../33_PCS path, fprintf(stderr,...)
 */
#include "../include/fs.h"
#include "../include/inode.h"
#include "../include/file.h"
#include "../include/buf.h"
#include "uiox_klibc.h"

/*
 * Algorithm read
 * input : user file descriptor, buffer, byte count
 * output: bytes read
 */
int fs_read(int fd, char *buf, uint32_t count)
{
    file_t  *fp;
    inode_t *ip;
    uint32_t done = 0;

    if (fd < 0 || fd >= NOFILE) return FS_EBADF;
    fp = u.u_ofile.ufd_file[fd];
    if (!fp) return FS_EBADF;
    if (!(fp->f_flag & O_RDONLY) && fp->f_flag != 0) return FS_EBADF;

    ip = fp->f_inode;
    if (!ip) return FS_EBADF;

    /* Pipe read */
    if ((ip->i_mode & IFMT) == IFIFO) {
        /* sim: return 0 (EOF) */
        return 0;
    }

    while (done < count && fp->f_offset < ip->i_size) {
        uint32_t blkno  = fp->f_offset / BLOCK_SIZE;
        uint32_t blkoff = fp->f_offset % BLOCK_SIZE;
        uint32_t avail  = BLOCK_SIZE - blkoff;
        uint32_t want   = count - done;
        uint32_t n      = (avail < want) ? avail : want;
        uint32_t left   = ip->i_size - fp->f_offset;
        if (n > left) n = left;

        if (blkno < (uint32_t)(NBLOCK_DIRECT + NBLOCK_INDIRECT) &&
            ip->i_addr[blkno]) {
            buf_t *bp = bread((uint16_t)ip->i_dev, ip->i_addr[blkno]);
            if (bp) {
                memcpy(buf + done, bp->b_data + blkoff, n);
                brelse(bp);
            } else {
                break;
            }
        } else {
            memset(buf + done, 0, n);
        }
        done         += n;
        fp->f_offset += n;
    }

    ip->i_flag |= IACC;
    return (int)done;
}

int fs_lseek(int fd, int32_t offset, int whence)
{
    file_t  *fp;
    int32_t  new_off;

    if (fd < 0 || fd >= NOFILE) return FS_EBADF;
    fp = u.u_ofile.ufd_file[fd];
    if (!fp) return FS_EBADF;

    switch (whence) {
        case 0: new_off = offset; break;                                  /* SEEK_SET */
        case 1: new_off = (int32_t)fp->f_offset + offset; break;         /* SEEK_CUR */
        case 2: new_off = (int32_t)fp->f_inode->i_size + offset; break;  /* SEEK_END */
        default: return FS_EBADF;
    }
    if (new_off < 0) return FS_EBADF;
    fp->f_offset = (uint32_t)new_off;
    return (int)fp->f_offset;
}
