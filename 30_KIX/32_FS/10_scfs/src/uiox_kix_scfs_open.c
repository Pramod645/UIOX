/*
 * 30_KIX/32_FS/10_scfs/src/uiox_kix_scfs_open.c
 *
 * SCFS — Algorithm open.  Bach, The Design of the UNIX Operating System.
 *
 * ── Bach's algorithm, verbatim ─────────────────────────────────────────
 * inputs: file name, type of open, file permission (for creation type)
 * output: file descriptor
 * {
 *   convert file name to inode (algorithm namei);
 *   if (file does not exist or not permitted access)
 *       return (error);
 *   allocate file table entry for inode, initialize count, offset;
 *   allocate user file descriptor entry, set pointer to file table entry;
 *   if (type of open specifies truncate file)
 *       free all file blocks (algorithm free);
 *   unlock (namei);        // locked above in namei
 *   return (user file descriptor);
 * }
 *
 * ── layering ───────────────────────────────────────────────────────────
 * SCFS owns the FILE TABLE and the USER FD TABLE.  01_fsa owns namei(),
 * iget(), iput(), fs_free() and the inode cache.  Every call below goes
 * straight into 01_fsa — no bridge, no vtable.
 *
 * @version 1.0.0  @date 2026-09-23
 */
#include "uiox_kix_scfs_internal.h"

/* Bach splits the permission word's low 9 bits; the mode word's top
 * nibble carries the file type under 01_fsa's encoding. */
#define SCFS_PERM_MASK 0777u

/*
 * open() — Algorithm open.
 *
 * The fd returned indexes the USER FILE DESCRIPTOR TABLE; the slot points
 * at a FILE TABLE entry, which points at the inode.
 */
int uiox_kix_scfs_open(const char *path, int flags, uint16_t mode)
{
    if (!path) return SCFS_EFAULT;

    /* ── 1. convert the file name to an inode (algorithm namei) ─────── */
    InCoreInode *ip = namei(path, scfs_cwd_get(), 0u, 0u);   /* 01_fsa */

    if (!ip) {
        /*
         * namei returned no inode.  Two cases, and Bach treats them
         * together: the file does not exist, or it exists but the path
         * walk was refused.  Only O_CREAT can turn the first into a
         * success; the second must stay an error.
         */
        if (!(flags & O_CREAT)) return SCFS_ENOENT;

        ip = scfs_create_node(path, (uint16_t)(mode & SCFS_PERM_MASK));
        if (!ip) return SCFS_EACCES;
    } else {
        /* ── 2. the file exists — check access is permitted ─────────── */
        int need_w = (flags & O_WRONLY) || (flags & O_RDWR);
        int need_r = !(flags & O_WRONLY) || (flags & O_RDWR);

        if (!inode_access_ok(ip, 0u, 0u, need_r, need_w, 0)) {   /* 01_fsa */
            iput(ip);                                            /* 01_fsa */
            return SCFS_EACCES;
        }

        /* O_EXCL with O_CREAT means "must not exist" — it does. */
        if ((flags & O_CREAT) && (flags & O_EXCL)) {
            iput(ip);
            return SCFS_EEXIST;
        }
    }

    /* ── 3. allocate a FILE TABLE entry ─────────────────────────────── */
    /* Bach initializes the reference count to 1 and the offset to 0,
     * except for an append-mode open, which starts at the file's end. */
    scfs_file_t *f = scfs_falloc(ip, flags, SCFS_PERM_MASK);
    if (!f) { iput(ip); return SCFS_ENFILE; }

    /* ── 4. allocate a USER FD entry pointing at that file entry ────── */
    int fd = scfs_ufd_alloc(f);
    if (fd < 0) {
        scfs_fclose_entry(f);        /* undo step 3 */
        iput(ip);                    /* 01_fsa */
        return SCFS_EMFILE;
    }

    /* ── 5. O_TRUNC: free all the file's blocks (algorithm free) ────── */
    if ((flags & O_TRUNC) && (flags & (O_WRONLY | O_RDWR))) {
        if (ip->size != 0u) {
            fs_free_inode_blocks(ip);       /* 01_fsa: algorithm free */
            iupdate(ip);                    /* write the zeroed inode */
        }
    }

    /*
     * ── 6. unlock the inode ─────────────────────────────────────────
     * namei() returned it locked.  01_fsa's iput() releases the lock
     * while keeping the cache reference, which is exactly what open
     * needs: the inode stays resident because the file table points
     * at it.
     */
    ip->locked = false;

    return fd;
}
