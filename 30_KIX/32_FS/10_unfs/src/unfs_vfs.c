/*
 *  30_KIX/32_FS/10_unfs/src/unfs_vfs.c
 *
 *  UNFS's VFS registration — the table that names 10_unfs's functions.
 *
 *  ── why this table is not the same shape as SCFS's ───────────────────
 *  UNFS is a different storage model: group BITMAPS with EXTENTS, not
 *  Bach's free list with indirect blocks.  Three entries differ in kind,
 *  not just in name:
 *
 *      alloc_block      unfs_alloc_block(dev)      — bitmap scan
 *      free_run         unfs_free_run(dev, first, count)
 *                                                  — a RUN, natively
 *      iget             unfs_iget(dev, inum)       — returns InCoreInode*
 *
 *  The other entries name 01_fsa's functions, because UNFS sits ON TOP of
 *  01_fsa rather than beside it: unfs_iget() bridges unfs_inode_t (the
 *  bytes) into InCoreInode (what bmap and namei speak), and everything
 *  above that seam is Bach's.
 *
 *  ── unfs_iget's signature ────────────────────────────────────────────
 *      InCoreInode *unfs_iget(uiox_uint8_t dev, uiox_uint32_t inum);
 *
 *  It takes a uint8_t dev, which is the table's shape exactly — no shim
 *  needed.  That was deliberate in unfs_iget.c: the byte reader has no
 *  dependency on unfs_fs_t, so it can be named here without dragging the
 *  mount state in.
 *
  *  ── what is still missing ────────────────────────────────────────────
 *  bmap_alloc(), not this table.  It is implemented (bmap.c, @version
 *  3.1.0): it allocates through unfs_alloc_run() and places the block into
 *  the inode's extent map, covering the four placement cases inline.  The
 *  ONE remaining limit is case 4 — no cover and the inline array is full —
 *  where the overflow tree block is NOT written.  That case reports
 *  valid = false and returns the allocated block in r.blkno for a reclaim
 *  pass, rather than claiming a mapping it cannot make (bmap.c:359–360).
 *
 *  unfs_cow_block() has the same limit from the other direction: it copies
 *  a block but the COW extent update is the same missing piece.
 *
 *  Nothing here is a fact about this table.  The dispatch is complete.
 *
 *  @version 1.0.0  @date 2026-10-03
 */
#include "unfs_fs.h"        /* unfs_fs_t, the mount state              */
#include "unfs_alloc.h"     /* unfs_alloc_block, unfs_free_run         */
#include "vfs.h"            /* the ops table                           */
#include "namei.h"          /* dir_lookup / dir_add / dir_remove       */
                            /* — 01_fsa's directory operations, whose  */
                            /*   addresses the ops table below takes   */
#include "superblock.h"     /* SuperBlock, sb_get — for the mount shims */

/* The bridge, from unfs_iget.c.  Declared in unfs_fs.h. */
extern InCoreInode *unfs_iget(uiox_uint8_t dev, uiox_uint32_t inum);

/* ── the shims ────────────────────────────────────────────────────────
 * Both are type-widening only: 10_unfs uses uiox_uint8_t where 01_fsa
 * uses uint8_t.  They are the same type under UIOX_BASETYPES_COMPAT, but
 * the table's signature is the one that has to match, so the casts are
 * made explicit rather than relied on. */
static uint32_t unfs_ops_alloc_block(uint8_t dev)
{
    return (uint32_t)unfs_alloc_block((uiox_uint32_t)dev);
}

static int unfs_ops_free_run(uint8_t dev, uint32_t first, uint32_t count)
{
    return (int)unfs_free_run((uiox_uint32_t)dev,
                              (uiox_uint32_t)first,
                              (uiox_uint32_t)count);
}
/* ── the mount trio ───────────────────────────────────────────────────
 * The table's signature is SuperBlock*, the kernel entry points take
 * unfs_fs_t* (unfs_fs.h:135–137).  s_priv is the bridge: unfs_register()
 * stashes the handle there, so these unwrap what was already stashed.
 *
 * NULL s_priv means the volume was never registered with a handle — a
 * caller error, not a failed mount.  Do NOT invent a handle here.
 *
 * This is NOT a cast of SuperBlock* to unfs_fs_t*: they are different
 * structs and the cast would compile, then read garbage. */
static unfs_fs_t *unfs_priv(SuperBlock *sb)
{
    return (sb != NULL) ? (unfs_fs_t *)sb->s_priv : NULL;
}

static int unfs_ops_mount(SuperBlock *sb, uint32_t dev)
{
    unfs_fs_t *fs = unfs_priv(sb);
    if (!fs) return UNFS_EINVAL;
    return unfs_kern_mount(fs, (uiox_uint32_t)dev);
}

static int unfs_ops_unmount(SuperBlock *sb)
{
    unfs_fs_t *fs = unfs_priv(sb);
    if (!fs) return UNFS_EINVAL;
    return unfs_kern_unmount(fs);
}

static int unfs_ops_sync(SuperBlock *sb)
{
    unfs_fs_t *fs = unfs_priv(sb);
    if (!fs) return UNFS_EINVAL;
    return unfs_kern_sync(fs);
}
/* ── the table ────────────────────────────────────────────────────────*/
static const uiox_fs_ops_t unfs_fsops = {
    .name    = "unfs",
    /* The SuperBlock form, unwrapped through s_priv — see the shims above.
     * NOT a cast: SuperBlock and unfs_fs_t are different structs and the
     * cast would compile and then read garbage. */
    .mount   = unfs_ops_mount,
    .unmount = unfs_ops_unmount,
    .sync    = unfs_ops_sync,


    .iget    = unfs_iget,               /* 10_unfs — the byte→core bridge  */
    .iput    = iput,                    /* 01_fsa */
    .iupdate = iupdate,                 /* 01_fsa */
    .bmap    = bmap,                    /* 01_fsa */
    .bmap_alloc = bmap_alloc,           /* 01_fsa — implemented (bmap.c 3.1.0) */

    .dir_lookup = dir_lookup,           /* 01_fsa */
    .dir_add    = dir_add,              /* 01_fsa */
    .dir_remove = dir_remove,           /* 01_fsa */

    .alloc_block = unfs_ops_alloc_block,
    .free_run    = unfs_ops_free_run,
    .free_inode_blocks = fs_free_inode_blocks,   /* 01_fsa */
};
/* Register UNFS for 'dev', stashing the mount handle where the ops table
 * can reach it.  The caller mounts first, then calls this — registration
 * publishes the handle, so the reverse order would leave s_priv pointing
 * at an unfilled struct.
 *
 * Called from uiox_fs_init.c after vfs_init() and before vfs_mount_root()
 * (vfs.md:111). */
int unfs_register(uint8_t dev, unfs_fs_t *fs)
{
    SuperBlock *sb;

    if (!fs) return UNFS_EINVAL;

    sb = sb_get(dev);
    if (!sb) return UNFS_EINVAL;

    sb->s_priv = fs;
    return vfs_register_fs(&unfs_fsops, dev);
}
