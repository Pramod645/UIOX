/*
 *  30_KIX/32FileSystem/10_scfs/src/uiox_kix_scfs_mknod.c
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
 *  -- the device-number gap, and how it closed ------------------------
 *  v1.2 wrote ip->i_major and ip->i_minor.  Neither exists, so Bach's
 *  step 7 had nowhere to write and a CHAR or BLOCK node was REFUSED with
 *  ENOSYS: creating one would have produced a node that resolves and
 *  reaches no driver.
 *
 *  v1.3 supplies the fields.  InCoreInode now carries
 *
 *      uint16_t idev_class;   which subsystem owns the node
 *      uint16_t idev_unit;    which instance of that class
 *
 *  which REPLACE the major/minor pair.  uiox_devclass_t already packs a
 *  class's family into the high nibble and the class into the low 12
 *  bits, so one 16-bit field names the subsystem with nothing for anyone
 *  to allocate and keep unique — and that same (cls, unit) pair is the
 *  seed of uiox_dev_dev_t and of the binding registry ioctl() resolves
 *  through.  See 34_DSS/include/uiox_devclass.h.
 *
 *  Where the pair comes from: the NAME, not the arguments.  mknod's own
 *  (major, minor) are two 8-bit values and cannot hold a class — CHG
 *  alone is 0x3003 — so uiox_dev_parse() reads the node component the way
 *  a user writes it.  Backwards compatibility is not a concern: the
 *  arguments were never storable, so no caller could have relied on them.
 *
 *      /dev/chg0   -> UIOX_DEVCLASS_CHG, unit 0
 *      /dev/kbd2   -> UIOX_DEVCLASS_KBD, unit 2
 *
 *  A name that names no class is refused with ENOENT.  A node that
 *  resolves but reaches no driver is worse than no node: it opens, it
 *  reads nothing, and the failure surfaces far from its cause.
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
 *  -- CHANGED in this revision ----------------------------------------
 *  scfs_create_node() tested the mode with a renamed macro; see
 *  scfs_table.c.  Not this file.
 *
 *  @version 1.3.0  @date 2026-10-08
 */
#include "uiox_kix_scfs_internal.h"

/* The device vocabulary and uiox_dev_parse(). */
#include "uiox_devclass.h"

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

/* -- the dirent type for an inode type ---------------------------------
 * d_type and i_mode are unrelated numeric systems: the inode wants
 * UNFS_IF*, the directory entry wants UNFS_DT_*.  Map rather than cast.
 * -------------------------------------------------------------------- */
static uint8_t scfs_dtype_of(uint16_t type)
{
    if (type == SCFS_S_IFCHR) return (uint8_t)UNFS_DT_CHR;
    if (type == SCFS_S_IFBLK) return (uint8_t)UNFS_DT_BLK;
    return (uint8_t)UNFS_DT_FIFO;
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

    uiox_devclass_t cls  = UIOX_DEVCLASS_NONE;
    uint16_t        unit = 0u;
    int             is_dev;

    /* The two arguments Bach passes are no longer read: a class needs 16
     * bits and they carry 8 each.  The name supplies the pair instead. */
    (void)major; (void)minor;

    if (!path) return SCFS_EFAULT;

    type = (uint16_t)(mode & SCFS_S_IFMT);

    /* -- 1. which node types this call creates ----------------------- */
    is_dev = (type == SCFS_S_IFCHR || type == SCFS_S_IFBLK);

    if (!is_dev && type != SCFS_S_IFIFO) {
        /* A regular file is open(O_CREAT)'s job, a symlink its own call,
         * a directory mkdir's — mknod may make one, but only with the '.'
         * and '..' entries mkdir writes, which this path would skip. */
        return SCFS_EINVAL;
    }

    /* -- 2. the parent directory (algorithm namei) -------------------- */
    rc = scfs_path_split(path, parent, sizeof(parent), &name);
    if (rc != SCFS_OK) return rc;

    nlen = scfs_name_len(name);
    if (nlen == 0u) return SCFS_EINVAL;

    /* A special file must name the subsystem it belongs to BEFORE the
     * inode is allocated: refusing here costs nothing, whereas refusing
     * after ialloc would mean freeing an inode that was just written. */
    if (is_dev) {
        rc = uiox_dev_parse(name, nlen, &cls, &unit);
        if (rc == -ENOENT) return SCFS_ENODEV;   /* names no known class */
        if (rc == -ERANGE) return SCFS_EINVAL;   /* index out of range   */
        if (rc != 0)       return SCFS_EINVAL;
    }

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
    /* The ON-DISK encoding (UNFS_IF*), not the old FileType nibble —
     * (FT_FIFO << 12) is 0x5000, which is not a valid type. */
    unfs_if = is_dev ? (uint16_t)((type == SCFS_S_IFCHR) ? UNFS_IFCHR
                                                         : UNFS_IFBLK)
                     : (uint16_t)UNFS_IFIFO;

    ip = ialloc(unfs_if, scfs_apply_umask((uint16_t)(mode & 0777u)), 0u, 0u);
    if (!ip) { iput(dir); return SCFS_ENOSPC; }

    ip->nlink = 1;

    /* -- 5. create the directory entry (algorithm dir_add) ------------ */
    /* The DIRENT encoding — d_type and i_mode are unrelated systems. */
    if (dir_add(dir, name, nlen, ip->ino,
                scfs_dtype_of(type)) != 0) {            /* 01_fsa */
        iput(ip);
        iput(dir);
        return SCFS_EIO;
    }

    /* -- 6. release the parent (algorithm iput) ----------------------- */
    iput(dir);                                          /* 01_fsa */

    /* -- 7. the device identity (Bach: major, minor) ------------------ */
    if (is_dev) {
        /* Bach writes the two numbers here.  The class and unit take
         * their place: together they are the key uiox_dev_lookup() is
         * indexed by, which is what lets ioctl() on this node reach the
         * driver.  A plain file or a FIFO has no subsystem behind it, so
         * both fields keep their zeroed value — UIOX_DEVCLASS_NONE for
         * the class — and ioctl() answers ENOTTY on the mode alone. */
        ip->idev_class = (uint16_t)cls;
        ip->idev_unit  = unit;
        ip->flags |= IFLAG_CHANGED;
    }

    /* A node has no data blocks; the ialloc'd inode is already empty. */
    ip->size = 0;
    ip->flags |= IFLAG_CHANGED;
    iupdate(ip);                                        /* 01_fsa */

    /* -- 8. release the new node (algorithm iput) --------------------- */
    iput(ip);                                           /* 01_fsa */

    return SCFS_OK;
}

/* sys_* alias - the consolidated syscalls.c owns this; keep one.  The
 * (major, minor) pair is still in the signature because the dispatch
 * table's SYS_MKNOD entry passes three arguments; it is ignored. */
int sys_mknod(const char *path, uint16_t mode, uint8_t major, uint8_t minor)
{ return uiox_kix_scfs_mknod(path, mode, major, minor); }
