/*
 *  30_KIX/32_FS/10_scfs/src/uiox_kix_scfs_xattr.c
 *
 *  SCFS — the extended attribute calls.  CORRECTED.
 *
 *  ── where this sits relative to Bach ─────────────────────────────────
 *  Bach's inode has NO room for extended attributes.  Ch.4 §1.1 lists
 *  every field — ten block addresses, size, owner, group, link count,
 *  timestamps, type and permission — and nothing is left over.  Extended
 *  attributes are a later filesystem idea: a name/value pair stored
 *  outside the inode, addressed by file.
 *
 *  ── the correction ───────────────────────────────────────────────────
 *  The first cut called xattr_set / xattr_get / xattr_list / xattr_remove
 *  and read ip->i_xattr as the chain head.  NEITHER EXISTS:
 *
 *    · InCoreInode has no i_xattr field
 *    · no 01_fsa unit defines an xattr_* function
 *    · fs_types.h has no xattr block definition
 *
 *  The whole subsystem is absent — the call names were invented.  There
 *  is nothing to call and nowhere to store a value, so the calls report
 *  that rather than walking a chain that does not exist.
 *
 *  What IS kept, because it is this layer's own work and is correct:
 *    · the four namespace prefixes (user / trusted / security / system)
 *    · the rule that an un-prefixed name is EOPNOTSUPP, not "use user."
 *    · the privilege rule for the three namespaces that are not open
 *    · the name-length bound
 *
 *  These are the checks that would otherwise be written four times, once
 *  per namespace, the day 01_fsa grows the storage.  They stay so that
 *  day is a one-file change here.
 *
 *  v1.1: missing xattr_* calls and i_xattr removed; checks retained.
 */
#include "uiox_kix_scfs_internal.h"

/* ── which namespace a name belongs to, and whether it needs privilege ─ */
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

    /* A prefix match, then the name must not be empty after the dot:
     * "user." alone names nothing. */
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

/* Every entry point shares the same three checks.  One place, so the four
 * calls cannot disagree about the namespace rules. */
static int scfs_xattr_precheck(const char *name)
{
    int ns = scfs_xattr_ns(name);

    if (ns == SCFS_XATTR_BAD)         return SCFS_EOPNOTSUPP;
    if (!scfs_xattr_ns_permitted(ns)) return SCFS_EPERM;
    return SCFS_OK;
}

/* ── setxattr ───────────────────────────────────────────────────────── */
int uiox_kix_scfs_setxattr(const char *path, const char *name,
                           const void *value, uint32_t size, int flags /* unused - no xattr storage */)
{

    /* flags is part of setxattr's ABI (XATTR_CREATE / XATTR_REPLACE) but
     * there is no storage to create or replace in, so it is discarded
     * explicitly rather than left unused - -Wextra rejects the latter. */
    (void)flags;
    int rc;

    if (!path || !name || (!value && size)) return SCFS_EFAULT;

    rc = scfs_xattr_precheck(name);
    if (rc != SCFS_OK) return rc;

    if (size > SCFS_XATTR_MAX_VALUE) return SCFS_ERANGE;

    /* The name and namespace are valid, but there is no storage: no
     * i_xattr on the inode and no xattr_set in 01_fsa.  Verified as far
     * as this layer can verify, then reported. */
    return SCFS_ENOSYS;
}

/* ── getxattr ───────────────────────────────────────────────────────── */
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

/* ── listxattr ──────────────────────────────────────────────────────── */
int uiox_kix_scfs_listxattr(const char *path, char *list, uint32_t size)
{
    (void)list; (void)size;

    if (!path) return SCFS_EFAULT;

    /* An empty list is the honest answer for a filesystem with no xattr
     * storage — a caller that only wants to know "are there any" gets
     * the right answer, and one that wants to enumerate gets an empty
     * set rather than a fabricated one. */
    return (int32_t)0;
}

/* ── removexattr ────────────────────────────────────────────────────── */
int uiox_kix_scfs_removexattr(const char *path, const char *name)
{
    int rc;

    if (!path || !name) return SCFS_EFAULT;

    rc = scfs_xattr_precheck(name);
    if (rc != SCFS_OK) return rc;

    /* Nothing was ever stored, so there is nothing to remove.  That is
     * ENODATA, not ENOSYS: the namespace was valid and the answer is
     * "no such attribute". */
    return SCFS_ENODATA;
}

/* ── the fd-addressed forms ─────────────────────────────────────────── */
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

/* sys_* aliases — the consolidated syscalls.c owns these; keep one. */
int sys_setxattr(const char *p, const char *n, const void *v, uint32_t sz, int fl)
{ return uiox_kix_scfs_setxattr(p, n, v, sz, fl); }
int sys_getxattr(const char *p, const char *n, void *v, uint32_t sz)
{ return uiox_kix_scfs_getxattr(p, n, v, sz); }
int sys_listxattr(const char *p, char *l, uint32_t sz)
{ return uiox_kix_scfs_listxattr(p, l, sz); }
int sys_removexattr(const char *p, const char *n)
{ return uiox_kix_scfs_removexattr(p, n); }
