/*
 * 30_KIX/32_FS/10_scfs/src/uiox_kix_scfs_link.c
 *
 * SCFS — Algorithm link.  Bach, The Design of the UNIX Operating System.
 *
 * ── Bach's algorithm, verbatim ─────────────────────────────────────────
 * "The link system call links a file to a new name in the file system
 *  directory structure, creating a new directory entry for an existing
 *  inode."
 *
 * input: existing file name, new file name
 * output: none
 * {
 *   get inode for existing file name (algorithm namei);
 *   if (too many links on file, or linking directory without super user)
 *   {
 *       release inode (algorithm iput);
 *       return (error);
 *   }
 *   increment link count on inode;
 *   update disk copy of inode;
 *   unlock inode;
 *   get parent inode for directory to contain new file name
 *       (algorithm namei);
 *   if (new file name already exists, or existing file and new file on
 *       different file systems)
 *   {
 *       undo update done above;
 *       return (error);
 *   }
 *   create new directory entry in parent directory of new file name:
 *       include new file name, inode number of existing file name;
 *   release parent directory inode (algorithm iput);
 *   release inode of existing file (algorithm iput);
 * }
 *
 * ── the point of a hard link ───────────────────────────────────────────
 * NO new inode is allocated.  Two names point at one inode, and i_nlink
 * counts how many.  The blocks are freed only when unlink has removed the
 * last name AND no open file still references the inode — which is why
 * link and unlink both matter to the deferred-free rule.
 *
 * @version 1.0.0  @date 2026-09-23
 */
#include "uiox_kix_scfs_internal.h"

int uiox_kix_scfs_link(const char *oldpath, const char *newpath)
{
    if (!oldpath || !newpath) return SCFS_EFAULT;

    /* ── 1. inode of the existing file ─────────────────────────────── */
    InCoreInode *ip = namei(oldpath, scfs_cwd_get(), 0u, 0u);  /* 01_fsa */
    if (!ip) return SCFS_ENOENT;

    /* ── 2. too many links, or a directory link from a non-super-user ─ */
    /* Bach's MAX_LINKS check; 01_fsa's inode carries i_nlink as uint16. */
    if (ip->nlink >= 32767u) {
        iput(ip);
        return SCFS_EMLINK;
    }
    if (SCFS_S_ISDIR(ip->mode)) {
        /* Directories may not be hard-linked — it would make the tree a
         * graph and break the ".." walk.  Bach permits it only for the
         * super user; this layer has no credential, so it refuses. */
        iput(ip);
        return SCFS_EPERM;
    }

    /* ── 3. increment the link count, update the disk copy ──────────── */
    ip->nlink++;
    ip->flags |= IFLAG_CHANGED;
    iupdate(ip);                            /* 01_fsa */

    /* ── 4. unlock ─────────────────────────────────────────────────── */
    ip->locked = false;

    /* ── 5. parent directory of the new name ───────────────────────── */
    char        parent[SCFS_PATH_MAX];
    const char *name = (const char *)0;
    int rc = scfs_path_split(newpath, parent, sizeof(parent), &name);
    if (rc != SCFS_OK) {
        ip->nlink--;                        /* undo step 3 */
        iupdate(ip);
        iput(ip);
        return rc;
    }

    InCoreInode *dir = namei(parent, scfs_cwd_get(), 0u, 0u);  /* 01_fsa */
    if (!dir) {
        ip->nlink--;                        /* undo step 3 */
        iupdate(ip);
        iput(ip);
        return SCFS_ENOENT;
    }

    /* ── 6. the new name must not already exist ────────────────────── */
    if (dir_lookup(dir, name) != 0u) {      /* 01_fsa */
        dir->locked = false;
        iput(dir);
        ip->nlink--;                        /* undo step 3 */
        iupdate(ip);
        iput(ip);
        return SCFS_EEXIST;
    }

    /* ── 7. create the directory entry, recording the SAME inode ───── */
    /* Bach: "include new file name, inode number of existing file name".
     * No ialloc — that is the whole difference from open/creat. */
    rc = dir_add(dir, name, ip->ino);       /* 01_fsa */
    if (rc != 0) {
        dir->locked = false;
        iput(dir);
        ip->nlink--;                        /* undo step 3 */
        iupdate(ip);
        iput(ip);
        return SCFS_EIO;
    }

    /* ── 8. release the parent, then the existing file's inode ─────── */
    dir->locked = false;
    iput(dir);                              /* 01_fsa */
    iput(ip);                               /* 01_fsa */

    return SCFS_OK;
}
