#include "../include/proc_group.h"

/*
 * pg_print_info()
 *
 * Prints PID, PPID, PGID, SID.
 * Uses getpid(), getppid(), getpgrp(), getsid(0).
 */
void pg_print_info(const char *label)
{
    printf("%s: pid=%ld  ppid=%ld  pgid=%ld  sid=%ld\n",
           label ? label : "",
           (long)getpid(),
           (long)getppid(),
           (long)getpgrp(),
           (long)getsid(0));
    fflush(stdout);
}

/*
 * pg_create_new_group()
 *
 * Makes the calling process a process group leader.
 * setpgid(0, 0):
 *   - pid  = 0 → use caller's PID
 *   - pgid = 0 → use caller's PID as new PGID
 *
 * A process group leader has PID == PGID.
 *
 * Returns 0 on success, -1 on error.
 */
int pg_create_new_group(void)
{
    if (setpgid(0, 0) < 0) {
        perror("pg_create_new_group: setpgid");
        return -1;
    }
    return 0;
}

/*
 * pg_join_group()
 *
 * Moves process pid into process group pgid.
 *
 * Restrictions (POSIX):
 *   1. pid must be self or an unexec'd child.
 *   2. pgid must be an existing group in the same session, or
 *      pgid == pid (create new group).
 *
 * Returns 0 on success, -1 on error.
 */
int pg_join_group(pid_t pid, pid_t pgid)
{
    if (setpgid(pid, pgid) < 0) {
        perror("pg_join_group: setpgid");
        return -1;
    }
    return 0;
}

/*
 * pg_is_leader()
 *
 * A process group leader has getpid() == getpgrp().
 *
 * Returns 1 if caller is group leader, 0 otherwise.
 */
int pg_is_leader(void)
{
    return getpid() == getpgrp();
}

/*
 * pg_demo_fork_setpgid()
 *
 * Demonstrates race-condition-free process group assignment.
 *
 * The problem:
 *   After fork(), if only the parent calls setpgid(child, child),
 *   there is a window where the child runs before setpgid,
 *   so the child briefly belongs to the wrong group.
 *   Similarly, if only the child calls setpgid(0,0), the parent
 *   may try to send signals to the child's (old) group too early.
 *
 * Solution:
 *   Both parent AND child call setpgid.  One call will fail
 *   with EACCES or EPERM (child already exec'd), but that is OK.
 *   The first to execute guarantees the child is in the new group.
 *
 * Output:
 *   Shows that child has its own process group (PID == PGID).
 */
void pg_demo_fork_setpgid(void)
{
    pid_t pid;

    pg_print_info("before fork");

    pid = fork();
    if (pid < 0) {
        perror("pg_demo_fork_setpgid: fork");
        return;
    }

    if (pid == 0) {
        /* ── Child ──────────────────────────────────────────
         * Step 1 (child side): set own PGID.
         * Even if parent already did it, this is safe.
         */
        setpgid(0, 0);
        pg_print_info("child after setpgid");
        printf("child is_leader=%d\n", pg_is_leader());
        exit(0);

    } else {
        /* ── Parent ─────────────────────────────────────────
         * Step 1 (parent side): set child's PGID.
         * Race condition avoided: both sides set it.
         */
        setpgid(pid, pid);

        /* Wait for child */
        waitpid(pid, NULL, 0);
        pg_print_info("parent after child exits");
    }
}
