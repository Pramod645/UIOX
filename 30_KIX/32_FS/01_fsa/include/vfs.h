/*
 *  30_KIX/32_FS/01_fsa/include/vfs.h
 *
 *  The VFS — a dispatch table over the filesystem, not a second inode type.
 *
 *  ── what this is, and what it is NOT ─────────────────────────────────
 *  It IS:   one ops table per mounted volume, reached through s_sb.
 *  It is NOT: an inode-level vtable.  InCoreInode mirrors DiskInode
 *           field-for-field and DiskInode has a _Static_assert on 256
 *           bytes, so an ops pointer cannot go on the inode without
 *           breaking INODES_PER_BLOCK and every inode offset on disk.
 *
 *  ── where it attaches ────────────────────────────────────────────────
 *      SuperBlock.s_fsops   ← the table for this volume
 *      SuperBlock.s_priv    ← unfs_fs_t, when UNFS is the backend
 *
 *  Both are new fields on SuperBlock.  Nothing else changes shape: the
 *  Bach algorithms in 01_fsa (readi, bmap, namei, iget) are untouched and
 *  are what the table points AT.
 *
 *  ── why the ops are function pointers and not a switch ───────────────
 *  A switch on "which filesystem is this" needs editing every time a
 *  backend is added, and puts the SCFS/UNFS distinction into code that
 *  has no business knowing it.  A table is data: a backend registers by
 *  naming its own functions.
 *
 *  ── what is deliberately absent ──────────────────────────────────────
 *  No uiox_file_ops_t and no per-file dispatch.  10_scfs's file table is
 *  Bach's, and every entry there already reaches the right backend
 *  because the volume is known from the inode.  A per-file table would be
 *  a second place to look up the same fact.
 *
 *  @version 1.0.0  @date 2026-10-03
 */
#ifndef UIOX_VFS_H
#define UIOX_VFS_H

#include "fs_types.h"
#include "inode.h"
#include "bmap.h"
#include "superblock.h"

/* ═════════════════════════════════════════════════════════════════════
 * The filesystem operations table
 *
 * Every entry names a function that ALREADY EXISTS in 01_fsa.  The table
 * does not require a backend to be rewritten — only to be named.
 *
 * A NULL entry means the backend does not supply that operation.  The
 * dispatcher below tests before calling and returns VFS_ENOSYS, so a
 * partial backend is legal.
 * ═════════════════════════════════════════════════════════════════════ */
typedef struct uiox_fs_ops {

    const char *name;                    /* "scfs", "unfs" — for /proc */

    /* ── mount lifecycle ─────────────────────────────────────────── */
    int  (*mount)  (SuperBlock *sb, uint32_t dev);
    int  (*unmount)(SuperBlock *sb);
    int  (*sync)   (SuperBlock *sb);

    /* ── inode level — these are 01_fsa's, with dev threaded ─────── */
    InCoreInode *(*iget) (uint8_t dev, uint32_t ino);
    void         (*iput) (InCoreInode *ip);
    void         (*iupdate)(InCoreInode *ip);
    BmapResult   (*bmap) (InCoreInode *ip, uint32_t byte_off);
    BmapResult   (*bmap_alloc)(InCoreInode *ip, uint32_t byte_off);

    /* ── directory level — namei.h's shapes ──────────────────────── */
    uint32_t (*dir_lookup)(InCoreInode *dir, const char *name, uint32_t len);
    int      (*dir_add)   (InCoreInode *dir, const char *name, uint32_t len,
                           uint32_t ino, uint8_t type);
    int      (*dir_remove)(InCoreInode *dir, const char *name, uint32_t len);

    /* ── allocation — the two models differ, see the shims ───────── */
    uint32_t (*alloc_block)(uint8_t dev);
    int      (*free_run)   (uint8_t dev, uint32_t first, uint32_t count);
    void     (*free_inode_blocks)(InCoreInode *ip);
} uiox_fs_ops_t;

/* ═════════════════════════════════════════════════════════════════════
 * VFS entry points
 *
 * Each resolves the volume, then calls through the table.  A caller that
 * already holds the SuperBlock can use vfs_* on it directly; the dev
 * forms look it up.
 * ═════════════════════════════════════════════════════════════════════ */
#define VFS_OK        0
#define VFS_ENOSYS  -38
#define VFS_EINVAL  -22
#define VFS_ENODEV  -19
#define VFS_ENOENT   -2

/* Registration — a backend calls this from its own _register(). */
int  vfs_register_fs(const uiox_fs_ops_t *ops, uint8_t dev);
const uiox_fs_ops_t *vfs_fsops(uint8_t dev);

/* Init — clears the registry.  Called from uiox_fs_init(). */
void vfs_init(void);

/* ── the dispatchers ──────────────────────────────────────────────────
 * Each takes the DEVICE, resolves SuperBlock.s_fsops, and calls through.
 * A dev with no registered backend returns VFS_ENODEV.
 *
 * The bmap pair returns a BmapResult with .valid == false and .dev == 0
 * on a missing table, which every caller already tests — so a hole and a
 * missing filesystem are distinguishable by the dev field, not by a
 * separately-checked return code.
 * ─────────────────────────────────────────────────────────────────── */
InCoreInode *vfs_iget (uint8_t dev, uint32_t ino);
void         vfs_iput (InCoreInode *ip);
void         vfs_iupdate(InCoreInode *ip);
BmapResult   vfs_bmap (InCoreInode *ip, uint32_t byte_off);
BmapResult   vfs_bmap_alloc(InCoreInode *ip, uint32_t byte_off);

uint32_t vfs_dir_lookup(InCoreInode *dir, const char *name, uint32_t len);
int      vfs_dir_add   (InCoreInode *dir, const char *name, uint32_t len,
                        uint32_t ino, uint8_t type);
int      vfs_dir_remove(InCoreInode *dir, const char *name, uint32_t len);

uint32_t vfs_alloc_block(uint8_t dev);
int      vfs_free_run   (uint8_t dev, uint32_t first, uint32_t count);
void     vfs_free_inode_blocks(InCoreInode *ip);

/* ── mount ────────────────────────────────────────────────────────────
 * vfs_mount_root mounts the device carrying the root filesystem.
 *
 * NOTE: this returns VFS_ENOSYS today for both backends, and that is the
 * honest answer rather than a stub returning 0.  Mounting needs a device
 * number, and an InCoreInode carries none — which is the same gap that
 * makes mknod refuse SCFS_S_IFBLK.  The dispatch below is real; the
 * device is what is missing.
 * ─────────────────────────────────────────────────────────────────── */
int vfs_mount_root(uint8_t dev);
int vfs_mount(const char *path, const char *fstype, uint32_t dev);
int vfs_umount(uint8_t dev);
int vfs_sync(uint8_t dev);

#endif /* UIOX_VFS_H */
