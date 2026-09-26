/*
 * 30_KIX/32_FS/10_scfs/src/uiox_kix_scfs_unlink.c
 *
 * SCFS — Algorithm unlink.  Bach, The Design of the UNIX Operating System.
 *
 * ── Bach's algorithm, verbatim ─────────────────────────────────────────
 * input: file name
 * output: none
 * {
 *   get parent inode of file to be unlinked (algorithm namei);
 *       // if unlinking the current directory "."
 *   if (last component of file name is ".")
 *       increment inode reference count;
 *   else
 *       get inode of file to be unlinked (algorithm iget);
 *   if (file is directory but user is not super user)
 *   {
 *       release inodes (algorithm iput);
 *       return (error);
 *   }
 *   if (shared text file and link count currently 1)
 *       remove from region table;
 *   write parent directory: zero inode number of unlinked file;
 *   release inode parent directory (algorithm iput);
 *   decrement file link count;
 *   release file inode (algorithm iput);
 *       // iput checks if link count is 0: if so, releases file blocks
 *       // (algorithm free) and frees inode (algorithm ifree);
 * }
 *
 * ── the deferred free, which is the whole subtlety ─────────────────────
 * The name goes away immediately.  The blocks do not.  iput() frees them
 * only when BOTH conditions hold: i_nlink has reached 0 (this was the
 * last name) AND the inode's reference count has reached 0 (no open file
 * still names it).  That is why a file removed while open keeps working —
 * and why the released reference here must be dropped after the directory
 * write, not before.
 *
 * @version 1.0.0  @date 2026-09-23
 */
#include "uiox_kix_scfs_internal.h"

static uint32_t scfs_name_len(const char *name)
{
    uint32_t n = 0u;
    while (n < (uint32_t)UNFS_NAME_MAX && name[n] != '\0') n++;
    return n;
}

int uiox_kix_scfs_unlink(const char *path)
{
    if (!path) return SCFS_EFAULT;

    /* ── 1. the parent directory's inode ───────────────────────────── */
    char        parent[SCFS_PATH_MAX];
    const char *name = (const char *)0;
    int rc = scfs_path_split(path, parent, sizeof(parent), &name);
    if (rc != SCFS_OK) return rc;

    /* Removing "." or ".." is refused outright: it would detach a
     * directory from the tree.  Bach special-cases "." to keep the
     * reference count sane because his kernel allowed it; this one does
     * not, so the check is a refusal rather than a count adjustment. */
    if (name[0] == '.' && (name[1] == '\0' ||
        (name[1] == '.' && name[2] == '\0'))) {
        return SCFS_EINVAL;
    }

    InCoreInode *dir = namei(parent, scfs_cwd_get(), 0u, 0u);  /* 01_fsa */
    if (!dir) return SCFS_ENOENT;
    if (!SCFS_IS_DIR(dir->mode)) { iput(dir); return SCFS_ENOTDIR; }
    if (!inode_access_ok(dir, 0u, 0u, 0, 1, 1)) { iput(dir); return SCFS_EACCES; }

    /* ── 2. the inode of the file to be unlinked (algorithm iget) ──── */
    uint32_t ino = dir_lookup(dir, name, scfs_name_len(name));       /* 01_fsa */
    if (ino == 0u) {
        iput(dir);
        return SCFS_ENOENT;
    }

    InCoreInode *ip = iget(ino);                /* 01_fsa */
    if (!ip) {
        iput(dir);
        return SCFS_EIO;
    }

    /* ── 3. a directory may not be unlinked here ───────────────────── */
    /* Bach allows it only for the super user; rmdir exists for the
     * ordinary case, and this layer has no credential to check. */
    if (SCFS_IS_DIR(ip->mode)) {
        iput(ip);
        iput(dir);
        return SCFS_EISDIR;
    }

    /* ── 4. zero the inode number in the parent's directory entry ──── */
    /* Bach: "write parent directory: zero inode number of unlinked
     * file".  dir_remove() does exactly that and writes the block back
     * through the buffer cache. */
    rc = dir_remove(dir, name, scfs_name_len(name));                 /* 01_fsa */
    if (rc != 0) {
        iput(ip);
        iput(dir);
        return SCFS_EIO;
    }

    /* ── 5. release the parent directory's inode ───────────────────── */
    iput(dir);                                  /* 01_fsa */

    /* ── 6. decrement the file's link count ────────────────────────── */
    if (ip->nlink > 0u) ip->nlink--;
    ip->flags |= IFLAG_CHANGED;

    /* ── 7. release the file's inode (algorithm iput) ──────────────── */
    /*
     * This is where the deferred free happens, and 01_fsa's iput()
     * already implements the rule: when the reference count reaches 0
     * AND the link count is 0, it frees the blocks with fs_free_* and
     * returns the inode with ifree().  If an open file still holds the
     * inode, the reference count is above 0 here and the blocks survive
     * until that file is closed.
     */
    iupdate(ip);                                /* persist the nlink  */
    iput(ip);                                   /* 01_fsa */

    return SCFS_OK;
}
