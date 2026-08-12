/*
 * 30_KIX/32_FS/include/unfs_fs.h
 *
 * UIOX Native Filesystem (UNFS) — kernel VFS integration header.
 *
 * This header bridges the UNFS on-disk format (shared with 01_uBoot)
 * and the kernel VFS layer (32_FS/01_fsa/vfs.c).
 *
 * Key additions over the bootloader reader:
 *   - Full read/write support
 *   - Journal integration (32_FS/02_journal)
 *   - Page cache integration (31_BufferCache/00_FileBuff)
 *   - COW (copy-on-write) for snapshots
 *   - MAC label enforcement (33_PCS/05_sec)
 *   - Block allocator / inode allocator
 *
 * @version 1.0.0  @date 2026-07-29
 */

 #ifndef UNFS_FS_H
 #define UNFS_FS_H
 
 #include "uiox_vfs.h"
 #include "uiox_base_types.h"
 
 /* Re-use all on-disk types from the shared header */
 #include "unfs_disk.h"      /* unfs_sb_t, unfs_inode_t, unfs_extent_t etc */
 
 /* ── Kernel-only limits ─────────────────────────────────────────────── */
 #define UNFS_MAX_GROUPS       256u   /* max block groups per volume      */
 #define UNFS_SNAP_MAX         8u     /* max snapshots per volume         */
 #define UNFS_XATTR_MAX        16u    /* max extended attributes per inode*/
 #define UNFS_XATTR_VAL_MAX    256u   /* max xattr value size             */
 
 /* ── Extended attribute entry ───────────────────────────────────────── */
 typedef struct {
     char     name[64];
     uint8_t  value[UNFS_XATTR_VAL_MAX];
     uint16_t val_len;
     uint8_t  inuse;
 } unfs_xattr_t;
 
 /* ── In-memory snapshot descriptor ─────────────────────────────────── */
 typedef struct {
     uint64_t  snap_id;
     uint64_t  created_ns;        /* timestamp                           */
     uint32_t  root_ino;          /* snapshot root inode number          */
     char      name[32];
     uint8_t   inuse;
 } unfs_snap_t;
 
 /* ── In-memory UNFS superblock (kernel) ─────────────────────────────── */
 typedef struct {
     unfs_sb_t          disk;                       /* on-disk superblock */
     unfs_group_desc_t  groups[UNFS_MAX_GROUPS];    /* group descriptors  */
     uint32_t           n_groups;
     uiox_superblock_t *vfs_sb;                     /* owning VFS sb      */
     unfs_snap_t        snapshots[UNFS_SNAP_MAX];
     uint8_t            n_snapshots;
     uint8_t            mounted;
     uint8_t            cow_active;
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
 
 /* ── Public kernel API ──────────────────────────────────────────────── */
 
 /* Filesystem registration — called from scfs_init() equivalent */
 void  unfs_register(void);
 
 /* Mount / unmount */
 int   unfs_kern_mount  (uiox_superblock_t *sb, uint32_t dev_id);
 int   unfs_kern_unmount(uiox_superblock_t *sb);
 int   unfs_kern_sync   (uiox_superblock_t *sb);
 
 /* Block allocation */
 uint32_t unfs_alloc_block (unfs_fs_t *fs);
 void     unfs_free_block  (unfs_fs_t *fs, uint32_t blkno);
 
 /* Inode allocation */
 uint32_t unfs_alloc_inode (unfs_fs_t *fs, uint16_t mode);
 void     unfs_free_inode  (unfs_fs_t *fs, uint32_t ino);
 
 /* Inode read / write */
 int  unfs_read_inode (unfs_fs_t *fs, uint32_t ino,
                       unfs_inode_priv_t *out);
 int  unfs_write_inode(unfs_fs_t *fs, const unfs_inode_priv_t *priv);
 
 /* Extent management */
 int  unfs_extent_add (unfs_fs_t *fs, unfs_inode_priv_t *priv,
                       uint32_t logical, uint32_t physical, uint16_t len);
 int  unfs_extent_truncate(unfs_fs_t *fs, unfs_inode_priv_t *priv,
                            uint64_t new_size);
 
 /* COW block write */
 uint32_t unfs_cow_block(unfs_fs_t *fs, uint32_t old_phys);
 
 /* Snapshot */
 int  unfs_snap_create(unfs_fs_t *fs, const char *name);
 int  unfs_snap_delete(unfs_fs_t *fs, uint64_t snap_id);
 int  unfs_snap_list  (unfs_fs_t *fs, unfs_snap_t *out, uint32_t max);
 
 /* Extended attributes */
 int  unfs_xattr_get(unfs_inode_priv_t *priv, const char *name,
                     void *val_out, uint16_t *len_out);
 int  unfs_xattr_set(unfs_inode_priv_t *priv, const char *name,
                     const void *val, uint16_t len);
 
 /* VFS file operations (registered at mount time) */
 extern const uiox_file_ops_t  unfs_file_ops;
 extern const uiox_inode_ops_t unfs_inode_ops;
 extern const uiox_fs_ops_t    unfs_fs_ops;
 
 #endif /* UNFS_FS_H */
 