#ifndef PROC_SESSION_H
#define PROC_SESSION_H

/*
 * proc_session.h — Session management functions.
 *
 * APUE Chapter 9, Sections 9.5, 9.6, 9.7
 *
 * A session is a collection of one or more process groups.
 *
 * Session creation via setsid():
 *   If the caller is NOT a process group leader:
 *     1. A new session is created; caller becomes session leader.
 *     2. A new process group is created; caller becomes leader.
 *     3. Any controlling terminal association is broken.
 *   If caller IS already a group leader → error (EPERM).
 *
 * To safely call setsid():
 *   fork() first, parent exits, child calls setsid().
 *   Child's PID != inherited PGID, so it cannot be a leader.
 *
 * Controlling terminal:
 *   A session can have at most ONE controlling terminal.
 *   The session leader that opens the terminal becomes the
 *   controlling process.
 *   Foreground process group receives terminal input and signals
 *   (SIGINT, SIGQUIT, SIGTSTP from special keys).
 *   Background process groups receive SIGTTIN on terminal read,
 *   and optionally SIGTTOU on terminal write.
 *
 * Platform differences for allocating controlling terminal:
 *   System V / Linux / macOS:  first open() by session leader
 *                              without O_NOCTTY allocates it.
 *   BSD / FreeBSD:             requires ioctl(TIOCSCTTY).
 *   All platforms support TIOCSCTTY.
 *
 * Functions:
 *   getsid(pid)   — returns PGID of session leader (== session ID)
 *   tcgetpgrp(fd) — returns PGID of foreground process group
 *   tcsetpgrp(fd, pgid) — sets foreground process group
 *   tcgetsid(fd)  — returns session leader's PGID for the
 *                   controlling terminal on fd
 */

#include <unistd.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

/*
 * sess_create()
 *
 * Creates a new session by forking and having the child call
 * setsid().  The parent exits immediately.
 *
 * After return (in child):
 *   - New session with caller as session leader.
 *   - New process group with caller as group leader.
 *   - No controlling terminal.
 *
 * This function is the standard daemonization first step.
 * Returns the new session's PGID (== new PID) on success,
 * -1 on error.
 */
pid_t sess_create(void);

/*
 * sess_get_sid()
 *
 * Returns the process group ID of the session leader for the
 * session containing process pid.
 *
 * @param pid  0 = calling process.
 *
 * Wraps getsid(pid).
 */
pid_t sess_get_sid(pid_t pid);

/*
 * sess_get_foreground_pgid()
 *
 * Returns the PGID of the foreground process group associated
 * with the terminal open on fd.
 *
 * Wraps tcgetpgrp(fd).
 */
pid_t sess_get_foreground_pgid(int fd);

/*
 * sess_set_foreground_pgid()
 *
 * Sets the foreground process group of the terminal on fd to
 * the process group identified by pgid.
 *
 * pgrpid must be in the same session; fd must be the
 * controlling terminal.
 *
 * Wraps tcsetpgrp(fd, pgid).
 *
 * Returns 0 on success, -1 on error.
 */
int sess_set_foreground_pgid(int fd, pid_t pgid);

/*
 * sess_get_ctty_sid()
 *
 * Returns the session leader's PGID for the controlling terminal
 * open on fd.
 *
 * Wraps tcgetsid(fd).
 */
pid_t sess_get_ctty_sid(int fd);

/*
 * sess_print_relationships()
 *
 * Prints PID, PPID, PGID, SID, and the foreground PGID of the
 * controlling terminal for the current process.
 *
 * @param label  Prefix string for output.
 */
void sess_print_relationships(const char *label);

/*
 * sess_demo_new_session()
 *
 * Exercise 9.2: Forks, has child call setsid(), then verifies:
 *   - Child is a process group leader.
 *   - Child has no controlling terminal (tcgetpgrp returns -1).
 * Prints results to stdout.
 */
void sess_demo_new_session(void);

/*
 * sess_open_ctty()
 *
 * Attempts to open /dev/tty (the kernel synonym for the
 * controlling terminal of the current process).
 *
 * Returns open file descriptor on success, -1 if the process
 * has no controlling terminal.
 */
int sess_open_ctty(void);

#endif /* PROC_SESSION_H */
