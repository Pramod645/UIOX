#include "uiox_kix_scfs_internal.h"

/*
 * Bach has no rename algorithm: V7 offered link() then unlink(), which is
 * visibly non-atomic — a crash between the two leaves the file at BOTH
 * names.  rename(2) exists to make the pair atomic.
 *
 * ── the correction this file carries ─────────────────────────────────
 * The first cut tested `ndir->dev != odir->dev` for the cross-filesystem
 * case.  There is no dev field on InCoreInode.  The test is replaced with
 * what this filesystem CAN answer: whether the two directories sit under
 * different mounts, read from SCFS's own mount table.
 */
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

static InCoreInode *scfs_parent_of(const char *path, const char **name,
                                   char *scratch, uint32_t scratchsz)
{
    if (scfs_path_split(path, scratch, scratchsz, name) != SCFS_OK)
        return (InCoreInode *)0;

    return namei(scratch, scfs_cwd_get(), scfs_uid_get(), scfs_gid_get());
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

    oip = namei(oldpath, scfs_cwd_get(), scfs_uid_get(), scfs_gid_get());
    if (!oip) return SCFS_ENOENT;

    odir = scfs_parent_of(oldpath, &oname, opar, sizeof(opar));
    if (!odir) { iput(oip); return SCFS_ENOENT; }

    ndir = scfs_parent_of(newpath, &nname, npar, sizeof(npar));
    if (!ndir) { iput(odir); iput(oip); return SCFS_ENOENT; }

    if (!SCFS_IS_DIR(ndir->mode)) {
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

    if (oip->flags & IMOUNT) {
        iput(ndir); iput(odir); iput(oip);
        return SCFS_EBUSY;
    }

    nino = dir_lookup(ndir, nname);                     /* 01_fsa */

    if (nino != 0u) {
        nip = iget(nino);                               /* 01_fsa */

        if (!nip) {
            iput(ndir); iput(odir); iput(oip);
            return SCFS_EIO;
        }

        if (SCFS_IS_DIR(nip->mode)) {
            /* ── case 4: target is a directory ─────────────────────── */
            if (!SCFS_IS_DIR(oip->mode)) {
                iput(nip); iput(ndir); iput(odir); iput(oip);
                return SCFS_ENOTDIR;        /* file onto directory */
            }
            /* It must be empty: nlink 2 means only "." and the parent's
             * entry point at it. */
            if (nip->nlink > 2u) {
                iput(nip); iput(ndir); iput(odir); iput(oip);
                return SCFS_ENOTEMPTY;
            }
        }
    }

    /* ── the SAME inode at both names: nothing to do ───────────────── */
    if (nip && nip->ino == oip->ino) {
        iput(nip); iput(ndir); iput(odir); iput(oip);
        return SCFS_OK;                     /* POSIX: succeed, no change */
    }

    /* ── case 2/3: remove the target's name BEFORE adding the new one ─
     * ORDER MATTERS.  Removing first means the worst crash window leaves
     * the target missing and the source still present — recoverable, and
     * never two names for one inode. */
    if (nip) {
        if (dir_remove(ndir, nname) != 0) {
            iput(nip); iput(ndir); iput(odir); iput(oip);
            return SCFS_EIO;
        }
        if (nip->nlink > 0u) nip->nlink--;
        nip->flags |= IFLAG_CHANGED;
        iupdate(nip);
        iput(nip);
    }

    if (dir_remove(odir, oname) != 0) {
        iput(ndir); iput(odir); iput(oip);
        return SCFS_EIO;
    }

    /* No ialloc, no new inode: this is link()'s step, and it keeps the
     * file's contents and its inode number intact across a rename. */
    if (dir_add(ndir, nname, oip->ino) != 0) {
        dir_add(odir, oname, oip->ino);     /* put the old name back */
        iput(ndir); iput(odir); iput(oip);
        return SCFS_EIO;
    }

    /* ── a renamed directory: the parents' link counts move ────────── */
    if (SCFS_IS_DIR(oip->mode) && odir != ndir) {
        if (odir->nlink > 0u) odir->nlink--;
        ndir->nlink++;

        odir->flags |= (IFLAG_CHANGED | IFLAG_MODIFIED);
        ndir->flags |= (IFLAG_CHANGED | IFLAG_MODIFIED);
        iupdate(odir);
        iupdate(ndir);

        /* Its ".." must now name the new parent. */
        dir_remove(oip, "..");
        dir_add(oip, "..", ndir->ino);
    }

    oip->flags |= IFLAG_CHANGED;
    iupdate(oip);

    iput(ndir);
    iput(odir);
    iput(oip);
    return SCFS_OK;
}
