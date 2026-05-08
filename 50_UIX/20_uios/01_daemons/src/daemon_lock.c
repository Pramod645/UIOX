#include "../include/daemon_lock.h"

/*
 * lockfile()
 *
 * Places a POSIX write lock on the entire file fd.
 *
 * Uses F_SETLK (non-blocking).  If another process holds
 * a conflicting lock, errno is set to EACCES or EAGAIN and
 * -1 is returned immediately — the caller interprets this
 * as "another instance is running".
 *
 * The lock is automatically released by the kernel when the
 * process terminates or closes the file descriptor, so no
 * explicit cleanup is required.
 *
 * APUE §14.3 explains POSIX record locking in detail.
 */
int lockfile(int fd)
{
    struct flock fl;

    fl.l_type   = F_WRLCK;   /* exclusive write lock   */
    fl.l_start  = 0;          /* start at beginning     */
    fl.l_whence = SEEK_SET;   /* relative to start      */
    fl.l_len    = 0;          /* lock entire file       */

    return fcntl(fd, F_SETLK, &fl);
}

/*
 * already_running()
 *
 * Enforces the single-instance rule described in §13.5.
 *
 * Algorithm (Figure 13.6):
 *   1. Open/create the lock file with O_RDWR | O_CREAT.
 *   2. Call lockfile() to attempt a write lock.
 *   3. If lock fails with EACCES or EAGAIN: another instance
 *      is running — close fd and return 1.
 *   4. If lock fails for any other reason: log and exit.
 *   5. If lock succeeds:
 *      a. Truncate the file to 0 bytes.  This is necessary
 *         because a previous daemon may have had a longer PID
 *         string (e.g. "12345" → "9999" would leave "99995").
 *      b. Write the current PID as a decimal string.
 *      c. Return 0 (we are the only running instance).
 */
int already_running(void)
{
    return already_running_named(DAEMON_LOCKFILE);
}

/*
 * already_running_named()
 *
 * Same logic as already_running() but the caller supplies
 * the lock file path, following the convention of
 * /var/run/<name>.pid described in §13.6.
 */
int already_running_named(const char *lockpath)
{
    int  fd;
    char buf[32];

    /* Open or create the lock file */
    fd = open(lockpath, O_RDWR | O_CREAT, DAEMON_LOCKMODE);
    if (fd < 0) {
        syslog(LOG_ERR, "can't open %s: %s",
               lockpath, strerror(errno));
        exit(1);
    }

    /* Attempt to acquire a write lock */
    if (lockfile(fd) < 0) {
        if (errno == EACCES || errno == EAGAIN) {
            /*
             * Lock held by another process — another instance
             * of this daemon is already running.
             */
            close(fd);
            return 1;
        }
        /* Unexpected error */
        syslog(LOG_ERR, "can't lock %s: %s",
               lockpath, strerror(errno));
        exit(1);
    }

    /* Lock acquired — we are the only running instance */

    /* Truncate: discard any PID string from a previous run */
    ftruncate(fd, 0);

    /* Write our PID so administrators can find us */
    snprintf(buf, sizeof(buf), "%ld\n", (long)getpid());
    write(fd, buf, strlen(buf));

    /*
     * Do NOT close fd — closing it would release the lock.
     * The descriptor stays open for the lifetime of the
     * daemon; the lock is released automatically on exit.
     */

    return 0;
}
