#include "../include/daemon.h"

/*
 * daemonize()
 *
 * Transforms the calling process into a properly initialized
 * UNIX daemon, following all six coding rules from APUE §13.3.
 *
 * Step 1: umask(0)
 *   Clears the file mode creation mask so the daemon can
 *   create files with any required permissions without the
 *   inherited mask silently removing bits.
 *
 * Step 2 & 3: first fork + setsid()
 *   The parent exits so the shell believes the command
 *   finished.  The child calls setsid() to:
 *     (a) become leader of a new session,
 *     (b) become leader of a new process group,
 *     (c) lose its controlling terminal.
 *
 * Step 4: ignore SIGHUP + second fork
 *   After setsid() the child is a session leader.  We ignore
 *   SIGHUP and fork again so the grandchild is NOT a session
 *   leader.  Under System V rules only session leaders can
 *   acquire a controlling terminal, so this prevents any
 *   future open() from allocating one inadvertently.
 *
 * Step 5: chdir("/")
 *   The working directory inherited from the parent might
 *   live on a mounted filesystem.  Moving to "/" ensures
 *   that filesystem can be unmounted while the daemon runs.
 *
 * Step 6: close all file descriptors
 *   Descriptors inherited from the parent (terminal, pipes,
 *   sockets) are closed.  getrlimit() provides the upper
 *   bound; RLIM_INFINITY is capped at DAEMON_MAX_FD (1024).
 *
 * Step 7: redirect stdin/stdout/stderr to /dev/null
 *   Opens /dev/null O_RDWR and dup()s it to fds 0, 1, 2.
 *   Library calls that write to stdout/stderr produce no
 *   visible output.  Reads from stdin return EOF immediately.
 *
 * Step 8: openlog()
 *   Initialises the syslog connection using the daemon name
 *   as the ident string.  LOG_CONS means messages fall back
 *   to /dev/console if syslogd is unreachable.
 */
void daemonize(const char *cmd)
{
    int          i, fd0, fd1, fd2;
    pid_t        pid;
    struct rlimit rl;
    struct sigaction sa;

    /* ── Step 1: Clear file creation mask ──────────────── */
    umask(0);

    /* ── Get maximum number of open file descriptors ───── */
    if (getrlimit(RLIMIT_NOFILE, &rl) < 0) {
        fprintf(stderr, "%s: can't get file limit: %s\n",
                cmd, strerror(errno));
        exit(1);
    }

    /* ── Step 2: First fork — parent exits ─────────────── */
    if ((pid = fork()) < 0) {
        fprintf(stderr, "%s: can't fork: %s\n",
                cmd, strerror(errno));
        exit(1);
    } else if (pid != 0) {
        /* Parent exits — shell thinks command finished    */
        exit(0);
    }

    /* ── Step 3: Create a new session ──────────────────── */
    /*
     * Child is guaranteed not to be a process group leader
     * (it inherited the group but got a new PID), so setsid()
     * will succeed.  After setsid():
     *   • New session created
     *   • Child is session leader of the new session
     *   • Child is process group leader of a new group
     *   • No controlling terminal
     */
    setsid();

    /* ── Step 4: Ignore SIGHUP, then second fork ─────── */
    /*
     * The second fork ensures the daemon is NOT a session
     * leader, preventing it from ever acquiring a controlling
     * terminal under System V semantics (only a session leader
     * can acquire one by opening a terminal device).
     * We must ignore SIGHUP first because when the session
     * leader (first child) exits, the kernel sends SIGHUP to
     * every process in its foreground process group.
     */
    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGHUP, &sa, NULL) < 0) {
        fprintf(stderr, "%s: can't ignore SIGHUP: %s\n",
                cmd, strerror(errno));
        exit(1);
    }

    if ((pid = fork()) < 0) {
        fprintf(stderr, "%s: can't fork: %s\n",
                cmd, strerror(errno));
        exit(1);
    } else if (pid != 0) {
        /* First child exits — grandchild continues */
        exit(0);
    }

    /* ── Step 5: Change working directory to root ──────── */
    /*
     * Prevents the daemon from holding a reference to any
     * mounted filesystem.  A mounted filesystem cannot be
     * unmounted while any process has its current directory
     * inside it.
     */
    if (chdir("/") < 0) {
        fprintf(stderr, "%s: can't chdir to /: %s\n",
                cmd, strerror(errno));
        exit(1);
    }

    /* ── Step 6: Close all open file descriptors ────────── */
    /*
     * RLIM_INFINITY means no limit is set; fall back to a
     * safe maximum (1024) to avoid iterating forever.
     */
    if (rl.rlim_max == RLIM_INFINITY)
        rl.rlim_max = DAEMON_MAX_FD;

    for (i = 0; i < (int)rl.rlim_max; i++)
        close(i);

    /* ── Step 7: Attach fds 0/1/2 to /dev/null ─────────── */
    /*
     * open() returns the lowest available fd.  After closing
     * all fds above, the next open() will return 0 (stdin).
     * dup(0) → 1 (stdout), dup(0) → 2 (stderr).
     * Any library routine writing to stdout/stderr silently
     * discards the output.
     */
    fd0 = open(DAEMON_DEV_NULL, O_RDWR);   /* fd 0 = stdin  */
    fd1 = dup(0);                           /* fd 1 = stdout */
    fd2 = dup(0);                           /* fd 2 = stderr */

    /* ── Step 8: Initialise syslog ─────────────────────── */
    /*
     * LOG_CONS  — fall back to console if syslogd is down.
     * LOG_DAEMON — facility for system daemons.
     */
    openlog(cmd, LOG_CONS, LOG_DAEMON);

    /* Verify that /dev/null ended up on the expected fds   */
    if (fd0 != 0 || fd1 != 1 || fd2 != 2) {
        syslog(LOG_ERR,
               "unexpected file descriptors %d %d %d",
               fd0, fd1, fd2);
        exit(1);
    }
}
