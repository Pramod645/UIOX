/*
 * 30_KIX/32_FS/10_unfs/src/unfs_iget.c
 *
 * UNFS — the on-disk -> in-core i-node bridge.
 *
 * The ONE function set that reads an unfs_inode_disk_t (the bytes) and fills
 * an inode_t (what the rest of SCFS speaks).  Everything above — namei,
 * falloc, the file table, the syscalls — is Bach's algorithm and never sees
 * the on-disk layout.  Everything below — extents, group bitmaps, xattr —
 * is UNFS's private concern.
 *
 * The three conversions this file performs:
 *   1. 64-bit size   : i_size = unfs_mk64(d.i_size_lo, d.i_size_hi)
 *   2. 64-bit times  : i_atime = unfs_mk64(...), etc.
 *   3. extent model  : set IEXTENTS in i_flag; i_addr[] left zeroed.
 *
 * @version 2.0.0  @date 2026-09-21
 */
#include "unfs_disk.h"
#include "unfs_io.h"
#include "unfs_errno.h"
#include "uiox_kix_scfs_inode.h"
#include "uiox_kix_scfs_ops.h"

extern const uiox_inode_ops_t unfs_inode_ops;
extern const uiox_file_ops_t  unfs_file_ops;

static void mem_zero(void *p, uiox_uint32_t n)
{
    uiox_uint8_t *d = (uiox_uint8_t *)p;
    while (n--) *d++ = 0u;
}
static void mem_copy(void *d, const void *s, uiox_uint32_t n)
{
    uiox_uint8_t *dd = (uiox_uint8_t *)d;
    const uiox_uint8_t *ss = (const uiox_uint8_t *)s;
    while (n--) *dd++ = *ss++;
}

static inode_t *inode_alloc_slot(void)
{
    for (uiox_uint32_t i = 0u; i < NINODE; i++) {
        if (inode_table[i].i_count == 0u && inode_table[i].i_number == 0u)
            return &inode_table[i];
    }
    return (inode_t *)0;
}

int unfs_read_inode(uiox_uint32_t dev, uiox_uint32_t inum,
                    unfs_inode_disk_t *out)
{
    if (!out || inum == UNFS_NIL_INO) return UNFS_EINVAL;

    uiox_uint32_t byte_off = (inum - 1u) * UNFS_INODE_BYTES;
    uiox_uint32_t blk      = byte_off / UNFS_BLOCK_SIZE;
    uiox_uint32_t off      = byte_off % UNFS_BLOCK_SIZE;

    uiox_uint8_t blk0[UNFS_BLOCK_SIZE];
    if (unfs_bdev_read(dev, 0u, blk0) != UNFS_OK) return UNFS_EIO;

    unfs_sb_disk_t sb;
    mem_copy(&sb, blk0 + UNFS_SB_OFFSET, sizeof(sb));
    if (sb.s_magic != UNFS_MAGIC_V1 && sb.s_magic != UNFS_MAGIC_V2)
        return UNFS_EBADMAGIC;

    uiox_uint32_t table_blk = sb.s_inode_first_blk + blk;

    uiox_uint8_t ibuf[UNFS_BLOCK_SIZE];
    if (unfs_bdev_read(dev, table_blk, ibuf) != UNFS_OK) return UNFS_EIO;

    mem_copy(out, ibuf + off, sizeof(*out));
    return UNFS_OK;
}

int unfs_write_inode_disk(uiox_uint32_t dev, uiox_uint32_t inum,
                          const unfs_inode_disk_t *in)
{
    if (!in || inum == UNFS_NIL_INO) return UNFS_EINVAL;

    uiox_uint32_t byte_off = (inum - 1u) * UNFS_INODE_BYTES;
    uiox_uint32_t blk      = byte_off / UNFS_BLOCK_SIZE;
    uiox_uint32_t off      = byte_off % UNFS_BLOCK_SIZE;

    uiox_uint8_t blk0[UNFS_BLOCK_SIZE];
    if (unfs_bdev_read(dev, 0u, blk0) != UNFS_OK) return UNFS_EIO;
    unfs_sb_disk_t sb;
    mem_copy(&sb, blk0 + UNFS_SB_OFFSET, sizeof(sb));

    uiox_uint32_t table_blk = sb.s_inode_first_blk + blk;

    uiox_uint8_t ibuf[UNFS_BLOCK_SIZE];
    if (unfs_bdev_read(dev, table_blk, ibuf) != UNFS_OK)
        mem_zero(ibuf, UNFS_BLOCK_SIZE);

    mem_copy(ibuf + off, in, sizeof(*in));
    return unfs_bdev_write(dev, table_blk, ibuf);
}

static void unfs_inflate_inode(const unfs_inode_disk_t *d, inode_t *ip)
{
    mem_zero(ip, sizeof(*ip));

    ip->i_mode   = d->i_mode;
    ip->i_nlink  = d->i_nlink;
    ip->i_uid    = (uiox_uint16_t)d->i_uid;
    ip->i_gid    = (uiox_uint16_t)d->i_gid;

    /* conversion 1: 64-bit size from two halves */
    ip->i_size   = unfs_mk64(d->i_size_lo, d->i_size_hi);

    /* conversion 2: 64-bit timestamps from two halves each */
    ip->i_atime  = unfs_mk64(d->i_atime_lo, d->i_atime_hi);
    ip->i_mtime  = unfs_mk64(d->i_mtime_lo, d->i_mtime_hi);
    ip->i_ctime  = unfs_mk64(d->i_ctime_lo, d->i_ctime_hi);
    ip->i_btime  = unfs_mk64(d->i_btime_lo, d->i_btime_hi);

    /* conversion 3: extent model.  i_addr[] stays zero — UNFS never uses
     * Bach's block-address array; extents live on disk and are re-read by
     * the backend's read/truncate ops when needed. */
    ip->i_flag      |= IEXTENTS;
    ip->i_generation = d->i_generation;
    ip->i_seq        = d->i_seq;

    ip->i_pipe         = (struct uiox_pipe_buffer *)0;
    ip->i_pipe_readers = 0;
    ip->i_pipe_writers = 0;

    /* point the i-node at UNFS's op tables — this is what makes the
     * ops-table dispatch reach the UNFS backend. */
    ip->i_iop = &unfs_inode_ops;
    ip->i_fop = &unfs_file_ops;
}

inode_t *unfs_iget(uiox_uint16_t dev, uiox_uint32_t inum)
{
    if (inum == UNFS_NIL_INO) return (inode_t *)0;

    for (uiox_uint32_t i = 0u; i < NINODE; i++) {
        inode_t *ip = &inode_table[i];
        if (ip->i_dev == dev && ip->i_number == inum && ip->i_count > 0u) {
            ip->i_count++;
            return ip;
        }
    }

    unfs_inode_disk_t d;
    if (unfs_read_inode((uiox_uint32_t)dev, inum, &d) != UNFS_OK)
        return (inode_t *)0;

    inode_t *ip = inode_alloc_slot();
    if (!ip) return (inode_t *)0;

    unfs_inflate_inode(&d, ip);
    ip->i_dev    = dev;
    ip->i_number = inum;
    ip->i_count  = 1u;

    return ip;
}

void unfs_iput(inode_t *ip)
{
    if (!ip) return;
    if (ip->i_count > 0u) ip->i_count--;
    if (ip->i_count == 0u) {
        ip->i_number = 0u;
        ip->i_iop    = (const uiox_inode_ops_t *)0;
        ip->i_fop    = (const uiox_file_ops_t *)0;
    }
}
