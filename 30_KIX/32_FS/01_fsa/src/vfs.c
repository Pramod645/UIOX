/*
 *  30_KIX/32_FS/01_fsa/src/vfs.c
 *
 *  The VFS dispatch — one table per device, resolved by dev.
 *
 *  ── the storage ──────────────────────────────────────────────────────
 *  A flat array indexed by device number, sized MAX_DEVICES.  It is
 *  parallel to superblock.c's s_sb[MAX_DEVICES] and to 10_scfs's
 *  scfs_mount_table[NMOUNT] — three views of the same fact, which is why
 *  they are all indexed by dev rather than kept in one struct.
 *
 *  Aligning with superblock.c rather than inventing a fourth index space
 *  is deliberate: a future MAX_DEVICES change is one edit in fs_types.h,
 *  not three.
 *
 *  ── what a NULL table means ──────────────────────────────────────────
 *  A device with no registered filesystem.  Every dispatcher returns the
 *  error its own signature can carry — NULL for a pointer, a `.valid =
 *  false` BmapResult, VFS_ENODEV for an int — rather than aborting or
 *  substituting a default backend.  A caller that asked for dev 3 and
 *  silently got dev 0's filesystem would be reading another volume's
 *  inodes.
 *
 *  @version 1.0.0  @date 2026-10-03
 */
#include "vfs.h"

/* One table per device.  Not static-extern: this file owns it. */
static const uiox_fs_ops_t *s_fsops[MAX_DEVICES];

/* ── registration ─────────────────────────────────────────────────── */
int vfs_register_fs(const uiox_fs_ops_t *ops, uint8_t dev)
{
    if (!ops)             return VFS_EINVAL;
    if (dev >= MAX_DEVICES) return VFS_EINVAL;

    /* A second registration on the same dev is refused rather than
     * silently replacing: whichever backend ran second would win, and
     * the loser's mounts would resolve to the winner's functions. */
    if (s_fsops[dev]) return VFS_EINVAL;

    s_fsops[dev] = ops;
    return VFS_OK;
}

const uiox_fs_ops_t *vfs_fsops(uint8_t dev)
{
    if (dev >= MAX_DEVICES) return (const uiox_fs_ops_t *)0;
    return s_fsops[dev];
}

void vfs_init(void)
{
    uint8_t d;
    for (d = 0u; d < MAX_DEVICES; d++) s_fsops[d] = (const uiox_fs_ops_t *)0;
}

/* ═════════════════════════════════════════════════════════════════════
 * Inode level
 * ═════════════════════════════════════════════════════════════════════ */

InCoreInode *vfs_iget(uint8_t dev, uint32_t ino)
{
    const uiox_fs_ops_t *ops = vfs_fsops(dev);
    if (!ops || !ops->iget) return (InCoreInode *)0;
    return ops->iget(dev, ino);
}

void vfs_iput(InCoreInode *ip)
{
    const uiox_fs_ops_t *ops;

    if (!ip) return;
    ops = vfs_fsops(ip->dev);
    if (!ops || !ops->iput) return;

    ops->iput(ip);
}

void vfs_iupdate(InCoreInode *ip)
{
    const uiox_fs_ops_t *ops;

    if (!ip) return;
    ops = vfs_fsops(ip->dev);
    if (!ops || !ops->iupdate) return;

    ops->iupdate(ip);
}

/* ── a failed lookup returns a BmapResult with valid == false ────────
 * Every existing caller already tests `.valid`, so a missing table needs
 * no new error path — but the dev field is left 0 so a caller that wants
 * to tell "no filesystem" from "a hole" can: a hole keeps ip->dev. */
static BmapResult bmap_fail(void)
{
    BmapResult r;
    r.dev = 0u; r.blkno = 0u; r.blk_offset = 0u;
    r.io_bytes = 0u; r.readahead_blk = 0u; r.valid = false;
    return r;
}

BmapResult vfs_bmap(InCoreInode *ip, uint32_t byte_off)
{
    const uiox_fs_ops_t *ops;

    if (!ip) return bmap_fail();
    ops = vfs_fsops(ip->dev);
    if (!ops || !ops->bmap) return bmap_fail();

    return ops->bmap(ip, byte_off);
}

BmapResult vfs_bmap_alloc(InCoreInode *ip, uint32_t byte_off)
{
    const uiox_fs_ops_t *ops;

    if (!ip) return bmap_fail();
    ops = vfs_fsops(ip->dev);
    if (!ops || !ops->bmap_alloc) return bmap_fail();

    return ops->bmap_alloc(ip, byte_off);
}

/* ═════════════════════════════════════════════════════════════════════
 * Directory level
 *
 * dev comes from the DIRECTORY inode, not from a parameter — a directory
 * and its entries are on the same volume by construction.
 * ═════════════════════════════════════════════════════════════════════ */

uint32_t vfs_dir_lookup(InCoreInode *dir, const char *name, uint32_t len)
{
    const uiox_fs_ops_t *ops;

    if (!dir || !name) return 0u;
    ops = vfs_fsops(dir->dev);
    if (!ops || !ops->dir_lookup) return 0u;

    return ops->dir_lookup(dir, name, len);
}

int vfs_dir_add(InCoreInode *dir, const char *name, uint32_t len,
                uint32_t ino, uint8_t type)
{
    const uiox_fs_ops_t *ops;

    if (!dir || !name) return VFS_EINVAL;
    ops = vfs_fsops(dir->dev);
    if (!ops || !ops->dir_add) return VFS_ENOSYS;

    return ops->dir_add(dir, name, len, ino, type);
}

int vfs_dir_remove(InCoreInode *dir, const char *name, uint32_t len)
{
    const uiox_fs_ops_t *ops;

    if (!dir || !name) return VFS_EINVAL;
    ops = vfs_fsops(dir->dev);
    if (!ops || !ops->dir_remove) return VFS_ENOSYS;

    return ops->dir_remove(dir, name, len);
}

/* ═════════════════════════════════════════════════════════════════════
 * Allocation
 * ═════════════════════════════════════════════════════════════════════ */

uint32_t vfs_alloc_block(uint8_t dev)
{
    const uiox_fs_ops_t *ops = vfs_fsops(dev);
    if (!ops || !ops->alloc_block) return 0u;   /* 0 is never a block */
    return ops->alloc_block(dev);
}

int vfs_free_run(uint8_t dev, uint32_t first, uint32_t count)
{
    const uiox_fs_ops_t *ops = vfs_fsops(dev);
    if (!ops || !ops->free_run) return VFS_ENOSYS;
    return ops->free_run(dev, first, count);
}

void vfs_free_inode_blocks(InCoreInode *ip)
{
    const uiox_fs_ops_t *ops;

    if (!ip) return;
    ops = vfs_fsops(ip->dev);
    if (!ops || !ops->free_inode_blocks) return;

    ops->free_inode_blocks(ip);
}

/* ═════════════════════════════════════════════════════════════════════
 * Mount
 * ═════════════════════════════════════════════════════════════════════ */

//int vfs_mount(uint8_t dev, const char *fstype)
int vfs_mount(const char *path, const char *fstype, uint32_t dev)
{
     /* path   — the mount point, resolved to a dentry and attached
      * fstype — selects the backend ops table by name
      * dev    — the device number the backend reads from */
    (void)path;   /* mount-point attach not yet implemented */
    const uiox_fs_ops_t *ops = vfs_fsops(dev);
    (void)fstype;

    if (!ops)           return VFS_ENODEV;
    if (!ops->mount)    return VFS_ENOSYS;

    return ops->mount(sb_get(dev), (uint32_t)dev);
}

int vfs_mount_root(uint8_t dev)
{
    const uiox_fs_ops_t *ops = vfs_fsops(dev);

    if (!ops)        return VFS_ENODEV;
    if (!ops->mount) return VFS_ENOSYS;

    /* The superblock for this device, and the backend's mount hook.  The
     * hook returns ENOSYS today for both backends — see the header note:
     * mounting needs a device number, and an InCoreInode carries none.
     * This function exists so the path through it is complete and the
     * remaining gap is one level down, where it can be seen. */
    return ops->mount(sb_get(dev), (uint32_t)dev);
}

int vfs_umount(uint8_t dev)
{
    const uiox_fs_ops_t *ops = vfs_fsops(dev);
    if (!ops || !ops->unmount) return VFS_ENOSYS;
    return ops->unmount(sb_get(dev));
}

int vfs_sync(uint8_t dev)
{
    const uiox_fs_ops_t *ops = vfs_fsops(dev);
    if (!ops || !ops->sync) return VFS_ENOSYS;
    return ops->sync(sb_get(dev));
}
