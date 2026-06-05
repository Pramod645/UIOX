#ifndef PROC_ORPHAN_H
#define PROC_ORPHAN_H

/*
 * proc_orphan.h — Orphaned process group demonstration.
 *
 * APUE Chapter 9, Section 9.10
 *
 * An orphaned process group is one in which the parent of
 * EVERY member is either:
 *   (a) itself a member of the same group, OR
 *   (b) a member of a DIFFERENT session entirely.
 *
 * Equivalently: the group is NOT orphaned as long as at least
 * one member has a parent that is in a different process group
 * BUT the same session.  Such a parent could restart a stopped
 * child with SIGCONT.
 *
 * When a process group becomes orphaned and contains a
 * STOPPED process, POSIX.1 requires:
 *   1. Send SIGHUP  to every process in the group.
 *   2. Send SIGCONT to every process in the group.
 *
 * This ensures stopped processes aren't stranded indefinitely.
 *
 * If the child in an orphaned group tries to read from the
 * controlling terminal, the read() returns -1 with errno=EIO.
 * (Kernel cannot send SIGTTIN to stop it, since it would
 * never be continued.)
 *
 * Scenario (Figure 9.11 from text):
 *   login shell (PG 2837, session 2837)
 *     └── parent (PG 6099, session 2837) ← foreground job
 *           └── child  (PG 6099, session 2837)
 *
 *   1. Parent forks child (inherits PG 6099).
 *   2. Child establishes SIGHUP handler.
 *   3. Child sends SIGTSTP to itself → stops.
 *   4. Parent sleeps 5s then exits.
 *   5. Child's parent becomes init (PID 1).
 *   6. PG 6099 is now orphaned (parent=1 is in another session).
 *   7. Kernel sends SIGHUP then SIGCONT to PG 6099.
 *   8. Child's SIGHUP handler runs, prints message.
 *   9. Child continues, tries to read from tty → EIO.
 */

#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <termios.h>

/*
 * orphan_run_demo()
 *
 * Reproduces Figure 9.12 from APUE:
 *   - Prints parent's PID, PPID, PGID, and terminal PGID.
 *   - Forks a child.
 *   - Parent sleeps 5 seconds then exits.
 *   - Child:
 *       1. Prints its own PID/PPID/PGID/TPGID.
 *       2. Installs SIGHUP handler (sig_hup).
 *       3. Sends itself SIGTSTP → stops.
 *       4. When continued by POSIX orphan mechanism:
 *          - SIGHUP handler prints message.
 *          - Child prints updated PID/PPID/PGID/TPGID.
 *          - Child tries to read from stdin → gets EIO.
 *          - Child exits.
 */
void orphan_run_demo(void);

/*
 * orphan_pr_ids()
 *
 * Prints process identification information:
 *   "<name>: pid=N, ppid=N, pgrp=N, tpgrp=N"
 *
 * @param name  Label to print before the values.
 */
void orphan_pr_ids(const char *name);

/*
 * orphan_sig_hup()
 *
 * SIGHUP signal handler installed by the child in the demo.
 * Prints: "SIGHUP received, pid = N"
 */
void orphan_sig_hup(int signo);

/*
 * orphan_is_orphaned_pgrp()
 *
 * Checks whether the process group of the calling process is
 * orphaned.
 *
 * A group is orphaned if for every member, its parent is either:
 *   - in the same process group, OR
 *   - in a different session.
 *
 * Simplified check: compares parent's SID to own SID.
 * If parent SID != own SID, group is orphaned from parent's
 * perspective.
 *
 * Returns 1 if orphaned, 0 if not, -1 on error.
 */
int orphan_is_orphaned_pgrp(void);

#endif /* PROC_ORPHAN_H */
