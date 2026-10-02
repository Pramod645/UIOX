/*
 *  30_KIX/32_FS/10_scfs/src/uiox_kix_scfs_vfs.c
 *
 *  SCFS's VFS registration — the table that names 01_fsa's functions.
 *
 *  ── what this file does NOT do ───────────────────────────────────────
 *  It defines no algorithm.  Every entry below names a function that
 *  already exists, either in 01_fsa or in 10_scfs.  The file's whole job
 *  is to put those names where a dispatcher can find them, so the
 *  syscall layer stops having to know which filesystem it is talking to.
 *
 *  ── the allocator shim ───────────────────────────────────────────────
 *  SCFS allocates through Bach's free LIST: fs_alloc(dev) returns a
 *  locked, zeroed BufHdr for a block it has detached from the free list.
 *  The table's alloc_block returns a BLOCK NUMBER, because that is what
 *  bmap_alloc needs to place into an extent.
 *
 *  scfs_ops_alloc_block() below is that conversion: take the buffer, read
 *  the block number off it, brelse it, return the number.  It is the one
 *  genuinely new function in this file, and it exists because the two
 *  allocator models cannot be merged — a free list cannot answer "give me
 *  five contiguous blocks", which is what an extent allocator asks.
 *
 *  ── what the table cannot supply ─────────────────────────────────────
 *  mount and unmount are NULL.  uiox_kix_scfs_mount() returns SCFS_ENOSYS
 *  because a block special file cannot be created (mknod refuses
 *  SCFS_S_IFBLK — no i_major/i_minor in the inode), so there is no device
 *  path to mount through.  A NULL entry makes that visible at the table
 *  rather than at a call that reports it later.
 *
 *  @version 1.0.0  @date 2026-10-03
 */
#include "uiox_kix_scfs_internal.h"
#include "vfs.h"

/* ── the one new function: BufHdr * → block number ─────────────────── */
static uint32_t scfs_ops_alloc_block(uint8_t dev)
{
    BufHdr *b = fs_alloc(dev);          /* 01_fsa — locked, zeroed */
    uint32_t blk;

    if (!b) return 0u;                  /* 0 is never a data block */

    blk = b->blkno;
    brelse(b);                          /* fs_alloc's contract */

    return blk;
}

/* ── free_run over Bach's free list ───────────────────────────────────
 * fs_free() takes ONE block; the table's signature frees a run.  A loop
 * is the honest translation — the free list has no run form, and
 * pretending otherwise would mean a second allocator. */
static int scfs_ops_free_run(uint8_t dev, uint32_t first, uint32_t count)
{
    uint32_t i;

    if (count == 0u) return VFS_EINVAL;

    for (i = 0u; i < count; i++)
        fs_free(dev, first + i);        /* 01_fsa */

    return VFS_OK;
}

/* ── the table ────────────────────────────────────────────────────────
 * Every entry names an existing function.  The three marked 01_fsa are
 * Bach's algorithms, called directly — which is what the internal header
 * means by "SCFS calls 01_fsa directly".  The vtable does not change
 * that; it only lets something ELSE make the same call. */
static const uiox_fs_ops_t scfs_fsops = {
    .name    = "scfs",

    /* no device path yet — see the header note */
    .mount   = (int (*)(SuperBlock *, uint32_t))0,
    .unmount = (int (*)(SuperBlock *))0,
    .sync    = (int (*)(SuperBlock *))0,

    /* ── inode level: 01_fsa, verbatim ────────────────────────────── */
    .iget    = iget_dev,                /* 01_fsa */
    .iput    = iput,                    /* 01_fsa */
    .iupdate = iupdate,                 /* 01_fsa */
    .bmap    = bmap,                    /* 01_fsa */
    .bmap_alloc = bmap_alloc,           /* 01_fsa */

    /* ── directory level: namei.c, verbatim ───────────────────────── */
    .dir_lookup = dir_lookup,           /* 01_fsa */
    .dir_add    = dir_add,              /* 01_fsa */
    .dir_remove = dir_remove,           /* 01_fsa */

    /* ── allocation: the shims above ──────────────────────────────── */
    .alloc_block = scfs_ops_alloc_block,
    .free_run    = scfs_ops_free_run,
    .free_inode_blocks = fs_free_inode_blocks,   /* 01_fsa */
};

/* ── registration ─────────────────────────────────────────────────────
 * Called from uiox_fs_init() after vfs_init().  ROOT_DEV is 0 — see
 * inode.h — so the root filesystem registers on device 0.
 */
int uiox_kix_scfs_register(uint8_t dev)
{
    return vfs_register_fs(&scfs_fsops, dev);
}
