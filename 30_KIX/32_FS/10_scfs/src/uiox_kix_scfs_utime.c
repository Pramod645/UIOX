/*
 * 30_KIX/32_FS/10_scfs/src/uiox_kix_scfs_utime.c
 *
 * SCFS — utime, utimes, futimes, futimens.
 * Bach, The Design of the UNIX Operating System.
 *
 * ── what Bach says ─────────────────────────────────────────────────────
 * The inode carries three timestamps, and Bach names all three when he
 * lists what a status call returns — "file access times" — and writes
 * them at fixed points:
 *
 *   i_atime   the last READ  of the file's data
 *   i_mtime   the last WRITE of the file's data
 *   i_ctime   the last change to the INODE itself
 *
 * The third is the one people get wrong.  Bach's chown, chmod, link and
 * unlink algorithms all change the inode without touching file data, and
 * each of them updates i_ctime for exactly that reason.  A ctime is not
 * "the last modification"; it is "the last time the metadata moved".
 *
 * ── utime ──────────────────────────────────────────────────────────────
 * utime(2) lets a caller set atime and mtime explicitly — the call tar,
 * cp -p and every backup tool needs so a restored file keeps its dates.
 * POSIX says passing NULL for the pair sets both to "now", which is what
 * a program uses to say "this file has just been touched".
 *
 * ctime is NOT settable.  It always moves to now, because the caller has
 * just changed the inode by making this very call.
 *
 * @version 1.0.0  @date 2026-09-23
 */
#include "uiox_kix_scfs_internal.h"

/* The current time.  Bach reads it from the system clock; this kernel has
 * no clock source wired into the file layer, so the value SCFS holds is
 * used and a bring-up build leaves it at 0.  The unit is seconds since
 * the epoch, matching the int64_t fields in the inode. */
static int64_t s_now = 0;

void    scfs_time_set(int64_t t) { s_now = t; }
int64_t scfs_time_now(void)      { return s_now; }

/* ── the shared body ─────────────────────────────────────────────────
 * Bach's shape exactly: change fields in the in-core inode, mark it
 * changed, write it back.  Called by all four entries below.
 */
static int scfs_do_utime(InCoreInode *ip, const int64_t *ts)
{
    if (!ip) return SCFS_EINVAL;

    if (ts) {
        /* The caller supplied both stamps. */
        ip->atime = ts[0];
        ip->mtime = ts[1];
    } else {
        /* NULL means "now" for both — the touch(1) case. */
        int64_t now = scfs_time_now();
        ip->atime = now;
        ip->mtime = now;
    }

    /* Changing the inode always moves ctime, and it is not the caller's
     * to set.  Bach does this in every algorithm that writes an inode. */
    ip->ctime = scfs_time_now();

    ip->flags |= IFLAG_CHANGED;
    iupdate(ip);                            /* 01_fsa */
    return SCFS_OK;
}

/* ── utime() — two explicit stamps, or "now" ────────────────────────── */
int uiox_kix_scfs_utime(const char *path, const int64_t *times)
{
    if (!path) return SCFS_EFAULT;

    InCoreInode *ip = namei(path, scfs_cwd_get(), 0u, 0u);   /* 01_fsa */
    if (!ip) return SCFS_ENOENT;

    int rc = scfs_do_utime(ip, times);

    ip->locked = false;
    iput(ip);                               /* 01_fsa */
    return rc;
}

/*
 * utimes() — the same call with microsecond precision.
 *
 * 01_fsa's inode stores whole seconds, so the fractional part is dropped
 * rather than rounded: a caller setting 1.9s would otherwise get 2s, and
 * a stamp that never existed is worse than a truncated one.
 */
int uiox_kix_scfs_utimes(const char *path, const int64_t *times_us)
{
    int64_t secs[2];

    if (!path) return SCFS_EFAULT;

    if (times_us) {
        secs[0] = times_us[0] / 1000000;
        secs[1] = times_us[1] / 1000000;
    }

    return uiox_kix_scfs_utime(path, times_us ? secs : (const int64_t *)0);
}

/* ── futimes() / futimens() — by descriptor ──────────────────────────
 * No namei and no iput: the file table entry already holds the inode.
 */
int uiox_kix_scfs_futimes(int fd, const int64_t *times)
{
    scfs_file_t *f = scfs_getf(fd);
    if (!f) return SCFS_EBADF;

    return scfs_do_utime(f->f_inode, times);
}

int uiox_kix_scfs_futimens(int fd, const int64_t *times_us)
{
    int64_t secs[2];

    if (times_us) {
        secs[0] = times_us[0] / 1000000;
        secs[1] = times_us[1] / 1000000;
    }
    return uiox_kix_scfs_futimes(fd, times_us ? secs : (const int64_t *)0);
}
