#include "../include/proc_orphan.h"

/*
 * Module-level signal handler pointer.
 * sig_hup is installed by the child in orphan_run_demo().
 */
static void _sig_hup(int signo)
{
    /*
     * SIGHUP is sent by the kernel to every process in a newly
     * orphaned process group that contains a stopped process.
     * It is followed immediately by SIGCONT.
     *
     * Default action for SIGHUP: terminate.
     * We catch it here so the child can print a message and
     * continue executing.
     */
    printf("SIGHUP received, pid = %ld\n", (long)getpid());
    fflush(stdout);
}

void orphan_sig_hup(int signo)
{
    _sig_hup(signo);
}

/*
 * orphan_pr_ids()
 *
 * Prints the process identification information for the current
 * process.  tcgetpgrp(STDIN_FILENO) returns the PGID of the
 * foreground process group of the calling process's controlling
 * terminal.
 *
 * When the child is orphaned, its parent PID changes to 1 (init)
 * and the terminal PGID changes to the shell's PGID, showing that
 * the child is now in a background (orphaned) process group.
 */
void orphan_pr_ids(const char *name)
{
    printf("%s: pid=%ld  ppid=%ld  pgrp=%ld  tpgrp=%ld\n",
           name,
           (long)getpid(),
           (long)getppid(),
           (long)getpgrp(),
           (long)tcgetpgrp(STDIN_FILENO));
    fflush(stdout);
}

/*
 * orphan_is_orphaned_pgrp()
 *
 * Checks whether the calling process is in an orphaned group.
 *
 * Simplified test: if the calling process's SID equals its
 * parent's SID, they are in the same session, meaning the group
 * may not be orphaned.  If they differ, the parent is in a
 * different session — a necessary (but not sufficient) condition
 * for orphaning.
 *
 * A more complete check would scan all members of the group and
 * verify each parent's session, but that requires /proc or
 * platform-specific APIs.
 *
 * Returns 1 if likely orphaned, 0 if not, -1 on error.
 */
int orphan_is_orphaned_pgrp(void)
{
    pid_t my_sid     = getsid(0);
    pid_t my_ppid    = getppid();
    pid_t parent_sid = getsid(my_ppid);

    if (my_sid < 0 || parent_sid < 0)
        return -1;

    /* If parent is in a different session, group is likely orphaned */
    return (my_sid != parent_sid) ? 1 : 0;
}

/*
 * orphan_run_demo()
 *
 * Figure 9.12 from APUE — Creates an orphaned process group.
 *
 * Detailed walk-through:
 *
 * SETUP:
 *   A job-control shell places this program in process group 6099
 *   (its own PID), while the shell stays in group 2837.
 *   Both are in the same session.
 *
 * AFTER FORK:
 *   Parent (PID 6099): sleeps 5 seconds, then exits.
 *   Child  (PID 6100): inherits process group 6099.
 *
 * CHILD STEPS:
 *   1. orphan_pr_ids("child") — shows ppid=6099, pgrp=6099, tpgrp=6099.
 *   2. signal(SIGHUP, _sig_hup) — catch SIGHUP to stay alive.
 *   3. kill(getpid(), SIGTSTP) — child stops itself.
 *      (Like user pressing Ctrl-Z on a foreground job.)
 *
 * PARENT EXIT:
 *   Parent exits after 5s.
 *   Child is now orphaned: its new parent is init (PID 1).
 *   PID 1 is in a different session → group 6099 is orphaned.
 *   Group 6099 contains a STOPPED process → POSIX.1 §9.10:
 *     kernel sends SIGHUP  to every process in group 6099.
 *     kernel sends SIGCONT to every process in group 6099.
 *
 * CHILD RESUMES:
 *   SIGHUP handler prints "SIGHUP received, pid = 6100".
 *   SIGCONT resumes the child.
 *   Child prints new IDs: ppid now == 1, tpgrp == 2837.
 *   Child tries read(STDIN_FILENO, ...) from the controlling
 *   terminal.  Since it's in an orphaned background group, the
 *   kernel cannot send SIGTTIN (would never be continued).
 *   POSIX.1: read() returns -1, errno = EIO.
 *
 * Expected output:
 *   parent: pid=6099, ppid=2837, pgrp=6099, tpgrp=6099
 *   child:  pid=6100, ppid=6099, pgrp=6099, tpgrp=6099
 *   [shell prompt appears here — parent has exited]
 *   SIGHUP received, pid = 6100
 *   child:  pid=6100, ppid=1,    pgrp=6099, tpgrp=2837
 *   read error 5 on controlling TTY
 */
void orphan_run_demo(void)
{
    char  c;
    pid_t pid;

    /* Print parent's process relationships */
    orphan_pr_ids("parent");

    pid = fork();
    if (pid < 0) {
        perror("orphan_run_demo: fork");
        return;
    }

    if (pid > 0) {
        /* ── Parent ─────────────────────────────────────────
         * Sleep 5 seconds to give child time to stop itself,
         * then exit.  This makes the child's group orphaned.
         */
        sleep(5);
        /* Parent exits — child's PPID becomes 1 */
        exit(0);

    } else {
        /* ── Child ──────────────────────────────────────────
         *
         * Step 1: Print child's initial process relationships.
         * At this point ppid == original parent's PID.
         */
        orphan_pr_ids("child");

        /*
         * Step 2: Install SIGHUP handler.
         * Without this, the default action (terminate) would
         * kill the child when the orphan mechanism fires SIGHUP.
         * We install the handler to catch SIGHUP, print a message,
         * and continue executing.
         */
        if (signal(SIGHUP, _sig_hup) == SIG_ERR) {
            perror("child: signal SIGHUP");
            exit(1);
        }

        /*
         * Step 3: Send SIGTSTP to ourselves — stop the child.
         * This simulates pressing Ctrl-Z to suspend a process.
         * The child will remain stopped until SIGCONT is received.
         *
         * SIGTSTP vs SIGSTOP:
         *   SIGTSTP can be caught/ignored; SIGSTOP cannot.
         *   We use SIGTSTP here as the text shows.
         */
        if (kill(getpid(), SIGTSTP) < 0) {
            perror("child: kill SIGTSTP");
            exit(1);
        }

        /*
         * Execution resumes here after SIGCONT is received.
         * This happens because:
         *   1. Parent exited → child's group became orphaned.
         *   2. Orphaned group contained a stopped process.
         *   3. Kernel sent SIGHUP + SIGCONT to the group.
         *   4. SIGHUP handler ran (printed message).
         *   5. SIGCONT resumed the child from SIGTSTP state.
         *
         * Step 4: Print updated process relationships.
         * ppid should now be 1 (init).
         * tpgrp should now be the shell's PGID (not ours).
         */
        orphan_pr_ids("child");

        /*
         * Step 5: Try to read from controlling terminal.
         *
         * The child is now in an orphaned, background process
         * group.  If the kernel sent SIGTTIN to stop the child
         * for reading from the terminal, the child could never
         * be continued (nobody to send SIGCONT to it now that
         * it's orphaned from its session's foreground jobs).
         *
         * POSIX.1 §9.10 resolution:
         *   Instead of stopping the child with SIGTTIN, the
         *   read() returns -1 with errno = EIO.
         *   This allows the application to detect the condition
         *   and exit cleanly rather than hanging forever.
         */
        if (read(STDIN_FILENO, &c, 1) != 1)
            printf("read error %d on controlling TTY\n", errno);

        exit(0);
    }
}
