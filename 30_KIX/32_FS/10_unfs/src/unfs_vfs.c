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
 *  bmap_alloc is 01_fsa's, and it currently stops short of placing a
 *  block: bmap.h says the overflow tree "is not written by anything yet",
 *  and unfs_cow_block's own comment says COW is inert until bmap_alloc
 *  exists.  The table names it anyway — the dispatch is complete, the
 *  implementation below it is not, and that is a fact about 01_fsa
 *  rather than about this table.
 *
 *  @version 1.0.0  @date 2026-10-03
 */
#include "unfs_fs.h"        /* unfs_fs_t, the mount state              */
#include "unfs_alloc.h"     /* unfs_alloc_block, unfs_free_run         */
#include "vfs.h"            /* the ops table                           */
#include "namei.h"          /* dir_lookup / dir_add / dir_remove       */
                            /* — 01_fsa's directory operations, whose  */
                            /*   addresses the ops table below takes   */

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

/* ── the table ────────────────────────────────────────────────────────*/
static const uiox_fs_ops_t unfs_fsops = {
    .name    = "unfs",

    /* mount is unfs_kern_mount(unfs_fs_t*, dev), which needs the handle
     * rather than a SuperBlock — so the SuperBlock form cannot be filled
     * until s_priv carries the unfs_fs_t.  Left NULL rather than cast. */
    .mount   = (int (*)(SuperBlock *, uint32_t))0,
    .unmount = (int (*)(SuperBlock *))0,
    .sync    = (int (*)(SuperBlock *))0,

    .iget    = unfs_iget,               /* 10_unfs — the byte→core bridge  */
    .iput    = iput,                    /* 01_fsa */
    .iupdate = iupdate,                 /* 01_fsa */
    .bmap    = bmap,                    /* 01_fsa */
    .bmap_alloc = bmap_alloc,           /* 01_fsa — implementation pending */

    .dir_lookup = dir_lookup,           /* 01_fsa */
    .dir_add    = dir_add,              /* 01_fsa */
    .dir_remove = dir_remove,           /* 01_fsa */

    .alloc_block = unfs_ops_alloc_block,
    .free_run    = unfs_ops_free_run,
    .free_inode_blocks = fs_free_inode_blocks,   /* 01_fsa */
};

int unfs_register(uint8_t dev)
{
    return vfs_register_fs(&unfs_fsops, dev);
}
