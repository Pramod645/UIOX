#include "../include/exit_wait.h"
#include "../include/signal.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/* ── close_all_files ─────────────────────────────────────────
 * Close all open file descriptors (internal close algorithm).
 */
void close_all_files(struct u_area *u)
{
    if (!u) return;
    printf("[exit] closing all open files\n");
    /*
     * for i in 0..NOFILE-1:
     *   if u->u_ofile[i]:
     *     fs_close(i)   -- internal variant
     */
}

/* ── release_proc_regions ────────────────────────────────────
 * Free all regions attached to the process (algorithm freereg).
 */
void release_proc_regions(struct proc *p, struct u_area *u)
{
    (void)p;
    if (!u) return;
    printf("[exit] releasing all process regions\n");
    /*
     * for each pregion entry in u->u_pregs:
     *   detachreg(&u->u_pregs[i])
     */
}

/* ── write_acct_record ───────────────────────────────────────
 * Write process accounting information to the accounting file.
 */
void write_acct_record(struct proc *p, struct u_area *u,
                       int exit_code)
{
    if (!p) return;
    acct_record_t rec;
    memset(&rec, 0, sizeof(rec));

    /* rec.ar_pid       = p->p_pid;        */
    /* rec.ar_ppid      = p->p_ppid;       */
    /* rec.ar_uid       = p->p_uid;        */
    /* rec.ar_utime     = u->u_timer.tr_utime; */
    /* rec.ar_stime     = u->u_timer.tr_stime; */
    rec.ar_exit_code = exit_code;

    printf("[exit] accounting record written: "
           "exit_code=%d\n", exit_code);
    (void)u;
}

/* ── reparent_children ───────────────────────────────────────
 * Assign parent PID of all children to init (pid 1).
 * If any children were already zombie, send SIGCHLD to init.
 */
void reparent_children(struct proc *exiting_proc)
{
    if (!exiting_proc) return;
    int had_zombie = 0;

    /*
     * Scan proc_table:
     *   for each proc where p_ppid == exiting_proc->p_pid:
     *     p->p_ppid = INIT_PID
     *     if p->p_state == PROC_ZOMBIE:
     *       had_zombie = 1
     */

    printf("[exit] reparented children to init (pid=1)\n");

    if (had_zombie) {
        /* send_signal(proc_find(INIT_PID), SIGCHLD); */
        printf("[exit] sent SIGCHLD to init for zombie children\n");
    }
}

/* ─────────────────────────────────────────────────────────────
 * 4. Algorithm exit
 *    input : exit status code
 *    output: never returns
 */
void kernel_exit(int status)
{
    struct proc   *p = NULL;  /* current_proc in real kernel  */
    struct u_area *u = NULL;  /* &u in real kernel            */

    printf("[exit] process exiting with status=%d\n", status);

    /* ── Ignore all signals ────────────────────────────────── */
    /* p->p_sigmask = ~0u;  (block everything) */
    printf("[exit] all signals ignored\n");

    /* ── If process group leader with control terminal:
     *    send SIGHUP to all members, reset their pgid to 0 ── */
    /* if (p->p_pid == p->p_pgid && p->p_ttyp) { */
    /*     for each proc q in group: send_signal(q, SIGHUP);   */
    /*     for each proc q in group: q->p_pgid = 0;            */
    /* } */
    printf("[exit] handled process group leader cleanup\n");

    /* ── Close all open files ──────────────────────────────── */
    close_all_files(u);

    /* ── Release current directory inode (algorithm iput) ─── */
    /* iput(u->u_cdir); u->u_cdir = NULL; */
    printf("[exit] released current directory inode\n");

    /* ── Release changed root inode if applicable ──────────── */
    /* if (u->u_rdir) { iput(u->u_rdir); u->u_rdir = NULL; } */
    printf("[exit] released root inode\n");

    /* ── Free regions and memory (algorithm freereg) ───────── */
    release_proc_regions(p, u);

    /* ── Write accounting record ───────────────────────────── */
    write_acct_record(p, u, status);

    /* ── Make process state zombie ─────────────────────────── */
    /* proc_set_state(p, PROC_ZOMBIE); */
    /* p->p_exit_code = status;        */
    printf("[exit] process state = ZOMBIE\n");

    /* ── Reparent all children to init ─────────────────────── */
    reparent_children(p);

    /* ── Send SIGCHLD to parent ─────────────────────────────── */
    /* send_signal(proc_find(p->p_ppid), SIGCHLD); */
    printf("[exit] sent SIGCHLD to parent\n");

    /* ── Context switch (never returns) ────────────────────── */
    /* proc_t *next = sched_pick();        */
    /* context_switch_proc(p, next);       */
    printf("[exit] context switch (process terminated)\n");
}

/* ─────────────────────────────────────────────────────────────
 * 5. Algorithm wait
 *    input : pointer to status variable
 *    output: child PID and exit code; -1 on error
 */
int kernel_wait(int *status_ptr)
{
    struct proc *p = NULL;  /* current_proc */

    /* If waiting process has no child processes — error */
    int has_children = 0;   /* scan proc_table for p_ppid match */
    (void)p;

    if (!has_children) {
        fprintf(stderr, "[wait] no child processes\n");
        return -1;
    }

    /* Infinite loop until a zombie child is found */
    for (;;) {

        /* ── Check for zombie child ────────────────────────── */
        struct proc *zombie = NULL;
        /*
         * for each proc q where q->p_ppid == p->p_pid:
         *   has_children = 1
         *   if q->p_state == PROC_ZOMBIE:
         *     zombie = q; break
         */

        if (zombie) {
            /* Found a zombie child */
            uint32_t child_pid  = 0; /* zombie->p_pid       */
            int      exit_code  = 0; /* zombie->p_exit_code */

            /* Add child CPU usage to parent */
            /* p->p_timers.p_cutime += zombie->p_timers.p_utime */
            /* p->p_timers.p_cstime += zombie->p_timers.p_stime */
            printf("[wait] collected zombie child pid=%u "
                   "exit_code=%d\n", child_pid, exit_code);

            /* Free child process table entry */
            /* proc_free(zombie); */

            if (status_ptr)
                *status_ptr = exit_code;

            return (int)child_pid;
        }

        /* ── No zombie yet ─────────────────────────────────── */
        if (!has_children) {
            fprintf(stderr, "[wait] no children remain\n");
            return -1;
        }

        /* Sleep at interruptible priority waiting for child exit */
        printf("[wait] sleeping, waiting for child to exit\n");
        /* proc_sleep((uintptr_t)p, PWAIT, 1); */

        /* Woken by SIGCHLD — loop back and check again */
    }
}
