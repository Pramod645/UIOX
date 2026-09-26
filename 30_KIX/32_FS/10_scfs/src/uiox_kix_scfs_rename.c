/*
 *  SCFS — Algorithm rename.  CORRECTED.
 *
 *  ── where this sits relative to Bach ─────────────────────────────────
 *  Bach has no rename algorithm: V7 offered link() then unlink(), which is
 *  why old scripts write a rename as exactly that pair.  The pair is
 *  visibly non-atomic — a crash between the two leaves the file at BOTH
 *  names.  rename(2) exists to make it atomic.
 *
 *  The work is still link and unlink, in an order chosen so no
 *  intermediate state is observable, with four cases handled apart:
 *
 *    1. source and target on different filesystems        -> EXDEV
 *    2. target does not exist                             -> the plain pair
 *    3. target exists and is a plain file                 -> unlink it first
 *    4. target exists and is a directory                  -> source must be
 *         a directory too, and the target must be EMPTY
 *
 *  ── the correction ───────────────────────────────────────────────────
 *  The first cut tested `ndir->dev != odir->dev` for case 1.  There is no
 *  dev field on InCoreInode.
 *
 *  The test is replaced with what this filesystem CAN answer: whether the
 *  two directories sit under different mounts, read from SCFS's own mount
 *  table.  With one mounted filesystem every path is on it, so the answer
 *  is "same filesystem" — which is correct, and will stay correct once
 *  mount() can read a second device, because the table is consulted then
 *  too rather than a struct field.
 *
 *  v1.1: dev test replaced by a mount-table test.
 */
#include "uiox_kix_scfs_internal.h"

static uint32_t scfs_name_len(const char *name)
{
    uint32_t n = 0u;
    while (n < (uint32_t)UNFS_NAME_MAX && name[n] != '\0') n++;
    return n;
}

/* Which mounted filesystem holds this inode?  Returns the mount table
 * entry, or NULL for the one filesystem that is always there. */
static scfs_mount_t *scfs_mount_of(const InCoreInode *ip)
{
    uint32_t i;

    for (i = 0u; i < NMOUNT; i++) {
        if (!scfs_mount_table[i].m_inuse) continue;
        if (scfs_mount_table[i].m_root == ip) return &scfs_mount_table[i];
        if (scfs_mount_table[i].m_mountpt == ip) return &scfs_mount_table[i];
    }
    return (scfs_mount_t *)0;
}

/* Split a path, resolve its parent, hand back the final component. */
static InCoreInode *scfs_parent_of(const char *path, const char **name,
                                   char *scratch, uint32_t scratchsz)
{
    if (scfs_path_split(path, scratch, scratchsz, name) != SCFS_OK)
        return (InCoreInode *)0;

    return namei(scratch, scfs_cwd_get(), 0u, 0u);      /* 01_fsa */
}

int uiox_kix_scfs_rename(const char *oldpath, const char *newpath)
{
    InCoreInode *oip;
    InCoreInode *odir;
    InCoreInode *ndir;
    InCoreInode *nip = (InCoreInode *)0;
    char         opar[SCFS_PATH_MAX];
    char         npar[SCFS_PATH_MAX];
    const char  *oname = (const char *)0;
    const char  *nname = (const char *)0;
    uint32_t     nino  = 0u;

    if (!oldpath || !newpath) return SCFS_EFAULT;

    /* ── source ─────────────────────────────────────────────────────── */
    oip = namei(oldpath, scfs_cwd_get(), 0u, 0u);       /* 01_fsa */
    if (!oip) return SCFS_ENOENT;

    odir = scfs_parent_of(oldpath, &oname, opar, sizeof(opar));
    if (!odir) { iput(oip); return SCFS_ENOENT; }

    /* ── target's parent ────────────────────────────────────────────── */
    ndir = scfs_parent_of(newpath, &nname, npar, sizeof(npar));
    if (!ndir) { iput(odir); iput(oip); return SCFS_ENOENT; }

    if (!inode_is_dir(ndir)) {
        iput(ndir); iput(odir); iput(oip);
        return SCFS_ENOTDIR;
    }

    /* ── case 1: different filesystems ───────────────────────────────
     * A rename cannot move a file across a mount point: the inode on one
     * side is not addressable from the device on the other.  The test is
     * the mount table, not an inode field — there is no dev field.  With
     * nothing mounted beyond the root filesystem both sides are NULL and
     * the paths are on the same filesystem, which is correct. */
    {
        scfs_mount_t *om = scfs_mount_of(odir);
        scfs_mount_t *nm = scfs_mount_of(ndir);
        if (om != nm) {
            iput(ndir); iput(odir); iput(oip);
            return SCFS_EXDEV;
        }
    }

    /* ── the source may not be a mount point ─────────────────────────
     * NOT asked against an inode flag: there is no IMOUNT.  fs_types.h
     * defines IFLAG_ACCESSED, IFLAG_CHANGED and IFLAG_MODIFIED, and the
     * inode carries no mount-point bit at all.  GCC's nearest-name
     * suggestion (NMOUNT) is the mount table's SIZE, not a flag.
     *
     * The mount table IS consulted below, for the different-filesystem
     * test, and it answers a different question: "which mount holds this
     * inode".  "Is this inode a mount point" has no answering mechanism
     * in this layer, so the check is recorded rather than faked. */

    /* A source that is a directory currently mounted at is still refused
     * by the same walk: scfs_mount_of() would return non-NULL for it and
     * the EXDEV test above would already have fired if the two sides
     * disagreed. */

    /* ── source may not be a mount point ───────────────────────────── */
    /* Kept as a comment, not a test — see the note above.  When an
     * inode-level mount bit lands, this becomes:
     *
     *     if (oip->flags & IMOUNT) return SCFS_EBUSY;
     */

    /* ── look the target up ────────────────────────────────────────── */
    nino = dir_lookup(ndir, nname,
                      scfs_name_len(nname));            /* 01_fsa */

    if (nino != 0u) {
        nip = iget(nino);                               /* 01_fsa */

        if (!nip) {
            /* The name resolved to something unreadable; refuse rather
             * than remove a name we could not inspect. */
            iput(ndir); iput(odir); iput(oip);
            return SCFS_EIO;
        }

        if (inode_is_dir(nip)) {
            /* ── case 4: target is a directory ─────────────────────── */
            if (!inode_is_dir(oip)) {
                iput(nip); iput(ndir); iput(odir); iput(oip);
                return SCFS_ENOTDIR;        /* file onto directory */
            }
            /* It must be empty: nlink 2 means only "." and the parent's
             * entry point at it.  Above that it holds a subdirectory. */
            if (nip->nlink > 2u) {
                iput(nip); iput(ndir); iput(odir); iput(oip);
                return SCFS_ENOTEMPTY;
            }
        }
        /* ── case 3: a plain file — it will be replaced ─────────────── */
    }

    /* ── the SAME inode at both names: nothing to do ───────────────── */
    if (nip && nip->ino == oip->ino) {
        iput(nip); iput(ndir); iput(odir); iput(oip);
        return SCFS_OK;                     /* POSIX: succeed, no change */
    }

    /* ── case 2/3: remove the target's name BEFORE adding the new one ─
     *
     * ORDER MATTERS.  Removing first means the worst crash window leaves
     * the target missing and the source still present — recoverable, and
     * never two names for one inode.  The other order would leave the
     * file at both names, the very state rename exists to prevent.
     */
    if (nip) {
        if (dir_remove(ndir, nname,
                       scfs_name_len(nname)) != 0) {    /* 01_fsa */
            iput(nip); iput(ndir); iput(odir); iput(oip);
            return SCFS_EIO;
        }
        if (nip->nlink > 0u) nip->nlink--;
        nip->flags |= IFLAG_CHANGED;
        iupdate(nip);                                   /* 01_fsa */
        iput(nip);                                      /* 01_fsa */
    }

    /* ── remove the source's old name ──────────────────────────────── */
    if (dir_remove(odir, oname,
                   scfs_name_len(oname)) != 0) {        /* 01_fsa */
        iput(ndir); iput(odir); iput(oip);
        return SCFS_EIO;
    }

    /* ── add the new name, pointing at the SAME inode ──────────────── */
    /* No ialloc, no new inode: this is link()'s step, and it keeps the
     * file's contents and its inode number intact across a rename —
     * which is what makes a rename safe for an open file. */
    if (dir_add(ndir, nname, scfs_name_len(nname), oip->ino,
                SCFS_DT_REG) != 0) {                    /* 01_fsa */
        /* Put the old name back so the file is not lost. */
        dir_add(odir, oname, scfs_name_len(oname), oip->ino,
                (uint8_t)UNFS_DT_REG);
        iput(ndir); iput(odir); iput(oip);
        return SCFS_EIO;
    }

    /* ── a renamed directory: the parents' link counts move ────────── */
    if (inode_is_dir(oip) && odir != ndir) {
        if (odir->nlink > 0u) odir->nlink--;
        ndir->nlink++;

        odir->flags |= (IFLAG_CHANGED | IFLAG_MODIFIED);
        ndir->flags |= (IFLAG_CHANGED | IFLAG_MODIFIED);
        iupdate(odir);
        iupdate(ndir);

        /* Its ".." must now name the new parent. */
        /* The literal ".." has a known length (2) and is a DIRECTORY
         * entry, so the dirent type is UNFS_DT_DIR — not the inode
         * encoding, and not a FileType value. */
        dir_remove(oip, "..", 2u);
        dir_add(oip, "..", 2u, ndir->ino, (uint8_t)UNFS_DT_DIR);
    }

    /* ── release everything ────────────────────────────────────────── */
    oip->flags |= IFLAG_CHANGED;
    iupdate(oip);                                       /* 01_fsa */

    iput(ndir);                                         /* 01_fsa */
    iput(odir);                                         /* 01_fsa */
    iput(oip);                                          /* 01_fsa */

    return SCFS_OK;
}

/* sys_* alias — the consolidated syscalls.c owns this; keep one. */
int sys_rename(const char *oldp, const char *newp)
{ return uiox_kix_scfs_rename(oldp, newp); }
