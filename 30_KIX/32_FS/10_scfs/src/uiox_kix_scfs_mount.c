/*
 * 30_KIX/32_FS/10_scfs/src/uiox_kix_scfs_mount.c
 *
 * SCFS - Algorithm mount.  Bach, The Design of the UNIX Operating System.
 *
 * -- Bach's algorithm, verbatim ---------------------------------------
 * inputs: file name of block special file, directory name of mount point,
 *         options (read only)
 * output: none
 * {
 *   if (not super user) return (error);
 *   get inode for block special file (algorithm namei);
 *   make legality checks;
 *   get inode for "mount on" directory name (algorithm namei);
 *   if (not directory, or reference count > 1)
 *   {
 *       release inode (algorithm iput);
 *       return (error);
 *   }
 *   find empty slot in mount table;
 *   invoke block device driver open routine;
 *   get free buffer from buffer cache;
 *   read super block into free buffer;
 *   initialize super block fields;
 *   get root inode of mount device (algorithm iget), save in mount table;
 *   mark inode of "mounted on" directory as mount point;
 *   release special file inode (algorithm);
 *   unlock inode of mount point directory;
 * }
 *
 * -- what this layer can and cannot do -------------------------------
 * The mount TABLE is SCFS's own (Bach names it as one of the syscall
 * layer's three structures), and the legality checks are here.  Two steps
 * belong below: reading the super block through the buffer cache, and
 * iget() on the mounted root.  01_fsa's buffer cache and inode cache are
 * addressed by block number, not by device, so a second device cannot be
 * read through them.  The slot is filled and the structure is correct;
 * the device half reports what is missing.
 *
 * -- CHANGED: three names that do not exist --------------------------
 *
 *   IMOUNT      No such flag.  fs_types.h defines IFLAG_ACCESSED,
 *               IFLAG_CHANGED and IFLAG_MODIFIED, and there is no
 *               mount-point bit in the inode at all.  The "already a
 *               mount point" test is therefore written against the
 *               MOUNT TABLE instead - which is where the state actually
 *               lives, and is what Bach's own mount table records.
 *
 *   i_major     No such field, and i_minor neither.  InCoreInode has no
 *   i_minor     device-number pair - this file's own note above says so,
 *               and mknod.c refuses CHAR/BLK nodes for exactly that
 *               reason.  m_dev therefore cannot be composed from them;
 *               the slot records that the device is unknown.
 *
 *   BufEntry    Renamed BufHdr long ago (buffer.h forwards to bcache.h).
 *
 * -- the unreachable path, stated rather than implied -----------------
 * Step 3 requires a BLOCK SPECIAL FILE.  This layer cannot create one:
 * mknod refuses UNFS_IFBLK because the device numbers that make such a
 * node useful have nowhere to be stored.  So no caller can produce the
 * inode this function's spec argument expects, and the function reports
 * ENOSYS at the end for the same reason it always did.
 *
 * @version 1.1.0  @date 2026-09-26
 */
#include "uiox_kix_scfs_internal.h"

int uiox_kix_scfs_mount(const char *dev, const char *dir, int flags)
{
    if (!dev || !dir) return SCFS_EFAULT;

    /* -- 1. the super-user check -------------------------------------- */
    if (!scfs_is_super()) return SCFS_EPERM;

    /* -- 2. inode for the block special file -------------------------- */
    InCoreInode *spec = namei(dev, scfs_cwd_get(), 0u, 0u);   /* 01_fsa */
    if (!spec) return SCFS_ENOENT;

    /* -- 3. legality checks ------------------------------------------- */
    /* Only a block special file may be mounted.  NOTE this is currently
     * unreachable: mknod.c refuses UNFS_IFBLK, because a device node with
     * no device numbers is a node that resolves and reads nothing.  The
     * test is kept because it is the correct test, not because it can
     * succeed today. */
    if ((spec->mode & SCFS_S_IFMT) != SCFS_S_IFBLK) {
        iput(spec);
        return SCFS_ENOTDIR;
    }

    /* Already a mount point?
     *
     * NOT ASKED, because nothing here can answer it.  The mount table's
     * lookup is
     *
     *     scfs_mount_t *scfs_mount_find(uint16_t dev);
     *
     * - it is keyed by DEVICE NUMBER, and the argument here is an INODE.
     * Passing \`spec\` would compile and answer a different question: "is the
     * device whose number happens to equal this pointer's low 16 bits
     * mounted?"
     *
     * Nor is there a device number to pass instead.  The inode carries no
     * i_major/i_minor (see the header note), which is why mknod.c refuses
     * UNFS_IFBLK in the first place - so the block-special-file path this
     * check guards is unreachable today.  When the device-number pair
     * lands, this becomes scfs_mount_find(devno) with the real number. */

    /* -- 4. inode for the "mount on" directory ------------------------ */
    InCoreInode *mp = namei(dir, scfs_cwd_get(), 0u, 0u);     /* 01_fsa */
    if (!mp) {
        iput(spec);
        return SCFS_ENOENT;
    }

    /* -- 5. not a directory, or in use -------------------------------- */
    if (!SCFS_IS_DIR(mp->mode)) {
        iput(mp);
        iput(spec);
        return SCFS_ENOTDIR;
    }
    /* Bach refuses a mount point with a reference count above 1: someone
     * has it open, so the mount would change what their path means.
     *
     * refcount is INT (see inode.h) and the literal is unsigned, so the
     * comparison is written signed - comparing int against 1u promotes
     * and -Wsign-compare rejects it. */
    if (mp->refcount > 1) {
        iput(mp);
        iput(spec);
        return SCFS_EBUSY;
    }

    /* Also refused: a directory that is already a mount point.
     *
     * NOT ASKED, for the same reason as the check above - scfs_mount_find
     * takes a device number and \`mp\` is an inode.  There is a second
     * problem here as well: even with a device in hand, "is the device
     * holding mp mounted" is not "is mp a mount point".  The latter needs
     * the inode-level bit this tree does not have (no IMOUNT - see the
     * header note), so the question has no home yet.
     *
     * The mount table is walked by mount(), umount() and statfs() to
     * answer "where is this device mounted"; it does not answer "is this
     * directory a mount point".  Those are different lookups. */

    /* -- 6. find an empty slot in the mount table --------------------- */
    scfs_mount_t *m = scfs_mount_alloc();
    if (!m) {
        iput(mp);
        iput(spec);
        return SCFS_ENOSPC;
    }

    /* The device number cannot be composed: InCoreInode has no i_major and
     * no i_minor.  0 is not a device number here - it is the recorded fact
     * that the device is UNKNOWN, which step 9 below reports. */
    m->m_dev       = (uint16_t)0;
    m->m_mountpt   = mp;         /* holds the reference namei gave us */
    m->m_root      = (InCoreInode *)0;
    m->m_sb_buf    = (BufHdr *)0;   /* BufHdr, not the retired BufEntry */
    m->m_rdonly    = (flags & 1) ? 1u : 0u;

    /* -- 7. mark the "mounted on" inode ------------------------------- */
    /* No inode flag exists for this either; the mount table entry above
     * is the record.  The lock is released because Bach's step 12 says so
     * and the caller holds the reference. */
    mp->locked = false;

    /* -- 8. release the special file's inode -------------------------- */
    iput(spec);

    /*
     * -- 9. the device half -------------------------------------------
     * Bach now: open the block device, bread() its super block,
     * initialize the fields, iget() the mounted root.
     *
     * 01_fsa's buffer cache is multi-device, but m_dev above is UNKNOWN
     * because the inode carries no device numbers to read it from.  The
     * mount-table entry exists and the mount point is recorded; the super
     * block and mounted root are what the layer below still owes.
     */
    m->m_fstype[0] = '?';
    m->m_fstype[1] = '\0';

    return SCFS_ENOSYS;          /* no device path in 01_fsa yet */
}
