#include "uiox_kix_scfs_internal.h"

/*
 * ── the correction this file carries ─────────────────────────────────
 * The first cut claimed a super-user bypass around inode_access_ok().
 * inode.c's inode_access_ok() tests owner / group / other and nothing
 * else — there is NO uid-0 bypass in it.  So the bypass made access()
 * disagree with open(): access() would say yes and open() would fail
 * with EACCES.  That is the worst failure access() can have, since
 * predicting open() is its entire job.
 *
 * The bypass is REMOVED.  If one is wanted it belongs in
 * inode_access_ok() in 01_fsa, where open(), chdir() and every other
 * caller would get it too — one place, one answer.
 */
static uint16_t s_uid = 0u;
static uint16_t s_gid = 0u;

void     scfs_cred_set(uint16_t uid, uint16_t gid) { s_uid = uid; s_gid = gid; }
uint16_t scfs_uid_get (void) { return s_uid; }
uint16_t scfs_gid_get (void) { return s_gid; }

/*
 * scfs_is_super — "is the current credential the super user?"
 *
 * Answers a question about the CALLER.  It does NOT grant a permission
 * bypass — inode_access_ok() decides access, and it has none.  Functions
 * that need privilege (chroot, mount, the trusted xattr namespace) test
 * this and refuse.
 */
int scfs_is_super(void) { return (s_uid == 0u) ? 1 : 0; }

/* ── the shared permission test — no bypass, no special cases ───────── */
int scfs_perm_test(InCoreInode *ip, int mode)
{
    if (!ip) return 0;

    return inode_access_ok(ip,
                           scfs_uid_get(), scfs_gid_get(),
                           (mode & SCFS_R_OK) ? 1 : 0,
                           (mode & SCFS_W_OK) ? 1 : 0,
                           (mode & SCFS_X_OK) ? 1 : 0) ? 1 : 0;
}

/* ── access() — by path name ────────────────────────────────────────── */
int uiox_kix_scfs_access(const char *path, int mode)
{
    InCoreInode *ip;
    int          ok;

    if (!path) return SCFS_EFAULT;
    if (mode & ~(SCFS_R_OK | SCFS_W_OK | SCFS_X_OK | SCFS_F_OK))
        return SCFS_EINVAL;

    /* Bach's namei takes the credentials, so the directory walk applies
     * the same test to every component on the way. */
    ip = namei(path, scfs_cwd_get(), scfs_uid_get(), scfs_gid_get());
    if (!ip) return SCFS_ENOENT;

    /* F_OK asks only that the file exist — namei resolving is the whole
     * answer, so no permission bits are consulted. */
    ok = (mode == SCFS_F_OK) ? 1 : scfs_perm_test(ip, mode);

    iput(ip);
    return ok ? SCFS_OK : SCFS_EACCES;
}

uint16_t uiox_kix_scfs_umask(uint16_t mask) { return scfs_umask_set(mask); }

/*
 * scfs_apply_umask — the masking step every creating call performs.
 * Bach:  perm = requested & ~umask.
 * Clears only the permission bits; the file type nibble is added later by
 * ialloc() itself.
 */
uint16_t scfs_apply_umask(uint16_t perm)
{
    return (uint16_t)((perm & 0777u) & ~(scfs_umask_get() & 0777u));
}

/* ── setgid — chown with only the group field moving ────────────────── */
int uiox_kix_scfs_setgid(const char *path, uint16_t gid)
{
    InCoreInode *ip;

    if (!path) return SCFS_EFAULT;

    ip = namei(path, scfs_cwd_get(), scfs_uid_get(), scfs_gid_get());
    if (!ip) return SCFS_ENOENT;

    ip->gid = gid;
    ip->flags |= IFLAG_CHANGED;
    iupdate(ip);
    iput(ip);
    return SCFS_OK;
}
