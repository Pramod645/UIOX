/*
 * 30_KIX/32_FS/10_unfs/include/unfs_fs.h
 *
 * UIOX Native Filesystem (UNFS) — kernel-side UNFS integration header.
 *
 * This is a fork of 32_FS/include/unfs_fs.h.  Same surface, plus the
 * snapshot / copy-on-write declarations, so 10_unfs has ONE header to
 * include rather than two.
 *
 * -- CHANGED IN v3.0.0: PROTOTYPES ALIGNED TO THE IMPLEMENTATIONS -----
 *
 * Every declaration below now matches the function that defines it.  An
 * earlier revision declared the VFS-era shapes while the .c files had
 * moved on, which produced three "conflicting types" errors per build:
 *
 *   unfs_alloc_block   declared (unfs_fs_t *)      defined (uint32_t dev)
 *   unfs_read_inode    declared (unfs_fs_t *,
 *                                uint32_t,
 *                                unfs_inode_priv_t *)  defined
 *                      (uint32_t dev, uint32_t ino, unfs_inode_t *)
 *   unfs_snap_delete   declared uint64_t           defined uiox_uint64_t
 *
 * The device-parameter forms are correct, not the struct ones:
 * unfs_alloc.c works on (dev, want) and deliberately depends on neither
 * unfs_fs_t nor 01_fsa, so it can sit beside Bach's free-list allocator
 * without sharing state.  unfs_read_inode is the BYTE reader — it fills a
 * unfs_inode_t, the on-disk image.  unfs_inode_priv_t is a different
 * thing (that image plus xattrs and a dirty flag) and the reader must not
 * require it.
 *
 * -- the uiox_ spellings ---------------------------------------------
 * Types come from unfs_format.h, which defines uiox_uint32_t and friends.
 * The bare uint64_t only exists if uiox_klibc.h is reached through the
 * include chain, and it is a DIFFERENT TYPE:
 *
 *     uiox_uint64_t = unsigned long        (uiox_base_types.h)
 *     uint64_t      = unsigned long long   (uiox_klibc.h)
 *
 * Distinct types in C, so mixing them is a hard error under -Werror.
 * This header uses the uiox_ spellings throughout, which is what
 * unfs_format.h supplies without needing klibc.
 *
 * -- what was removed from the ORIGINAL, and why ----------------------
 *
 *   unfs_disk.h   REMOVED — it disagreed with the bootloader on three
 *                 layout facts (512-byte inodes, superblock at byte 1024,
 *                 versioned magic).  unfs_format.h is the survivor.
 *
 *   uiox_vfs.h    REMOVED — it existed only under
 *                 33_PCS/dataFlowFilesForBackup/FS/, which no Makefile
 *                 puts on -I.  With it go uiox_superblock_t,
 *                 uiox_file_ops_t, uiox_inode_ops_t and uiox_fs_ops_t.
 *
 * 01_fsa has no ops tables and no registry.  It calls iget_dev(), bmap()
 * and dir_add() by name, and its superblock is a SuperBlock.
 *
 * @version 3.0.0  @date 2026-09-27
 */
#ifndef UNFS_FS_H
#define UNFS_FS_H

#include "unfs_format.h"      /* the on-disk format — the survivor  */
#include "superblock.h"       /* SuperBlock, 01_fsa's own type      */

/* ── Kernel-only limits ─────────────────────────────────────────────── */
#define UNFS_MAX_GROUPS       256u   /* max block groups per volume      */
#define UNFS_XATTR_MAX        16u    /* max extended attributes per inode*/
#define UNFS_XATTR_VAL_MAX    256u   /* max xattr value size             */
#define UNFS_SNAP_MAX         8u     /* max snapshots per volume         */
#define UNFS_SNAP_NAME_MAX    32u    /* snapshot name incl. NUL          */

/* ── Extended attribute entry ───────────────────────────────────────── */
typedef struct {
    char     name[64];
    uint8_t  value[UNFS_XATTR_VAL_MAX];
    uint16_t val_len;
    uint8_t  inuse;
} unfs_xattr_t;

/* ── In-memory snapshot descriptor ──────────────────────────────────────
 * Records WHICH inode tree the snapshot roots at, not a copy of the tree.
 * COW keeps the old blocks alive, so a snapshot is read by walking
 * extents as they stood when it was taken. */
typedef struct {
    uiox_uint64_t  snap_id;            /* 1-based, never reused         */
    uiox_uint64_t  created_ns;         /* 0 — no clock in this layer    */
    uiox_uint32_t  root_ino;           /* UNFS_ROOT_INO at snapshot time*/
    char           name[UNFS_SNAP_NAME_MAX];
    uiox_uint8_t   inuse;
} unfs_snap_t;

/* ── In-memory UNFS superblock (kernel) ─────────────────────────────── */
typedef struct {
    unfs_sb_t          disk;                       /* on-disk superblock */
    unfs_group_desc_t  groups[UNFS_MAX_GROUPS];    /* group descriptors  */
    uiox_uint32_t      n_groups;
    SuperBlock        *sb;        /* 01_fsa's superblock, not the VFS's */

    /* ── snapshot / COW state ─────────────────────────────────────────
     * cow_active lives HERE, in memory, and is NOT written back to the
     * on-disk superblock: unfs_sb_t has no such field, and COW state is
     * private to a mount.  A fresh mount correctly starts with it off. */
    unfs_snap_t        snapshots[UNFS_SNAP_MAX];
    uiox_uint8_t       n_snapshots;
    uiox_uint8_t       cow_active;

    /* The device these blocks come from.  unfs_cow_block() reads through
     * the block cache and needs to name the device. */
    uiox_uint32_t      dev_id;

    uiox_uint8_t       mounted;
} unfs_fs_t;

/* ── In-memory inode private data ───────────────────────────────────── */
typedef struct {
    unfs_inode_t  disk;                  /* on-disk inode copy          */
    uiox_uint32_t ino;                   /* inode number                */
    unfs_fs_t    *fs;                    /* owning filesystem           */
    unfs_xattr_t  xattrs[UNFS_XATTR_MAX];
    uiox_uint8_t  n_xattrs;
    uiox_uint8_t  dirty;                 /* inode needs writeback       */
} unfs_inode_priv_t;

/* ═════════════════════════════════════════════════════════════════════
 * Public kernel API
 *
 * Registration is gone: there is no registry.  These are called directly,
 * the way 01_fsa's own entry points are.
 * ═════════════════════════════════════════════════════════════════════ */

/* ── Mount / unmount / sync ───────────────────────────────────────────
 * unfs_mount.c implements these.  They take the handle, not a SuperBlock:
 * the group descriptors have no home on SuperBlock, which is Bach's
 * free-list model, so the mount state lives in unfs_fs_t. */
int   unfs_kern_mount  (unfs_fs_t *fs, uiox_uint32_t dev);
int   unfs_kern_unmount(unfs_fs_t *fs);
int   unfs_kern_sync   (unfs_fs_t *fs);

/* ── Block allocation — bitmap-based, in unfs_alloc.c ─────────────────
 * Takes a DEVICE.  unfs_alloc.c works on (dev, want) and depends on
 * neither unfs_fs_t nor 01_fsa, so it sits beside Bach's free-list
 * allocator (fs_alloc_begin / fs_free in superblock.h) without the two
 * sharing state.  A bitmap can answer "give me five contiguous blocks";
 * a free list cannot, which is what an extent allocator needs.
 * cannot, which is what an extent allocator needs.
 *
 * The declarations live in unfs_alloc.h: unfs_alloc.c implements them, and
 * this header does not duplicate them.  Include unfs_alloc.h for
 * unfs_alloc_run, unfs_alloc_block, unfs_free_run, unfs_group_free_blocks,
 * unfs_alloc_inode_bit and unfs_free_inode_bit. */
/* ── Inode read / write — the BYTE level ──────────────────────────────
 * unfs_iget.c implements these.  unfs_read_inode fills a unfs_inode_t,
 * the on-disk image, at the group-0-absolute inode table base
 * (UNFS_GROUP0_ITABLE).  It is NOT the same as unfs_inode_priv_t, which
 * carries xattrs and a dirty flag; requiring that here would make the
 * byte reader depend on kernel-only state. */
int  unfs_read_inode      (uiox_uint32_t dev, uiox_uint32_t ino,
                           unfs_inode_t *out);
int  unfs_write_inode_disk(uiox_uint32_t dev, uiox_uint32_t ino,
                           const unfs_inode_t *in);

/* ── In-core inode bridge — unfs_iget.c ───────────────────────────────
 * InCoreInode is inode.h's cache entry.  unfs_iget() returns NULL for
 * both "no such inode" and "too large to represent in core" — the second
 * happens when the on-disk uint64_t i_size exceeds InCoreInode's
 * uint32_t size.  unfs_inode_size_ok() tells the two apart for a caller
 * that must report the difference. */
InCoreInode *unfs_iget         (uiox_uint8_t dev, uiox_uint32_t inum);
void         unfs_iput         (InCoreInode *ip);
int          unfs_inode_size_ok(uiox_uint8_t dev, uiox_uint32_t inum);

/* Keep the private-data wrappers, which are a different level: they take
 * the kernel struct and add xattr and dirty tracking on top. */
int  unfs_read_inode_priv (unfs_fs_t *fs, uiox_uint32_t ino,
                           unfs_inode_priv_t *out);
int  unfs_write_inode_priv(unfs_fs_t *fs, const unfs_inode_priv_t *priv);

/* ── Extent management — the piece 01_fsa's truncate.c trims but never
 *    adds.  NOT IMPLEMENTED YET: bmap_alloc() reports ENOSPC rather than
 *    placing a block, so a write path stops short of writing. */
int  unfs_extent_add (unfs_fs_t *fs, unfs_inode_priv_t *priv,
                      uiox_uint32_t logical, uiox_uint32_t physical,
                      uiox_uint16_t len);
int  unfs_extent_truncate(unfs_fs_t *fs, unfs_inode_priv_t *priv,
                          uiox_uint64_t new_size);

/* ── Snapshot + copy-on-write ─────────────────────────────────────────
 * There is NO JOURNAL wired in, so these make no journalled updates.
 * unfs_snap_delete() takes a third parameter for exactly that reason: the
 * reclaim of orphaned COW blocks was a journal checkpoint, and with no
 * journal there is nothing to checkpoint.  The count left behind is
 * REPORTED through reclaimed_out rather than performed.  Pass NULL if you
 * do not need it.
 *
 * unfs_cow_block() copies a block but does NOT repoint the inode's extent
 * at the copy — see the gap note in unfs_extra_unfs_snap.c.  Until
 * bmap_alloc() exists the caller must not use its result. */
int           unfs_snap_create(unfs_fs_t *fs, const char *name);
int           unfs_snap_delete(unfs_fs_t *fs, uiox_uint64_t snap_id,
                               uiox_uint32_t *reclaimed_out);
int           unfs_snap_list  (unfs_fs_t *fs, unfs_snap_t *out,
                               uiox_uint32_t max);
uiox_uint32_t unfs_cow_block  (unfs_fs_t *fs, uiox_uint32_t old_phys);

/* ── Extended attributes — 10_scfs's setxattr/getxattr return ENOSYS
 *    today because there is nowhere to store a value.  This is where. */
int  unfs_xattr_get(unfs_inode_priv_t *priv, const char *name,
                    void *val_out, uiox_uint16_t *len_out);
int  unfs_xattr_set(unfs_inode_priv_t *priv, const char *name,
                    const void *val, uiox_uint16_t len);

/*
 * REMOVED: the three ops tables and unfs_register().
 *
 *     extern const uiox_file_ops_t  unfs_file_ops;
 *     extern const uiox_inode_ops_t unfs_inode_ops;
 *     extern const uiox_fs_ops_t    unfs_fs_ops;
 *     void unfs_register(void);
 *
 * They declared a dispatch layer that uiox_vfs.h provided and 01_fsa does
 * not implement.  Keeping them would compile into symbols nothing calls.
 * If a VFS is ever built, they come back with it.
 */

#endif /* UNFS_FS_H */
