/*
 *  30_KIX/32_FS/10_scfs/src/uiox_kix_scfs_mknod.c
 *
 *  SCFS - Algorithm make new node (mknod).  CORRECTED.
 *
 *  -- Bach's algorithm, verbatim -------------------------------------
 *  inputs: node (file name), file type, permission,
 *          major, minor device number (for block/char special files)
 *  output: none
 *  {
 *    if (new node not named pipe and user not super user)
 *        return (error);
 *    get inode of parent of new node (algorithm namei);
 *    if (new node already exists)
 *    {
 *        release parent inode (algorithm iput);
 *        return (error);
 *    }
 *    assign free inode from file system for new node (algorithm ialloc);
 *    create new directory entry in parent directory: include new node name
 *        and newly assigned inode number;
 *    release parent directory inode (algorithm iput);
 *    if (new node is block or character special file)
 *        write major, minor numbers into inode structure;
 *    release new node inode (algorithm iput);
 *  }
 *
 *  -- the device-number gap -----------------------------------------
 *  The first cut wrote ip->i_major and ip->i_minor.  Neither exists.
 *  InCoreInode carries no device-number fields and unfs_inode_t does not
 *  either, so Bach's step 7 HAS NOWHERE TO WRITE.  A device node created
 *  here would have no way to name its driver.
 *
 *  Two consequences, both stated rather than hidden:
 *
 *    - a CHAR or BLOCK node is REFUSED, because the numbers that make it
 *      useful cannot be stored.  Creating one would produce a node that
 *      resolves and reads nothing.
 *
 *    - a FIFO has no device numbers to store, so it is created normally
 *      and works.  A DIRECTORY is refused: that is mkdir's job, because
 *      mkdir also writes '.' and '..'.
 *
 *  -- CHANGED: the dirent calls, and one missing helper --------------
 *  dir_lookup() and dir_add() take an explicit name LENGTH now, because
 *  UNFS dirents are variable-length and a name is not NUL-terminated on
 *  disk:
 *
 *      dir_lookup(dir, name, len)
 *      dir_add   (dir, name, len, ino, type)
 *
 *  The length comes from scfs_name_len() below.  The previous call used
 *  scfs_strlen(), which is NOT declared anywhere in this layer:
 *
 *      mknod.c:116: implicit declaration of function 'scfs_strlen'
 *
 *  @version 1.2.0  @date 2026-09-26
 */
#include "uiox_kix_scfs_internal.h"

/* -- the length of one path component, in bytes ------------------------
 * Local, and bounded: a name read off disk is not NUL-terminated, so a
 * scan with no bound would walk off the end of the buffer.
 * -------------------------------------------------------------------- */
static uint32_t scfs_name_len(const char *name)
{
    uint32_t n = 0u;

    while (n < (uint32_t)UNFS_NAME_MAX && name[n] != '\0') n++;
    return n;
}

int uiox_kix_scfs_mknod(const char *path, uint16_t mode,
                        uint8_t major, uint8_t minor)
{
    InCoreInode *dir;
    InCoreInode *ip;
    uint16_t     unfs_if;
    uint16_t     type;
    char         parent[SCFS_PATH_MAX];
    const char  *name = (const char *)0;
    uint32_t     nlen;
    int          rc;

    (void)major; (void)minor;   /* no inode field to hold them - see above */

    if (!path) return SCFS_EFAULT;

    type = (uint16_t)(mode & SCFS_S_IFMT);

    /* -- 1. the privilege rule, and the device-number gap ------------- */
    if (type == SCFS_S_IFCHR || type == SCFS_S_IFBLK) {
        /* Bach writes the major and minor numbers into the inode here.
         * There is no such field, so the node cannot be made to name its
         * driver.  ENOSYS rather than a node that resolves to nothing. */
        return SCFS_ENOSYS;
    }

    if (type != SCFS_S_IFIFO && type != SCFS_S_IFDIR) {
        /* Anything else is not a node Bach's mknod creates: a regular
         * file is open(O_CREAT)'s job, a symlink has its own call. */
        return SCFS_EINVAL;
    }

    if (type == SCFS_S_IFDIR) {
        /* A directory is mkdir's job; mknod may make one, but only with
         * the '.' and '..' entries mkdir writes - which this path would
         * skip.  Refuse rather than make a directory with no parent link. */
        return SCFS_EINVAL;
    }

    /* -- 2. the parent directory (algorithm namei) -------------------- */
    rc = scfs_path_split(path, parent, sizeof(parent), &name);
    if (rc != SCFS_OK) return rc;

    nlen = scfs_name_len(name);
    if (nlen == 0u) return SCFS_EINVAL;

    dir = namei(parent, scfs_cwd_get(), 0u, 0u);        /* 01_fsa */
    if (!dir) return SCFS_ENOENT;
    if (!inode_is_dir(dir)) { iput(dir); return SCFS_ENOTDIR; }
    if (!inode_access_ok(dir, 0u, 0u, 0, 1, 0)) { iput(dir); return SCFS_EACCES; }

    /* -- 3. the node must not already exist --------------------------- */
    if (dir_lookup(dir, name, nlen) != 0u) {            /* 01_fsa */
        iput(dir);
        return SCFS_EEXIST;
    }

    /* -- 4. assign a free inode (algorithm ialloc) -------------------- */
    /* A FIFO is the only type this path reaches with the checks above.
     * The value is the ON-DISK encoding (UNFS_IF*), not the old FileType
     * nibble - (FT_FIFO << 12) is 0x5000, which is not a valid type. */
    unfs_if = (uint16_t)UNFS_IFIFO;

    ip = ialloc(unfs_if, scfs_apply_umask((uint16_t)(mode & 0777u)), 0u, 0u);
    if (!ip) { iput(dir); return SCFS_ENOSPC; }

    ip->nlink = 1;

    /* -- 5. create the directory entry (algorithm dir_add) ------------ */
    /* The type is the DIRENT encoding (UNFS_DT_*), not the inode one -
     * d_type and i_mode are unrelated numeric systems. */
    if (dir_add(dir, name, nlen, ip->ino,
                (uint8_t)UNFS_DT_FIFO) != 0) {          /* 01_fsa */
        iput(ip);
        iput(dir);
        return SCFS_EIO;
    }

    /* -- 6. release the parent (algorithm iput) ----------------------- */
    iput(dir);                                          /* 01_fsa */

    /* -- 7. the device numbers - SKIPPED, no field --------------------
     * Bach writes major and minor here for a special file.  The only
     * type that reaches this point is a FIFO, which has no device
     * numbers.  See the header note. */

    /* A node has no data blocks; the ialloc'd inode is already empty. */
    ip->size = 0;
    ip->flags |= IFLAG_CHANGED;
    iupdate(ip);                                        /* 01_fsa */

    /* -- 8. release the new node (algorithm iput) --------------------- */
    iput(ip);                                           /* 01_fsa */

    return SCFS_OK;
}

/* sys_* alias - the consolidated syscalls.c owns this; keep one. */
int sys_mknod(const char *path, uint16_t mode, uint8_t major, uint8_t minor)
{ return uiox_kix_scfs_mknod(path, mode, major, minor); }
