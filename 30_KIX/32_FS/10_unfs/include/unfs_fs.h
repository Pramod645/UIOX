/*
 * 30_KIX/32_FS/10_unfs/include/unfs_fs.h
 *
 * UIOX Native Filesystem (UNFS) — kernel-side UNFS integration header.
 *
 * This is a fork of 32_FS/include/unfs_fs.h.  Same surface, plus the
 * snapshot / copy-on-write declarations, so 10_unfs has ONE header to
 * include rather than two.  unfs_extra_unfs_snap.h is retired — delete it.
 *
 * -- CHANGED FROM 32_FS/include/unfs_fs.h -----------------------------
 *
 *  1. unfs_fs_t gained four fields the snapshot code needs:
 *
 *         unfs_snap_t snapshots[UNFS_SNAP_MAX];
 *         uint8_t     n_snapshots;
 *         uint8_t     cow_active;
 *         uint32_t    dev_id;
 *
 *     The first three were present in an earlier revision and were lost;
 *     unfs_extra_unfs_snap.c dereferences all of them.  dev_id is new —
 *     unfs_cow_block() reads through the block cache and needs to say
 *     WHICH device, and nothing on this struct said so before.
 *
 *  2. The snapshot API and unfs_cow_block() are declared here.  They were
 *     previously in a separate header, which meant two files to keep in
 *     step for one subsystem.
 *
 *  3. UNFS_SNAP_MAX and UNFS_SNAP_NAME_MAX are defined here.
 *
 *  4. unfs_extra_unfs_snap.h is NOT included — it should be deleted.
 *
 * -- what was removed from the ORIGINAL, and why (unchanged) ----------
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
 * @version 2.1.0  @date 2026-09-27
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
    uint64_t  snap_id;                 /* 1-based, never reused         */
    uint64_t  created_ns;              /* 0 — no clock in this layer    */
    uint32_t  root_ino;                /* UNFS_ROOT_INO at snapshot time*/
    char      name[UNFS_SNAP_NAME_MAX];
    uint8_t   inuse;
} unfs_snap_t;

/* ── In-memory UNFS superblock (kernel) ─────────────────────────────── */
typedef struct {
    unfs_sb_t          disk;                       /* on-disk superblock */
    unfs_group_desc_t  groups[UNFS_MAX_GROUPS];    /* group descriptors  */
    uint32_t           n_groups;
    SuperBlock        *sb;        /* 01_fsa's superblock, not the VFS's */

    /* ── snapshot / COW state ─────────────────────────────────────────
     * cow_active lives HERE, in memory, and is NOT written back to the
     * on-disk superblock.  An earlier revision had an s_cow_enabled
     * byte in unfs_sb_t; it was removed from the format, and COW state
     * is private to a mount.  unfs_extra_unfs_snap.c must therefore
     * drop its two fs->disk.s_cow_enabled assignments — see the note in
     * that file.  Nothing is lost: the flag only steers unfs_cow_block,
     * and a fresh mount correctly starts with COW off. */
    unfs_snap_t        snapshots[UNFS_SNAP_MAX];
    uint8_t            n_snapshots;
    uint8_t            cow_active;

    /* The device these blocks come from.  unfs_cow_block() reads through
     * the block cache and needs to name the device; nothing else on this
     * struct did.  If SuperBlock turns out to carry the device number,
     * prefer fs->sb->… and delete this field. */
    uint32_t           dev_id;

    uint8_t            mounted;
} unfs_fs_t;

/* ── In-memory inode private data ───────────────────────────────────── */
typedef struct {
    unfs_inode_t  disk;                  /* on-disk inode copy          */
    uint32_t      ino;                   /* inode number                */
    unfs_fs_t    *fs;                    /* owning filesystem           */
    unfs_xattr_t  xattrs[UNFS_XATTR_MAX];
    uint8_t       n_xattrs;
    uint8_t       dirty;                 /* inode needs writeback       */
} unfs_inode_priv_t;

/* ── Block allocator state ──────────────────────────────────────────── */
typedef struct {
    uint32_t  last_grp;    /* last group used for allocation            */
    uint32_t  last_blk;    /* last block allocated                      */
} unfs_alloc_t;

/* ═════════════════════════════════════════════════════════════════════
 * Public kernel API
 *
 * Registration is gone: there is no registry.  These are called directly,
 * the way 01_fsa's own entry points are, and it is how 10_scfs's ENOSYS
 * stubs for xattr and the missing extent allocator get a body.
 * ═════════════════════════════════════════════════════════════════════ */

/* Mount / unmount / sync — 01_fsa provides these; declared here so a
 * caller has one include.  They are NOT new implementations. */
int   unfs_kern_mount  (SuperBlock *sb, uint32_t dev_id);
int   unfs_kern_unmount(SuperBlock *sb);
int   unfs_kern_sync   (SuperBlock *sb);

/* Block allocation — bitmap-based; 01_fsa's fs_alloc is Bach's cached
 * free list and cannot coalesce runs the way an extent allocator needs.
 *
 * unfs_alloc_block has NO IMPLEMENTATION YET.  unfs_cow_block() calls it,
 * so anything using snapshots will link-fail until a body exists.  The
 * body is an allocator over the group bitmaps unfs_format.c lays down. */
uint32_t unfs_alloc_block (unfs_fs_t *fs);
void     unfs_free_block  (unfs_fs_t *fs, uint32_t blkno);

/* Inode allocation — likewise bitmap-based. */
uint32_t unfs_alloc_inode (unfs_fs_t *fs, uint16_t mode);
void     unfs_free_inode  (unfs_fs_t *fs, uint32_t ino);

/* Inode read / write — 01_fsa's inode_disk_read / iupdate are the
 * implementations; these are the UNFS_priv-flavoured wrappers. */
int  unfs_read_inode (unfs_fs_t *fs, uint32_t ino, unfs_inode_priv_t *out);
int  unfs_write_inode(unfs_fs_t *fs, const unfs_inode_priv_t *priv);

/* Extent management — the piece 01_fsa's truncate.c trims but never adds. */
int  unfs_extent_add (unfs_fs_t *fs, unfs_inode_priv_t *priv,
                      uint32_t logical, uint32_t physical, uint16_t len);
int  unfs_extent_truncate(unfs_fs_t *fs, unfs_inode_priv_t *priv,
                          uint64_t new_size);

/* ── Snapshot + copy-on-write ─────────────────────────────────────────
 * Snapshot creation records the root inode and turns COW on; deletion
 * clears it and turns COW off when the last one goes.
 *
 * There is NO JOURNAL wired in, so these make no journalled updates.
 * unfs_snap_delete() takes a third parameter for exactly that reason:
 * the reclaim of orphaned COW blocks was a journal checkpoint, and with
 * no journal there is nothing to checkpoint.  The count of blocks left
 * behind is REPORTED through reclaimed_out rather than performed.
 * Pass NULL if you do not need it.
 *
 * unfs_cow_block() returns the new physical block, old_phys when COW is
 * off, or 0 when allocation failed. */
int      unfs_snap_create(unfs_fs_t *fs, const char *name);
int      unfs_snap_delete(unfs_fs_t *fs, uint64_t snap_id,
                          uint32_t *reclaimed_out);
int      unfs_snap_list  (unfs_fs_t *fs, unfs_snap_t *out, uint32_t max);
uint32_t unfs_cow_block  (unfs_fs_t *fs, uint32_t old_phys);

/* Extended attributes — 10_scfs's setxattr/getxattr return ENOSYS today
 * because there is nowhere to store a value.  This is where. */
int  unfs_xattr_get(unfs_inode_priv_t *priv, const char *name,
                    void *val_out, uint16_t *len_out);
int  unfs_xattr_set(unfs_inode_priv_t *priv, const char *name,
                    const void *val, uint16_t len);

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
