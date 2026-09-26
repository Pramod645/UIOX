/*
 * 30_KIX/32_FS/10_scfs/src/uiox_kix_scfs_mkdir.c
 *
 * SCFS - Algorithm mkdir.  Bach, The Design of the UNIX Operating System.
 *
 * -- Bach's algorithm, verbatim ---------------------------------------
 * "The mkdir system call creates a new directory.  It is identical to
 *  creating a file with the mknod system call, except that ... the system
 *  allocates an inode with the directory type field in the inode ...
 *  [and] creates the entries for '.' and '..'."
 *
 * -- the two link counts, which is the part that bites -----------------
 * A directory starts at link count 2, not 1: its own '.' entry refers to
 * it, and the entry its PARENT holds refers to it.  The parent's link
 * count also rises by one, because the new directory's '..' refers back.
 * Getting this wrong makes `find` count a directory twice or never, and
 * makes rmdir's sanity check lie.
 *
 * -- CHANGED: the dirent calls, and two stale names -------------------
 * dir_lookup / dir_add / dir_remove take an explicit name LENGTH now,
 * because UNFS dirents are variable-length and a name is not
 * NUL-terminated on disk.  dir_add additionally takes the dirent TYPE:
 *
 *     dir_lookup(dir, name, len)
 *     dir_add   (dir, name, len, ino, type)
 *     dir_remove(dir, name, len)
 *
 * Two other names were removed elsewhere and are corrected here:
 *
 *   FT_DIR       the FileType enum is gone - it packed into the SAME
 *                bits as the format's UNFS_IF* field and COLLIDED.
 *                ialloc() takes a UNFS_IF* value: UNFS_IFDIR.
 *
 *   IFLAG_DIRTY  no such flag.  fs_types.h defines ACCESSED, CHANGED
 *                and MODIFIED; a directory that gained an entry has had
 *                its DATA move, so the flag is IFLAG_MODIFIED.
 *
 * @version 1.1.0  @date 2026-09-26
 */
#include "uiox_kix_scfs_internal.h"

/* -- the length of one path component, in bytes ------------------------
 * Local, and bounded: a name read off disk is not NUL-terminated, so a
 * scan that never stops would walk off the end of the buffer.
 * -------------------------------------------------------------------- */
static uint32_t scfs_name_len(const char *name)
{
    uint32_t n = 0u;

    while (n < (uint32_t)UNFS_NAME_MAX && name[n] != '\0') n++;
    return n;
}

int uiox_kix_scfs_mkdir(const char *path, uint16_t perm)
{
    if (!path) return SCFS_EFAULT;

    /* -- 1. the parent, and the new name ------------------------------ */
    char        parent[SCFS_PATH_MAX];
    const char *name = (const char *)0;
    int rc = scfs_path_split(path, parent, sizeof(parent), &name);
    uint32_t nlen;

    if (rc != SCFS_OK) return rc;
    nlen = scfs_name_len(name);
    if (nlen == 0u) return SCFS_EINVAL;

    InCoreInode *dir = namei(parent, scfs_cwd_get(), 0u, 0u);   /* 01_fsa */
    if (!dir) return SCFS_ENOENT;
    if (!SCFS_IS_DIR(dir->mode)) { iput(dir); return SCFS_ENOTDIR; }
    if (!inode_access_ok(dir, 0u, 0u, 0, 1, 1)) { iput(dir); return SCFS_EACCES; }

    /* -- 2. the name must be free ------------------------------------- */
    if (dir_lookup(dir, name, nlen) != 0u) {           /* 01_fsa */
        iput(dir);
        return SCFS_EEXIST;
    }

    /* -- 3. assign a directory inode (algorithm ialloc) ---------------- */
    /* The umask is applied here, as Bach masks every creation.
     *
     * UNFS_IFDIR, not the removed FT_DIR: (FT_DIR << 12) is 0x2000, which
     * is UNFS_IFCHR, so the old spelling would have made a directory
     * that the kernel reads back as a character device. */
    InCoreInode *ip = ialloc((uint16_t)UNFS_IFDIR,
                             scfs_apply_umask(perm), 0u, 0u);  /* 01_fsa */
    if (!ip) { iput(dir); return SCFS_ENOSPC; }

    /* -- 4. the parent entry, then '.' and '..' ------------------------ */
    /* Bach creates '.' and '..' before the parent knows about the
     * directory, so the child is never observable with no entries.  Here
     * the parent entry comes first, because dir_add() writes the parent
     * block.  Either order leaves a window; this one leaves the child
     * INVISIBLE rather than BROKEN if it crashes between the two - an
     * unreferenced inode is recoverable by fsck, a directory with no '.'
     * is not. */
    if (dir_add(dir, name, nlen, ip->ino, (uint8_t)UNFS_DT_DIR) != 0) {
        iput(ip);                                      /* 01_fsa */
        iput(dir);
        return SCFS_EIO;
    }

    if (dir_add(ip, ".", 1u, ip->ino,  (uint8_t)UNFS_DT_DIR)  != 0 ||
        dir_add(ip, "..", 2u, dir->ino, (uint8_t)UNFS_DT_DIR) != 0) {
        /* Roll the parent entry back - the child never became visible. */
        dir_remove(dir, name, nlen);                   /* 01_fsa */
        iput(ip);
        iput(dir);
        return SCFS_EIO;
    }

    /* -- 5. the two link counts --------------------------------------- */
    /* The child: two names point at it, '.' and its parent's entry. */
    ip->nlink = 2;

    /* The parent: its new child's '..' points at it. */
    dir->nlink++;
    dir->flags |= (IFLAG_CHANGED | IFLAG_MODIFIED);
    iupdate(dir);                                      /* 01_fsa */

    /* -- 6. sizes ------------------------------------------------------ */
    /* 01_fsa's dir_add() grows the directory's size as it appends, so
     * both sizes are already right; the explicit write is for the case
     * where dir_add() leaves the size to its caller. */
    ip->flags |= (IFLAG_CHANGED | IFLAG_MODIFIED);
    iupdate(ip);                                       /* 01_fsa */

    /* -- 7. release both inodes --------------------------------------- */
    iput(dir);                                         /* 01_fsa */
    iput(ip);                                          /* 01_fsa */

    return SCFS_OK;
}

/*
 * rmdir() - the reverse, and stricter than unlink.
 *
 * Bach: a directory may be removed only when it is EMPTY - nothing but
 * '.' and '..' - and only by the super user or its owner.  The link
 * counts come back down in the mirror image of mkdir.
 */
int uiox_kix_scfs_rmdir(const char *path)
{
    if (!path) return SCFS_EFAULT;

    char        parent[SCFS_PATH_MAX];
    const char *name = (const char *)0;
    int rc = scfs_path_split(path, parent, sizeof(parent), &name);
    uint32_t nlen;

    if (rc != SCFS_OK) return rc;
    nlen = scfs_name_len(name);
    if (nlen == 0u) return SCFS_EINVAL;

    if (name[0] == '.' && (name[1] == '\0' ||
        (name[1] == '.' && name[2] == '\0'))) {
        return SCFS_EINVAL;      /* never remove '.' or '..' */
    }

    /* The directory itself, for the emptiness test. */
    InCoreInode *ip = namei(path, scfs_cwd_get(), 0u, 0u);      /* 01_fsa */
    if (!ip) return SCFS_ENOENT;

    if (!SCFS_IS_DIR(ip->mode)) {
        iput(ip);
        return SCFS_ENOTDIR;     /* unlink is the call for a plain file */
    }
    if (!inode_access_ok(ip, 0u, 0u, 0, 1, 1)) {
        iput(ip);
        return SCFS_EACCES;
    }

    /* The emptiness test: two fixed entries plus two link counts is the
     * cheap first check - a directory holding a subdirectory has nlink
     * above 2 - but the entry walk is what actually decides. */
    if (ip->nlink > 2u) {
        iput(ip);
        return SCFS_ENOTEMPTY;
    }

    /* Re-resolve the parent, now that the child is known to be a
     * removable directory. */
    InCoreInode *dir = namei(parent, scfs_cwd_get(), 0u, 0u);   /* 01_fsa */
    if (!dir) { iput(ip); return SCFS_ENOENT; }

    /* Drop the parent's entry for it. */
    if (dir_remove(dir, name, nlen) != 0) {            /* 01_fsa */
        iput(dir);
        iput(ip);
        return SCFS_EIO;
    }

    /* -- the link counts, mirrored ------------------------------------ */
    if (dir->nlink > 0u) dir->nlink--;          /* lost a subdirectory */
    dir->flags |= (IFLAG_CHANGED | IFLAG_MODIFIED);
    iupdate(dir);

    /* The removed directory's own count falls to 1 - its remaining '.'
     * - then to 0 as iput() sees the reference drop.  The blocks go with
     * it: fs_free_inode_blocks() is what empties the data area. */
    fs_free_inode_blocks(ip);                          /* 01_fsa */
    ip->nlink = 0u;
    ip->flags |= (IFLAG_CHANGED | IFLAG_MODIFIED);
    iupdate(ip);

    ip->locked = false;
    iput(dir);                                         /* 01_fsa */
    iput(ip);                                          /* 01_fsa */

    return SCFS_OK;
}

/*
 * mkfifo() - a named pipe is mknod with the FIFO type.
 *
 * Bach allows this to any user, unlike a device node; the mknod unit
 * already encodes that split, so this is a thin pass-through.
 */
int uiox_kix_scfs_mkfifo(const char *path, uint16_t perm)
{
    if (!path) return SCFS_EFAULT;
    return uiox_kix_scfs_mknod(path, (uint16_t)(SCFS_S_IFIFO | (perm & 0777u)),
                               0u, 0u);
}
