/*
 *  30_KIX/32_FS/10_scfs/include/uiox_kix_scfs_internal.h
 *
 *  SCFS — shared prologue for every unit under src/.
 *
 *  NOTE on this line: it used to name a file glob with a slash and a
 *  star, and the slash-star inside a block comment OPENS A NESTED
 *  COMMENT.  -Werror rejects that:
 *
 *      uiox_kix_scfs_internal.h:4: error: quoted slash-star within
 *      comment
 *
 *  The wording has been changed rather than escaped, because escaping
 *  does not help — the compiler scans the raw bytes and a backslash
 *  changes nothing.  It had been latent since the file was written;
 *  10_scfs had simply never been compiled.  Do not put a file glob, or
 *  any slash-star pair, inside a comment in this tree.
 *
 *  CORRECTED against the real 01_fsa headers.
 *
 *  ── what the first cut got wrong, and the fix ────────────────────────
 *    BLKSIZE            -> BLOCK_SIZE   (fs_types.h defines BLOCK_SIZE)
 *    sb_get() as a field reader -> sb_access.c accessors (named)
 *    readi/writei       -> now declared here and defined in
 *                          01_fsa/readwrite.c (NEW file)
 *
 *  ── the layer split ─────────────────────────────────────────────────
 *   10_scfs  FILE level   file table, user fd table, mount table
 *   01_fsa   INODE level  inode cache, buffer cache, bmap, superblock
 *
 *  SCFS calls 01_fsa directly.  No bridge, no vtable, no second inode type.
 *
 *  v1.1: header names corrected; no field names invented.
 */
#ifndef UIOX_KIX_SCFS_INTERNAL_H
#define UIOX_KIX_SCFS_INTERNAL_H

#include "uiox_kix_scfs.h"

/* ═════════════════════════════════════════════════════════════════════
 * 01_fsa — the i-node level, called directly
 *
 * These are the exact names and signatures 01_fsa exports.  The real
 * headers are included below, so a signature drift is a compile error
 * rather than a silent mismatch.
 * ═════════════════════════════════════════════════════════════════════ */

/* ── the real types — never redeclared in this layer ──────────────── */
#include "fs_types.h"     /* BLOCK_SIZE, MAX_*, NDIRECT, FileType, PERM_* */
#include "buffer.h"       /* BufEntry, getblk/bread/bwrite/brelse        */
#include "inode.h"        /* DiskInode, InCoreInode, iget/iput/iupdate   */
#include "namei.h"        /* DirEntry, ROOT_INO, namei/dir_*             */
#include "superblock.h"   /* SuperBlock, fs_alloc/fs_free/ialloc/ifree   */
#include "bmap.h"         /* BmapResult, bmap, bmap_alloc                */
#include "readwrite.h"    /* readi/writei/readi_at/writei_at  (NEW)      */

/* ═════════════════════════════════════════════════════════════════════
 * The 01_fsa super block accessors (01_fsa/src/sb_access.c)
 *
 * superblock.c declares `static SuperBlock sb;`, so no other unit can
 * reach it.  These are the named readers instead of a struct field walk.
 * ═════════════════════════════════════════════════════════════════════ */
uint32_t sb_total_blocks(void);
uint32_t sb_free_blocks (void);
uint32_t sb_total_inodes(void);
uint32_t sb_free_inodes (void);
uint32_t sb_block_size  (void);
int      sb_is_modified (void);
void     sb_clear_modified(void);

/* ═════════════════════════════════════════════════════════════════════
 * Extra 01_fsa symbols SCFS calls that the headers do not declare
 * ═════════════════════════════════════════════════════════════════════ */

/* fs_free_inode_blocks is defined in superblock.c and declared in
 * superblock.h — nothing further needed. */

/* inode_cache_sync does NOT exist in 01_fsa.  Bach's sync sweeps the
 * inode cache for changed inodes; this filesystem has no such sweep, so
 * SCFS's sync() must not call one.  It is replaced by a local walk of
 * the inode cache's public surface — see sync.c, which now relies on
 * buf_sync() plus iupdate() on the inodes it actually knows about. */

/* ═════════════════════════════════════════════════════════════════════
 * SCFS's own table helpers  (uiox_kix_scfs_table.c)
 * ═════════════════════════════════════════════════════════════════════ */

/* the hinge — descriptor number to file table entry, or NULL */
scfs_file_t *scfs_getf(int fd);

/* Bach's two allocation steps, kept separate:
 *   scfs_falloc       allocate a FILE TABLE entry (count, offset)
 *   scfs_ufd_alloc    allocate a USER FD slot pointing at it  */
scfs_file_t *scfs_falloc      (InCoreInode *ip, int flags, uint32_t perm_mask);
int          scfs_ufd_alloc   (scfs_file_t *f);
int          scfs_ufd_find    (void);
void         scfs_fclose_entry(scfs_file_t *f);
void         scfs_fput        (scfs_file_t *f);

/* dup: point two descriptors at ONE file table entry */
int          scfs_ufd_link    (scfs_file_t *f);

/* create a node for O_CREAT — ialloc plus the directory entry */
InCoreInode *scfs_create_node (const char *path, uint16_t perm);

/* split a path into its parent directory and final component */
int          scfs_path_split  (const char *path, char *parent, uint32_t sz,
                               const char **name);

/* ═════════════════════════════════════════════════════════════════════
 * Process state — cwd, root, umask, credentials
 *
 * Bach keeps cwd and root in the u area.  33_PCS owns them; the bring-up
 * fallback lives in uiox_kix_scfs_table.c and access.c.
 * ═════════════════════════════════════════════════════════════════════ */
InCoreInode *scfs_cwd_get (void);
void         scfs_cwd_set (InCoreInode *ip);
InCoreInode *scfs_root_get(void);
void         scfs_root_set(InCoreInode *ip);

uint16_t scfs_umask_get(void);
uint16_t scfs_umask_set(uint16_t mask);

void     scfs_ufdt_bind(uint16_t unused_);
void     scfs_cred_set (uint16_t uid, uint16_t gid);
uint16_t scfs_uid_get  (void);
uint16_t scfs_gid_get  (void);

/* the current time, as the inode stores it (seconds) */
void     scfs_time_set(int64_t t);
int64_t  scfs_time_now(void);

/* ═════════════════════════════════════════════════════════════════════
 * Mount table
 * ═════════════════════════════════════════════════════════════════════ */
scfs_mount_t *scfs_mount_alloc(void);
void          scfs_mount_free (scfs_mount_t *mp);
scfs_mount_t *scfs_mount_find (uint16_t dev);

#endif /* UIOX_KIX_SCFS_INTERNAL_H */
