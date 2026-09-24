#include "uiox_kix_scfs_internal.h"

/*
 * The inode carries three timestamps.  i_ctime is the one people get
 * wrong: a ctime is not "the last modification" — it is "the last time
 * the metadata moved".  Every changing call updates it.
 */
static int64_t s_now = 0;

void    scfs_time_set(int64_t t) { s_now = t; }
int64_t scfs_time_now(void)      { return s_now; }

/* ── the shared body ───────────────────────────────────────────────── */
static int scfs_do_utime(InCoreInode *ip, const int64_t *ts)
{
    if (!ip) return SCFS_EINVAL;

    if (ts) {
        ip->atime = (time_t)ts[0];
        ip->mtime = (time_t)ts[1];
    } else {
        int64_t now = scfs_time_now();
        ip->atime = (time_t)now;
        ip->mtime = (time_t)now;
    }

    /* Changing the inode always moves ctime, and it is not the caller's
     * to set. */
    ip->ctime = (time_t)scfs_time_now();

    ip->flags |= IFLAG_CHANGED;
    iupdate(ip);
    return SCFS_OK;
}

int uiox_kix_scfs_utime(const char *path, const int64_t *times)
{
    InCoreInode *ip;
    int          rc;

    if (!path) return SCFS_EFAULT;

    ip = namei(path, scfs_cwd_get(), scfs_uid_get(), scfs_gid_get());
    if (!ip) return SCFS_ENOENT;

    rc = scfs_do_utime(ip, times);
    ip->locked = false;
    iput(ip);
    return rc;
}

/* utimes — the same call with microsecond precision.  01_fsa's inode
 * stores whole seconds, so the fraction is DROPPED rather than rounded:
 * a stamp that never existed is worse than a truncated one. */
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
