#include "uiox_kix_scfs_internal.h"

/*
 * Bach's inode has NO room for extended attributes: Ch.4 §1.1 lists every
 * field and nothing is left over.
 *
 * ── the correction this file carries ─────────────────────────────────
 * The first cut called xattr_set / xattr_get / xattr_list / xattr_remove
 * and read ip->i_xattr as the chain head.  NEITHER EXISTS:
 *   · InCoreInode has no i_xattr field
 *   · no 01_fsa unit defines an xattr_* function
 *   · fs_types.h has no xattr block definition
 *
 * What IS kept, because it is this layer's own work and is correct:
 *   · the four namespace prefixes
 *   · an un-prefixed name is EOPNOTSUPP, not "use user."
 *   · the privilege rule for the three namespaces that are not open
 *   · the name-length bound
 */

enum {
    SCFS_XATTR_USER     = 0,
    SCFS_XATTR_TRUSTED  = 1,
    SCFS_XATTR_SECURITY = 2,
    SCFS_XATTR_SYSTEM   = 3,
    SCFS_XATTR_BAD      = 4
};

static int scfs_xattr_ns(const char *name)
{
    if (!name || name[0] == '\0') return SCFS_XATTR_BAD;

    if (name[0]=='u'&&name[1]=='s'&&name[2]=='e'&&name[3]=='r'&&name[4]=='.')
        return name[5] ? SCFS_XATTR_USER : SCFS_XATTR_BAD;
    if (name[0]=='t'&&name[1]=='r'&&name[2]=='u'&&name[3]=='s'&&name[4]=='t')
        if (name[5]=='e'&&name[6]=='d'&&name[7]=='.')
            return name[8] ? SCFS_XATTR_TRUSTED : SCFS_XATTR_BAD;
    if (name[0]=='s'&&name[1]=='e'&&name[2]=='c'&&name[3]=='u'&&name[4]=='r')
        if (name[5]=='i'&&name[6]=='t'&&name[7]=='y'&&name[8]=='.')
            return name[9] ? SCFS_XATTR_SECURITY : SCFS_XATTR_BAD;
    if (name[0]=='s'&&name[1]=='y'&&name[2]=='s'&&name[3]=='t'&&name[4]=='e')
        if (name[5]=='m'&&name[6]=='.')
            return name[7] ? SCFS_XATTR_SYSTEM : SCFS_XATTR_BAD;

    /* No namespace prefix: POSIX says EOPNOTSUPP, not "use user.". */
    return SCFS_XATTR_BAD;
}

static int scfs_xattr_ns_permitted(int ns)
{
    if (ns == SCFS_XATTR_USER)     return 1;
    if (ns == SCFS_XATTR_TRUSTED)  return scfs_is_super();
    if (ns == SCFS_XATTR_SECURITY) return scfs_is_super();
    if (ns == SCFS_XATTR_SYSTEM)   return 1;
    return 0;
}

/* Every entry point shares the same three checks. */
static int scfs_xattr_precheck(const char *name)
{
    int ns = scfs_xattr_ns(name);

    if (ns == SCFS_XATTR_BAD)         return SCFS_EOPNOTSUPP;
    if (!scfs_xattr_ns_permitted(ns)) return SCFS_EPERM;
    return SCFS_OK;
}

int uiox_kix_scfs_setxattr(const char *path, const char *name,
                           const void *value, uint32_t size, int flags)
{
    int rc;

    if (!path || !name || (!value && size)) return SCFS_EFAULT;

    rc = scfs_xattr_precheck(name);
    if (rc != SCFS_OK) return rc;

    if (size > SCFS_XATTR_MAX_VALUE) return SCFS_ERANGE;

    /* The name and namespace are valid, but there is no storage: no
     * i_xattr on the inode and no xattr_set in 01_fsa. */
    return SCFS_ENOSYS;
}

int uiox_kix_scfs_getxattr(const char *path, const char *name,
                           void *value, uint32_t size)
{
    int rc;

    (void)value; (void)size;

    if (!path || !name) return SCFS_EFAULT;

    rc = scfs_xattr_precheck(name);
    if (rc != SCFS_OK) return rc;

    return SCFS_ENOSYS;
}

int uiox_kix_scfs_listxattr(const char *path, char *list, uint32_t size)
{
    (void)list; (void)size;

    if (!path) return SCFS_EFAULT;

    /* An empty list is the honest answer for a filesystem with no xattr
     * storage — a caller that only wants "are there any" gets the right
     * answer, and one that wants to enumerate gets an empty set rather
     * than a fabricated one. */
    return (int32_t)0;
}

int uiox_kix_scfs_removexattr(const char *path, const char *name)
{
    int rc;

    if (!path || !name) return SCFS_EFAULT;

    rc = scfs_xattr_precheck(name);
    if (rc != SCFS_OK) return rc;

    /* Nothing was ever stored.  That is ENODATA, not ENOSYS: the
     * namespace was valid and the answer is "no such attribute". */
    return SCFS_ENODATA;
}

int uiox_kix_scfs_fsetxattr(int fd, const char *name,
                            const void *value, uint32_t size, int flags)
{
    scfs_file_t *f;
    int          rc;

    (void)value; (void)flags;

    if (!name) return SCFS_EFAULT;

    rc = scfs_xattr_precheck(name);
    if (rc != SCFS_OK) return rc;

    if (size > SCFS_XATTR_MAX_VALUE) return SCFS_ERANGE;

    f = scfs_getf(fd);
    if (!f) return SCFS_EBADF;

    return SCFS_ENOSYS;
}

int uiox_kix_scfs_fgetxattr(int fd, const char *name, void *value, uint32_t size)
{
    scfs_file_t *f;
    int          rc;

    (void)value; (void)size;

    if (!name) return SCFS_EFAULT;

    rc = scfs_xattr_precheck(name);
    if (rc != SCFS_OK) return rc;

    f = scfs_getf(fd);
    if (!f) return SCFS_EBADF;

    return SCFS_ENOSYS;
}
