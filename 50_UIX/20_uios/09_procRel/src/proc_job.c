#include "../include/proc_job.h"

/* ── Global job table ───────────────────────────────────── */
job_t job_table[JOB_MAX];
int   job_count = 0;

/* Shell's own PID and PGID — saved at startup */
static pid_t shell_pgid = 0;
static int   shell_terminal = -1;

/* ── Internal helper: save shell info ───────────────────── */
static void ensure_shell_info(void)
{
    if (shell_pgid == 0) {
        shell_pgid     = getpgrp();
        shell_terminal = STDIN_FILENO;
    }
}

/* ── job_add ─────────────────────────────────────────────
 *
 * Adds a job entry to the job table.
 * Job IDs are 1-based, matching shell conventions ([1], [2]...).
 *
 * Returns job ID on success, -1 if table full.
 */
int job_add(pid_t pgid, const char *cmd)
{
    int i;
    for (i = 0; i < JOB_MAX; i++) {
        if (!job_table[i].j_active) {
            job_table[i].j_id     = i + 1;
            job_table[i].j_pgid   = pgid;
            job_table[i].j_state  = JOB_RUNNING;
            job_table[i].j_procs  = NULL;
            job_table[i].j_active = 1;
            strncpy(job_table[i].j_cmd, cmd ? cmd : "",
                    sizeof(job_table[i].j_cmd) - 1);
            job_count++;
            return i + 1;
        }
    }
    return -1;  /* table full */
}

/* ── job_remove ──────────────────────────────────────────
 *
 * Removes a job from the table and frees its proc list.
 */
void job_remove(int job_id)
{
    int idx = job_id - 1;
    proc_info_t *p, *next;

    if (idx < 0 || idx >= JOB_MAX || !job_table[idx].j_active)
        return;

    /* Free process list */
    p = job_table[idx].j_procs;
    while (p) {
        next = p->pi_next;
        free(p);
        p = next;
    }

    memset(&job_table[idx], 0, sizeof(job_t));
    job_count--;
}

/* ── job_find_by_pgid ────────────────────────────────────
 *
 * Searches job table for a job with matching PGID.
 */
job_t *job_find_by_pgid(pid_t pgid)
{
    for (int i = 0; i < JOB_MAX; i++) {
        if (job_table[i].j_active && job_table[i].j_pgid == pgid)
            return &job_table[i];
    }
    return NULL;
}

/* ── job_print_all ───────────────────────────────────────
 *
 * Prints active jobs in the format "[N] + state cmd".
 * This mimics what a job-control shell prints before its prompt.
 */
void job_print_all(void)
{
    for (int i = 0; i < JOB_MAX; i++) {
        if (!job_table[i].j_active) continue;

        const char *state;
        switch (job_table[i].j_state) {
        case JOB_RUNNING: state = "Running";  break;
        case JOB_STOPPED: state = "Stopped";  break;
        case JOB_DONE:    state = "Done";     break;
        default:          state = "Unknown";  break;
        }
        printf("[%d] + %s\t%s\n",
               job_table[i].j_id,
               state,
               job_table[i].j_cmd);
    }
}

/* ── job_update_status ───────────────────────────────────
 *
 * Updates job state based on the status from waitpid().
 *
 * WIFEXITED / WIFSIGNALED → JOB_DONE
 * WIFSTOPPED              → JOB_STOPPED (SIGTSTP from ^Z)
 * WIFCONTINUED            → JOB_RUNNING (SIGCONT from fg/bg)
 */
void job_update_status(pid_t pid, int status)
{
    /* Find which job this process belongs to by scanning
     * job process lists.  In our simplified version, each
     * job has one process whose PID == PGID.              */
    for (int i = 0; i < JOB_MAX; i++) {
        if (!job_table[i].j_active) continue;
        if (job_table[i].j_pgid != pid) continue;

        if (WIFEXITED(status) || WIFSIGNALED(status))
            job_table[i].j_state = JOB_DONE;
        else if (WIFSTOPPED(status))
            job_table[i].j_state = JOB_STOPPED;
        else if (WIFCONTINUED(status))
            job_table[i].j_state = JOB_RUNNING;
        return;
    }
}

/* ── job_check_background ────────────────────────────────
 *
 * Non-blocking check for completed or stopped background jobs.
 * A shell calls this just before printing its prompt.
 *
 * Uses WNOHANG so it does not block.
 * WUNTRACED so stopped jobs (SIGTSTP) are also reported.
 * WCONTINUED so continued jobs (SIGCONT) are reported.
 */
void job_check_background(void)
{
    pid_t pid;
    int   status;

    while ((pid = waitpid(-1, &status,
                          WNOHANG | WUNTRACED | WCONTINUED)) > 0) {
        job_update_status(pid, status);

        job_t *j = job_find_by_pgid(pid);
        if (!j) continue;

        switch (j->j_state) {
        case JOB_DONE:
            printf("\n[%d] + Done\t%s\n", j->j_id, j->j_cmd);
            job_remove(j->j_id);
            break;
        case JOB_STOPPED:
            printf("\n[%d] + Stopped\t%s\n", j->j_id, j->j_cmd);
            break;
        case JOB_RUNNING:
            printf("\n[%d] + Running\t%s\n", j->j_id, j->j_cmd);
            break;
        }
    }
}

/* ── job_restore_shell_tty ───────────────────────────────
 *
 * After a foreground job finishes or stops, the shell must
 * reclaim the controlling terminal.
 *
 * tcsetpgrp(STDIN_FILENO, shell_pgid):
 *   Makes the shell the foreground process group again.
 *   Terminal input and signals now go to the shell.
 */
void job_restore_shell_tty(void)
{
    ensure_shell_info();
    if (shell_terminal >= 0) {
        if (tcsetpgrp(shell_terminal, shell_pgid) < 0)
            perror("job_restore_shell_tty: tcsetpgrp");
    }
}

/* ── job_wait_foreground ─────────────────────────────────
 *
 * Waits for the foreground job (process group pgid) to
 * either terminate or stop.
 *
 * Uses waitpid(-pgid, ...) to wait for any process in group.
 * WUNTRACED: return if process is stopped (SIGTSTP).
 *
 * Returns final exit status of the last waited process.
 */
int job_wait_foreground(pid_t pgid)
{
    pid_t pid;
    int   status = 0, last_status = 0;

    while ((pid = waitpid(-pgid, &status,
                          WUNTRACED | WCONTINUED)) > 0) {
        job_update_status(pid, status);
        last_status = status;

        if (WIFSTOPPED(status)) {
            /* Job stopped — return control to shell */
            break;
        }
        if (WIFEXITED(status) || WIFSIGNALED(status)) {
            /* Process exited — check if more in group */
        }
    }

    return last_status;
}

/* ── job_launch_foreground ───────────────────────────────
 *
 * Full foreground job launch sequence used by job-control shells.
 *
 * 1. fork()
 * 2. Child:
 *    a. setpgid(0, 0)    — child becomes its own group leader
 *    b. tcsetpgrp(stdin, child_pgid) — become foreground
 *    c. Reset signals to defaults (shell may have blocked them)
 *    d. execvp(argv[0], argv)
 *
 * 3. Parent:
 *    a. setpgid(child, child) — race avoidance
 *    b. tcsetpgrp(stdin, child_pgid) — hand off terminal
 *       (parent and child both do this; safe because idempotent)
 *    c. job_wait_foreground(child_pgid) — wait
 *    d. job_restore_shell_tty() — take back terminal
 *
 * Returns exit status.
 */
int job_launch_foreground(char *const argv[])
{
    pid_t pid;
    int   status;

    ensure_shell_info();

    pid = fork();
    if (pid < 0) {
        perror("job_launch_foreground: fork");
        return -1;
    }

    if (pid == 0) {
        /* ── Child ──────────────────────────────────────── */

        /* Step a: create new process group */
        setpgid(0, 0);

        /* Step b: become foreground process group */
        tcsetpgrp(shell_terminal, getpgrp());

        /* Step c: restore default signal dispositions */
        signal(SIGINT,  SIG_DFL);
        signal(SIGQUIT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);
        signal(SIGTTIN, SIG_DFL);
        signal(SIGTTOU, SIG_DFL);
        signal(SIGCHLD, SIG_DFL);

        /* Step d: exec the program */
        execvp(argv[0], argv);
        perror("job_launch_foreground: execvp");
        exit(127);

    } else {
        /* ── Parent ─────────────────────────────────────── */

        /* Race-avoidance: parent also calls setpgid */
        setpgid(pid, pid);

        /* Hand terminal to child's process group */
        tcsetpgrp(shell_terminal, pid);

        /* Wait for foreground job */
        status = job_wait_foreground(pid);

        /* Reclaim terminal */
        job_restore_shell_tty();

        if (WIFSTOPPED(status)) {
            int jid = job_add(pid, argv[0]);
            job_t *j = job_find_by_pgid(pid);
            if (j) j->j_state = JOB_STOPPED;
            printf("\n[%d] + Stopped\t%s\n", jid, argv[0]);
        }

        return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
    }
}

/* ── job_launch_background ───────────────────────────────
 *
 * Launches a background job.  The key difference from foreground:
 *   - Shell does NOT call tcsetpgrp → child group stays background.
 *   - Shell prints "[N] PID" and returns immediately.
 *   - Shell calls job_check_background() to collect status later.
 *
 * Background job that tries to read from terminal:
 *   Terminal driver sends SIGTTIN to background process group.
 *   Default action: stop the job.
 *   Shell will notify user; user uses fg to bring it forward.
 *
 * Background job that tries to write to terminal:
 *   If TOSTOP flag is set: terminal driver sends SIGTTOU → stopped.
 *   If TOSTOP not set: output appears on terminal (mixed with
 *   whatever the user is typing).
 *
 * Returns job ID on success, -1 on error.
 */
int job_launch_background(char *const argv[])
{
    pid_t pid;
    int   job_id;

    ensure_shell_info();

    pid = fork();
    if (pid < 0) {
        perror("job_launch_background: fork");
        return -1;
    }

    if (pid == 0) {
        /* ── Child ──────────────────────────────────────── */

        /* Create own process group — shell stays foreground */
        setpgid(0, 0);

        /* Restore signal defaults */
        signal(SIGINT,  SIG_DFL);
        signal(SIGQUIT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);
        signal(SIGTTIN, SIG_DFL);
        signal(SIGTTOU, SIG_DFL);

        execvp(argv[0], argv);
        perror("job_launch_background: execvp");
        exit(127);

    } else {
        /* ── Parent ─────────────────────────────────────── */

        /* Race avoidance */
        setpgid(pid, pid);

        /* Register job */
        job_id = job_add(pid, argv[0]);

        printf("[%d] %ld\n", job_id, (long)pid);
        return job_id;
    }
}

/* ── job_continue ────────────────────────────────────────
 *
 * Resumes a stopped job by sending SIGCONT to its process group.
 *
 * If foreground==1:
 *   1. tcsetpgrp() → make it the foreground group.
 *   2. kill(-pgid, SIGCONT) → resume.
 *   3. job_wait_foreground() → wait.
 *   4. job_restore_shell_tty() → reclaim terminal.
 *
 * If foreground==0 (bg):
 *   1. kill(-pgid, SIGCONT) → resume in background.
 *   2. Print "[N] cmd &".
 */
void job_continue(job_t *j, int foreground)
{
    if (!j) return;

    j->j_state = JOB_RUNNING;

    if (foreground) {
        /* Make this group the foreground */
        tcsetpgrp(shell_terminal, j->j_pgid);

        /* Resume */
        if (kill(-j->j_pgid, SIGCONT) < 0)
            perror("job_continue: kill SIGCONT");

        /* Wait */
        job_wait_foreground(j->j_pgid);

        /* Reclaim terminal */
        job_restore_shell_tty();

    } else {
        /* Background resume */
        if (kill(-j->j_pgid, SIGCONT) < 0)
            perror("job_continue: kill SIGCONT");
        printf("[%d] %s &\n", j->j_id, j->j_cmd);
    }
}
