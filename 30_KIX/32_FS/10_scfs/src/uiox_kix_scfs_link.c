/*
 * 30_KIX/32_FS/10_scfs/src/uiox_kix_scfs_link.c
 *
 * SCFS - Algorithm link.  Bach, The Design of the UNIX Operating System.
 *
 * -- Bach's algorithm, verbatim ---------------------------------------
 * "The link system call links a file to a new name in the file system
 *  directory structure, creating a new directory entry for an existing
 *  inode."
 *
 * -- the point of a hard link -----------------------------------------
 * NO new inode is allocated.  Two names point at one inode, and i_nlink
 * counts how many.  The blocks are freed only when unlink has removed the
 * last name AND no open file still references the inode - which is why
 * link and unlink both matter to the deferred-free rule.
 *
 * -- CHANGED: the dirent calls carry a LENGTH -------------------------
 * dir_lookup() and dir_add() take an explicit name length, because UNFS
 * dirents are variable-length and a name is not NUL-terminated on disk:
 *
 *     dir_lookup(dir, name, len)
 *     dir_add   (dir, name, len, ino, type)
 *
 * The previous calls passed a bare string and failed to compile:
 *
 *     link.c:97:  too few arguments to function 'dir_lookup'
 *     link.c:109: too few arguments to function 'dir_add'
 *
 * The length comes from scfs_name_len() below rather than from
 * scfs_strlen(), which is NOT declared anywhere in this layer - using it
 * would trade one compile error for another.
 *
 * @version 1.1.0  @date 2026-09-26
 */
#include "uiox_kix_scfs_internal.h"

/* -- the length of one path component, in bytes ------------------------
 * Deliberately local.  A component read off disk is not NUL-terminated,
 * and even here the bound matters: a name that never terminates must not
 * walk off the end, so the scan stops at UNFS_NAME_MAX.
 * -------------------------------------------------------------------- */
static uint32_t scfs_name_len(const char *name)
{
    uint32_t n = 0u;

    while (n < (uint32_t)UNFS_NAME_MAX && name[n] != '\0') n++;
    return n;
}

int uiox_kix_scfs_link(const char *oldpath, const char *newpath)
{
    if (!oldpath || !newpath) return SCFS_EFAULT;

    /* -- 1. inode of the existing file -------------------------------- */
    InCoreInode *ip = namei(oldpath, scfs_cwd_get(), 0u, 0u);  /* 01_fsa */
    if (!ip) return SCFS_ENOENT;

    /* -- 2. too many links, or a directory link from a non-super-user -- */
    /* Bach's MAX_LINKS check; 01_fsa's inode carries i_nlink as uint16. */
    if (ip->nlink >= 32767u) {
        iput(ip);
        return SCFS_EMLINK;
    }
    if (SCFS_IS_DIR(ip->mode)) {
        /* Directories may not be hard-linked - it would make the tree a
         * graph and break the ".." walk.  Bach permits it only for the
         * super user; this layer has no credential, so it refuses. */
        iput(ip);
        return SCFS_EPERM;
    }

    /* -- 3. increment the link count, update the disk copy ------------- */
    ip->nlink++;
    ip->flags |= IFLAG_CHANGED;
    iupdate(ip);                            /* 01_fsa */

    /* -- 4. unlock ----------------------------------------------------- */
    ip->locked = false;

    /* -- 5. parent directory of the new name --------------------------- */
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

    /* -- 6. the new name must not already exist ------------------------ */
    if (dir_lookup(dir, name, scfs_name_len(name)) != 0u) {   /* 01_fsa */
        dir->locked = false;
        iput(dir);
        ip->nlink--;                        /* undo step 3 */
        iupdate(ip);
        iput(ip);
        return SCFS_EEXIST;
    }

    /* -- 7. create the directory entry, recording the SAME inode ------ */
    /* Bach: "include new file name, inode number of existing file name".
     * No ialloc - that is the whole difference from open/creat.
     *
     * The type is the DIRENT encoding (UNFS_DT_*), not the inode one, and
     * this path guarantees a regular file: the SCFS_IS_DIR test above
     * refused every directory. */
    rc = dir_add(dir, name, scfs_name_len(name),
                 ip->ino, (uint8_t)UNFS_DT_REG);              /* 01_fsa */
    if (rc != 0) {
        dir->locked = false;
        iput(dir);
        ip->nlink--;                        /* undo step 3 */
        iupdate(ip);
        iput(ip);
        return SCFS_EIO;
    }

    /* -- 8. release the parent, then the existing file's inode --------- */
    dir->locked = false;
    iput(dir);                              /* 01_fsa */
    iput(ip);                               /* 01_fsa */

    return SCFS_OK;
}
