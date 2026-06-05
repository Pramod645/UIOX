#include "../include/proc_session.h"

/*
 * sess_create()
 *
 * Standard setsid() pattern:
 *   fork() → parent exits → child calls setsid().
 *
 * Why fork first?
 *   setsid() fails if the caller is already a process group
 *   leader (EPERM).  Since fork gives the child a new PID ≠ PGID
 *   (it inherits parent's PGID), the child is guaranteed NOT to
 *   be a group leader.
 *
 * After setsid() in the child:
 *   - New session created; child is session leader.
 *   - New process group; child is group leader.
 *   - Controlling terminal association broken.
 *
 * This function is called in the child after fork().
 * Returns new session PGID on success, -1 on error.
 */
pid_t sess_create(void)
{
    pid_t pid;

    pid = fork();
    if (pid < 0) {
        perror("sess_create: fork");
        return -1;
    }

    if (pid > 0) {
        /* Parent exits so child is orphaned and re-parented to init */
        exit(0);
    }

    /* ── Child: call setsid() ───────────────────────────────
     * Guaranteed not a process group leader because child PID
     * differs from inherited PGID.
     */
    pid = setsid();
    if (pid < 0) {
        perror("sess_create: setsid");
        return -1;
    }

    /* pid now == new session ID == new PGID == caller's PID */
    return pid;
}

/*
 * sess_get_sid()
 *
 * getsid(pid) returns the process group ID of the session leader
 * of the session that contains process pid.
 *
 * Since the session leader always leads its own process group,
 * this value is simultaneously:
 *   - the session ID (SVR4 / Linux extension)
 *   - the PGID of the session leader
 *
 * If pid == 0, returns session leader PGID for the calling process.
 */
pid_t sess_get_sid(pid_t pid)
{
    pid_t sid = getsid(pid);
    if (sid < 0)
        perror("sess_get_sid: getsid");
    return sid;
}

/*
 * sess_get_foreground_pgid()
 *
 * tcgetpgrp(fd) returns the PGID of the foreground process group
 * associated with the terminal open on fd.
 *
 * The terminal driver uses this value to deliver:
 *   SIGINT, SIGQUIT, SIGTSTP — to the foreground group.
 *   Terminal input             — to the foreground group.
 *
 * Background process groups receive SIGTTIN if they try to read,
 * and optionally SIGTTOU if TOSTOP flag is set and they write.
 *
 * Returns PGID of foreground group, or -1 on error.
 */
pid_t sess_get_foreground_pgid(int fd)
{
    pid_t pgid = tcgetpgrp(fd);
    if (pgid < 0)
        perror("sess_get_foreground_pgid: tcgetpgrp");
    return pgid;
}

/*
 * sess_set_foreground_pgid()
 *
 * tcsetpgrp(fd, pgid) sets the foreground process group for the
 * terminal on fd.
 *
 * Requirements:
 *   - fd must be the controlling terminal of the calling process.
 *   - pgid must be the PGID of a group in the same session.
 *
 * Called by job-control shells when:
 *   - Bringing a background job to the foreground (fg).
 *   - Restoring the shell as foreground after a job finishes.
 *
 * Returns 0 on success, -1 on error.
 */
int sess_set_foreground_pgid(int fd, pid_t pgid)
{
    if (tcsetpgrp(fd, pgid) < 0) {
        perror("sess_set_foreground_pgid: tcsetpgrp");
        return -1;
    }
    return 0;
}

/*
 * sess_get_ctty_sid()
 *
 * tcgetsid(fd) returns the PGID of the session leader for the
 * controlling terminal associated with fd.
 *
 * Equivalent to getsid(0) if fd is the caller's controlling
 * terminal.  Useful when an application manages controlling
 * terminals and needs to find which session owns one.
 *
 * Returns session leader PGID, or -1 on error.
 */
pid_t sess_get_ctty_sid(int fd)
{
    pid_t sid = tcgetsid(fd);
    if (sid < 0)
        perror("sess_get_ctty_sid: tcgetsid");
    return sid;
}

/*
 * sess_print_relationships()
 *
 * Prints a comprehensive view of the calling process's position
 * in the process group / session hierarchy.
 */
void sess_print_relationships(const char *label)
{
    pid_t fg_pgid = -1;
    int   ctty_fd;

    /* Try to get foreground PGID from controlling terminal */
    ctty_fd = open("/dev/tty", O_RDONLY);
    if (ctty_fd >= 0) {
        fg_pgid = tcgetpgrp(ctty_fd);
        close(ctty_fd);
    }

    printf("%s: pid=%ld  ppid=%ld  pgid=%ld  sid=%ld  fg_pgid=%ld\n",
           label ? label : "",
           (long)getpid(),
           (long)getppid(),
           (long)getpgrp(),
           (long)getsid(0),
           (long)fg_pgid);
    fflush(stdout);
}

/*
 * sess_demo_new_session()
 *
 * Exercise 9.2 from APUE:
 *   "Write a small program that calls fork and has the child
 *    create a new session. Verify that the child becomes a
 *    process group leader and that the child no longer has
 *    a controlling terminal."
 *
 * Steps:
 *   1. Print parent info.
 *   2. fork().
 *   3. Parent waits.
 *   4. Child calls setsid().
 *   5. Child prints new info: should show PID == PGID == SID.
 *   6. Child tries tcgetpgrp(STDIN_FILENO) → should fail with
 *      ENOTTY (no controlling terminal).
 */
void sess_demo_new_session(void)
{
    pid_t pid, sid;

    sess_print_relationships("parent before fork");

    pid = fork();
    if (pid < 0) {
        perror("sess_demo_new_session: fork");
        return;
    }

    if (pid == 0) {
        /* ── Child ─────────────────────────────────────────
         * Must not be a process group leader before setsid().
         * Since we just forked, child PID ≠ inherited PGID.
         */
        sid = setsid();
        if (sid < 0) {
            perror("child: setsid");
            exit(1);
        }

        sess_print_relationships("child after setsid");

        /* Verify: PID should equal PGID and SID */
        printf("child: PID==PGID ? %s  PID==SID ? %s\n",
               getpid() == getpgrp() ? "YES" : "NO",
               getpid() == getsid(0) ? "YES" : "NO");

        /* Verify: no controlling terminal */
        {
            int ctty = open("/dev/tty", O_RDONLY);
            if (ctty < 0)
                printf("child: no controlling terminal (correct)\n");
            else {
                printf("child: still has controlling terminal (unexpected)\n");
                close(ctty);
            }
        }

        /* Verify tcgetpgrp fails */
        {
            pid_t fg = tcgetpgrp(STDIN_FILENO);
            if (fg < 0)
                printf("child: tcgetpgrp failed: %s (expected)\n",
                       strerror(errno));
            else
                printf("child: tcgetpgrp returned %ld (unexpected)\n",
                       (long)fg);
        }

        exit(0);

    } else {
        /* Parent waits for child */
        waitpid(pid, NULL, 0);
        printf("parent: child %ld finished\n", (long)pid);
    }
}

/*
 * sess_open_ctty()
 *
 * Opens /dev/tty — the kernel's synonym for the calling process's
 * controlling terminal.  Every reference to /dev/tty goes through
 * the vnode structure allocated when the terminal was opened
 * (see Section 9.11, FreeBSD implementation).
 *
 * Returns fd on success, -1 if no controlling terminal exists.
 */
int sess_open_ctty(void)
{
    int fd = open("/dev/tty", O_RDWR);
    if (fd < 0 && errno != ENXIO)
        perror("sess_open_ctty: open /dev/tty");
    return fd;
}
