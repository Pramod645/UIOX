#include "../include/pty_fork.h"
#include <stdarg.h>

/*
 * pty_fork() — Figure 19.10 (APUE §19.4)
 *
 * Combines ptym_open + fork + setsid + ptys_open + dup2
 * into one portable function.
 *
 * Parent path:
 *   • Calls ptym_open() — gets master fd and slave name.
 *   • Copies slave_name to caller's buffer if non-NULL.
 *   • Calls fork().
 *   • Stores fdm in *ptrfdm.
 *   • Returns child PID.
 *
 * Child path (after fork):
 *   1. setsid()
 *      Creates a new session.  The child becomes the session
 *      leader of the new session.  It also becomes the leader
 *      of a new process group and loses any association with
 *      its previous controlling terminal.
 *      Prerequisite: child must NOT be a process group leader.
 *      This is guaranteed because fork() gives the child a new
 *      PID different from the parent's PGID.
 *
 *   2. ptys_open(pts_name)
 *      Opens the slave PTY.
 *      On Linux/macOS/Solaris: this automatically allocates the
 *      slave as the controlling terminal of the new session
 *      (because child is now a session leader without a CTY).
 *      On FreeBSD: must use TIOCSCTTY ioctl explicitly.
 *
 *   3. close(fdm)
 *      Child no longer needs the master fd; close it.
 *
 *   4. TIOCSCTTY (FreeBSD only)
 *      Explicitly allocates slave as controlling terminal.
 *      See Figure 9.8 in APUE for platform comparison.
 *
 *   5. tcsetattr() / TIOCSWINSZ
 *      Initialize slave's termios and window size to match
 *      the caller's real terminal (if slave_termios and
 *      slave_winsize are non-NULL).
 *      TCSANOW: apply changes immediately.
 *
 *   6. dup2() to stdin/stdout/stderr
 *      Makes the slave PTY the child's controlling terminal
 *      AND its standard I/O descriptors.
 *      Any program exec'd by the child will have a terminal
 *      for its standard streams.
 *      If fds > 2, close it (we have copies on 0, 1, 2).
 *
 *   7. return 0 — child returns like fork().
 */
pid_t pty_fork(int *ptrfdm,
               char *slave_name,
               int   slave_namesz,
               const struct termios *slave_termios,
               const struct winsize *slave_winsize)
{
    int   fdm, fds;
    pid_t pid;
    char  pts_name[PTY_NAME_MAX];

    /* Open PTY master — also initializes slave device */
    fdm = ptym_open(pts_name, sizeof(pts_name));
    if (fdm < 0) {
        fprintf(stderr, "pty_fork: can't open master pty: %s\n",
                pts_name);
        return -1;
    }

    /* Return slave name to caller if buffer provided */
    if (slave_name != NULL) {
        strncpy(slave_name, pts_name, (size_t)slave_namesz);
        slave_name[slave_namesz - 1] = '\0';
    }

    pid = fork();
    if (pid < 0) {
        close(fdm);
        return -1;

    } else if (pid == 0) {
        /* ── Child process ──────────────────────────────── */

        /*
         * Step 1: Create new session.
         * Child is no longer a process group leader (it has
         * a new PID), so setsid() will succeed.
         * After setsid():
         *   - new session with child as leader
         *   - new process group with child as leader
         *   - no controlling terminal
         */
        if (setsid() < 0) {
            fprintf(stderr, "pty_fork: setsid error: %s\n",
                    strerror(errno));
            exit(1);
        }

        /*
         * Step 2: Open slave PTY.
         * On Linux/macOS/Solaris: this open() allocates the slave
         * as the controlling terminal of the new session (because
         * child is a session leader without a CTY).
         */
        fds = ptys_open(pts_name);
        if (fds < 0) {
            fprintf(stderr, "pty_fork: can't open slave pty: %s\n",
                    strerror(errno));
            exit(1);
        }

        /* Step 3: Close master in child — no longer needed */
        close(fdm);

#if defined(__FreeBSD__) || defined(BSD)
        /*
         * Step 4 (FreeBSD only): Allocate controlling terminal.
         * FreeBSD's open() does not allocate a controlling terminal
         * as a side effect.  TIOCSCTTY explicitly assigns the slave
         * as the controlling terminal of the session.
         */
        if (ioctl(fds, TIOCSCTTY, (char *)0) < 0) {
            fprintf(stderr, "pty_fork: TIOCSCTTY error: %s\n",
                    strerror(errno));
            exit(1);
        }
#endif

        /*
         * Step 5a: Initialize slave termios.
         * If caller provided termios (from real terminal), apply them.
         * This makes the slave behave identically to the real terminal
         * for special characters, echo settings, etc.
         * TCSANOW: apply immediately without waiting.
         */
        if (slave_termios != NULL) {
            if (tcsetattr(fds, TCSANOW, slave_termios) < 0) {
                fprintf(stderr,
                        "pty_fork: tcsetattr error on slave: %s\n",
                        strerror(errno));
                exit(1);
            }
        }

        /*
         * Step 5b: Initialize slave window size.
         * TIOCSWINSZ: set window size (rows, cols, pixel dims).
         * If new size differs from current, kernel sends SIGWINCH
         * to foreground process group of slave — but since slave
         * has no processes yet, no signal is sent here.
         */
        if (slave_winsize != NULL) {
            if (ioctl(fds, TIOCSWINSZ, slave_winsize) < 0) {
                fprintf(stderr,
                        "pty_fork: TIOCSWINSZ error on slave: %s\n",
                        strerror(errno));
                exit(1);
            }
        }

        /*
         * Step 6: Make slave the standard I/O descriptors.
         * dup2(fds, 0): slave fd → stdin
         * dup2(fds, 1): slave fd → stdout
         * dup2(fds, 2): slave fd → stderr
         * If fds > 2, it is now redundant — close it.
         * Any program exec'd by this child will have a terminal
         * for all three standard streams.
         */
        if (dup2(fds, STDIN_FILENO) != STDIN_FILENO) {
            fprintf(stderr, "pty_fork: dup2 error to stdin\n");
            exit(1);
        }
        if (dup2(fds, STDOUT_FILENO) != STDOUT_FILENO) {
            fprintf(stderr, "pty_fork: dup2 error to stdout\n");
            exit(1);
        }
        if (dup2(fds, STDERR_FILENO) != STDERR_FILENO) {
            fprintf(stderr, "pty_fork: dup2 error to stderr\n");
            exit(1);
        }
        if (fds != STDIN_FILENO  &&
            fds != STDOUT_FILENO &&
            fds != STDERR_FILENO)
            close(fds);

        /* Step 7: child returns 0, exactly like fork() */
        return 0;

    } else {
        /* ── Parent process ─────────────────────────────── */
        *ptrfdm = fdm;   /* return master fd to caller */
        return pid;      /* return child PID */
    }
}
