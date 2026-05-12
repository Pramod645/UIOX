#ifndef PTY_FORK_H
#define PTY_FORK_H

/*
 * pty_fork.h — Combined fork + PTY setup function.
 *
 * Section 19.4 / Figure 19.10
 *
 * pty_fork() is the high-level function that most callers use.
 * It combines ptym_open, fork, setsid, ptys_open, and dup2
 * into one call, establishing the child as a session leader
 * with the PTY slave as its controlling terminal.
 *
 * Return values (same convention as fork):
 *   In parent: returns child PID, *ptrfdm = PTY master fd
 *   In child:  returns 0, stdin/stdout/stderr = PTY slave
 *   On error:  returns -1
 *
 * Terminal initialization:
 *   slave_termios:  If non-NULL, slave termios is set to this
 *                   before the child execs.  Pass the parent's
 *                   termios to have the slave match the real terminal.
 *   slave_winsize:  If non-NULL, slave window size is set to this.
 *                   Pass the parent's winsize from TIOCGWINSZ.
 *
 * Typical usage:
 *
 *   struct termios orig;
 *   struct winsize sz;
 *   tcgetattr(STDIN_FILENO, &orig);
 *   ioctl(STDIN_FILENO, TIOCGWINSZ, &sz);
 *
 *   pid = pty_fork(&fdm, name, sizeof(name), &orig, &sz);
 *   if (pid == 0) {
 *       execvp(argv[0], argv);
 *   }
 *   // parent: use fdm to talk to child
 */

#include "pty.h"
#include "pty_open.h"

/*
 * pty_fork()
 *
 * @param ptrfdm        Out: PTY master file descriptor (parent only).
 * @param slave_name    Out: slave device name (may be NULL).
 * @param slave_namesz  Size of slave_name buffer.
 * @param slave_termios Desired slave termios (NULL = implementation default).
 * @param slave_winsize Desired slave window size (NULL = zeroed).
 *
 * Returns: child PID in parent, 0 in child, -1 on error.
 */
pid_t pty_fork(int *ptrfdm,
               char *slave_name,
               int   slave_namesz,
               const struct termios *slave_termios,
               const struct winsize *slave_winsize);

#endif /* PTY_FORK_H */
