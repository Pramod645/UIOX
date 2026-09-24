/*
 *  30_KIX/32_FS/10_scfs/src/uiox_kix_scfs_access.c
 *
 *  SCFS — access, umask, the permission test.  CORRECTED.
 *
 *  ── what Bach says ───────────────────────────────────────────────────
 *  Algorithm open, step 2:
 *    "if (file is not being created and the process does not have
 *     permission to open the file)"
 *  and §1.3 on the test itself:
 *    "The kernel tests the access permissions of a file by comparing the
 *     owner, group, and other fields of the inode with the process's user
 *     and group ID."
 *
 *  01_fsa exports that test as inode_access_ok(), and access(2) is the
 *  same test presented as its own syscall.
 *
 *  ── the correction ───────────────────────────────────────────────────
 *  The first cut claimed "the super user bypasses everything but execute
 *  on a plain file — Bach states the exception", and implemented a bypass
 *  around inode_access_ok().  That is wrong for THIS filesystem.
 *
 *  inode.c's inode_access_ok() tests owner / group / other and nothing
 *  else.  There is NO uid-0 bypass in it.  So:
 *
 *    · the first cut's bypass was a 10_scfs invention that made access()
 *      disagree with open() — access() would say yes and open() would
 *      fail with EACCES
 *    · that is the worst possible failure for access(), whose whole
 *      purpose is to predict what open() will do
 *
 *  The bypass is REMOVED.  scfs_perm_test() now calls inode_access_ok()
 *  and reports exactly what it reports.  If a super-user bypass is wanted,
 *  it belongs in inode_access_ok() in 01_fsa, where open(), chdir() and
 *  every other caller would get it too — one place, one answer.
 *
 *  ── umask ────────────────────────────────────────────────────────────
 *  Bach keeps the creation mask in the u area (u.u_umask).  Every call
 *  that CREATES — open(O_CREAT), creat, mkdir, mknod, mkfifo — masks the
 *  requested permission before it reaches ialloc().  Mode 0666 with umask
 *  022 yields 0644.  That masking is scfs_apply_umask() below, and the
 *  units call it.
 *
 *  v1.1: super-user bypass removed; perm test delegates to 01_fsa.
 */
#include "uiox_kix_scfs_internal.h"

/* ═════════════════════════════════════════════════════════════════════
 * Credentials
 *
 * Bach reads the effective uid and gid out of the process structure.
 * This layer has no process, so it holds them and 33_PCS overrides with
 * scfs_cred_set().  uid 0 / gid 0 is the bring-up context.
 * ═════════════════════════════════════════════════════════════════════ */
static uint16_t s_uid = 0u;
static uint16_t s_gid = 0u;

void     scfs_cred_set(uint16_t uid, uint16_t gid) { s_uid = uid; s_gid = gid; }
uint16_t scfs_uid_get (void) { return s_uid; }
uint16_t scfs_gid_get (void) { return s_gid; }

/*
 * scfs_is_super — "is the current credential the super user?"
 *
 * Bach decides this from the effective uid in the process structure.
 * Until 33_PCS supplies one, uid 0 is the super user.
 *
 * NOTE: this answers a question about the CALLER.  It does not grant a
 * permission bypass — inode_access_ok() is the only thing that decides
 * access, and it has no bypass.  Functions that need privilege (chroot,
 * mount, the trusted xattr namespace) test this and refuse.
 */
int scfs_is_super(void) { return (s_uid == 0u) ? 1 : 0; }

/* ═════════════════════════════════════════════════════════════════════
 * The shared permission test
 *
 * No bypass, no special cases: exactly what 01_fsa decides.  Used by
 * access() and by any unit that needs the test without the syscall.
 * ═════════════════════════════════════════════════════════════════════ */
int scfs_perm_test(InCoreInode *ip, int mode)
{
    if (!ip) return 0;

    return inode_access_ok(ip,
                           scfs_uid_get(), scfs_gid_get(),
                           (mode & SCFS_R_OK) ? 1 : 0,
                           (mode & SCFS_W_OK) ? 1 : 0,
                           (mode & SCFS_X_OK) ? 1 : 0) ? 1 : 0;   /* 01_fsa */
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
    ip = namei(path, scfs_cwd_get(), scfs_uid_get(), scfs_gid_get());  /* 01_fsa */
    if (!ip) return SCFS_ENOENT;

    /* F_OK asks only that the file exist — namei resolving is the whole
     * answer, so no permission bits are consulted. */
    ok = (mode == SCFS_F_OK) ? 1 : scfs_perm_test(ip, mode);

    iput(ip);                               /* 01_fsa */
    return ok ? SCFS_OK : SCFS_EACCES;
}

/* ── umask() ────────────────────────────────────────────────────────── */
uint16_t uiox_kix_scfs_umask(uint16_t mask)
{
    /* The value lives in the table unit: scfs_umask_get()/set(). */
    return scfs_umask_set(mask);
}

/*
 * scfs_apply_umask — the masking step every creating call performs.
 *
 * Bach:  perm = requested & ~umask.
 *
 * Called by open(O_CREAT), creat, mkdir and mknod before the permission
 * reaches ialloc().  It clears only the permission bits: the file type
 * nibble is added by ialloc() itself and must be left alone here.
 *
 * fs_types.h defines PERM_U* / PERM_G* / PERM_O* as octal values that
 * sum to 0777, which is what the mask below covers.
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
    iupdate(ip);                            /* 01_fsa */

    iput(ip);                               /* 01_fsa */
    return SCFS_OK;
}

/* sys_* aliases — the consolidated syscalls.c owns these; keep one. */
int      sys_access(const char *path, int mode) { return uiox_kix_scfs_access(path, mode); }
uint16_t sys_umask (uint16_t mask)              { return uiox_kix_scfs_umask(mask); }
