#include "../include/pty_open.h"
#include <stdarg.h>

/*
 * ptym_open() — Figure 19.9
 *
 * Opens the next available PTY master using the portable
 * POSIX XSI sequence.
 *
 * Steps:
 *   1. posix_openpt(O_RDWR)
 *      Opens /dev/ptmx (or equivalent) and returns a master fd.
 *      O_RDWR: open for reading and writing.
 *      O_NOCTTY could be added to prevent master becoming
 *      a controlling terminal (not needed here since we want
 *      only the slave to become the controlling terminal).
 *
 *   2. grantpt(fdm)
 *      Sets slave device ownership: uid=caller's real uid,
 *      gid=tty (group with terminal write access), mode=0620.
 *      May fork/exec a setuid helper (e.g. /usr/lib/pt_chmod
 *      on Solaris), so SIGCHLD must not be caught by caller.
 *
 *   3. unlockpt(fdm)
 *      Clears the slave's internal lock.  Until this is called
 *      no process can open the slave, giving us time to
 *      initialize master and slave before either is used.
 *
 *   4. ptsname(fdm)
 *      Returns the slave pathname (e.g. "/dev/pts/5").
 *      Result may be in static storage — copy immediately.
 *
 * Error handling: on any failure, close fdm and return -1
 * with errno preserved from the failing call.
 */
int ptym_open(char *pts_name, int pts_namesz)
{
    char *ptr;
    int   fdm, err;

    /* Step 1: open next available PTY master */
    fdm = posix_openpt(O_RDWR);
    if (fdm < 0)
        return -1;

    /* Step 2: set slave device permissions */
    if (grantpt(fdm) < 0)
        goto errout;

    /* Step 3: unlock slave so it can be opened */
    if (unlockpt(fdm) < 0)
        goto errout;

    /* Step 4: get slave pathname */
    ptr = ptsname(fdm);
    if (ptr == NULL)
        goto errout;

    /*
     * Copy slave name to caller's buffer.
     * strncpy does not guarantee null termination if src is
     * longer than pts_namesz, so we force it.
     */
    strncpy(pts_name, ptr, (size_t)pts_namesz);
    pts_name[pts_namesz - 1] = '\0';

    return fdm;  /* success: return PTY master fd */

errout:
    err = errno;
    close(fdm);
    errno = err;
    return -1;
}

/*
 * ptys_open() — Figure 19.9 
 *
 * Opens the PTY slave device by name.
 *
 * On Linux, macOS, and FreeBSD:
 *   A plain open() is sufficient.  On Linux and macOS, if the
 *   caller is a session leader without a controlling terminal,
 *   this open() call automatically allocates the slave as the
 *   controlling terminal (System V behavior).
 *
 * On FreeBSD:
 *   open() does NOT allocate a controlling terminal automatically.
 *   The caller must issue ioctl(fds, TIOCSCTTY, 0) after this call.
 *   (See pty_fork.c.)
 *
 * On Solaris (STREAMS):
 *   After open(), we may need to push three modules:
 *
 *   ptem (pseudo terminal emulation module):
 *     Provides PTY-specific semantics above the slave.
 *     Together with ldterm, makes the slave behave like a terminal.
 *
 *   ldterm (terminal line discipline module):
 *     Provides canonical/noncanonical processing, echo, signal
 *     generation (SIGINT, SIGQUIT, SIGTSTP), flow control, etc.
 *
 *   ttcompat (terminal compatibility module):
 *     Provides backward compatibility for older ioctl calls
 *     from V7, 4BSD, and Xenix.  Optional but pushed automatically
 *     for network logins so we push it here too.
 *
 *   We first check if ldterm is already present (autopush may
 *   have configured it).  If so, skip pushing to avoid duplicates.
 */
int ptys_open(char *pts_name)
{
    int fds;
#if defined(SOLARIS)
    int err, setup;
#endif

    /* Open the slave PTY device */
    fds = open(pts_name, O_RDWR);
    if (fds < 0)
        return -1;

#if defined(SOLARIS)
    /*
     * Check if ldterm is already on the stream (autopush).
     * I_FIND: search for named module in stream.
     * Returns > 0 if found, 0 if not, -1 on error.
     */
    setup = ioctl(fds, I_FIND, "ldterm");
    if (setup < 0)
        goto errout;

    if (setup == 0) {
        /* Push ptem — PTY emulation module */
        if (ioctl(fds, I_PUSH, "ptem") < 0)
            goto errout;

        /* Push ldterm — terminal line discipline */
        if (ioctl(fds, I_PUSH, "ldterm") < 0)
            goto errout;

        /* Push ttcompat — ioctl compatibility */
        if (ioctl(fds, I_PUSH, "ttcompat") < 0)
            goto errout;
    }
    return fds;

errout:
    err = errno;
    close(fds);
    errno = err;
    return -1;
#else
    return fds;
#endif
}
