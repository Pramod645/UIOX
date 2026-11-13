/*
 * 50_UIX/20_uios/uix_uios.c
 *
 * PID 1 — the init process.
 *
 * The kernel mounts a root and execs /bin/uios; everything after that is
 * policy, and policy lives in userspace.  This file is the SUPERVISOR,
 * not the TABLE: the service list is uios_apps.c.
 *
 * A kernel thread that returns is a bug, and the same is true here.  PID
 * 1 exiting leaves the system with nothing to reap orphans or respawn a
 * service, so main() never returns.
 */
#include "uix_uios.h"

#define UIOS_SHUTDOWN_TRIES  100

static void apply_policy(uix_pid_t pid, int policy, int priority)
{
    uix_sched_param_t p;
    p.sched_priority = priority;
    (void)sys_sched_setscheduler(pid, policy, &p);
}

/* ── start one app ───────────────────────────────────────────────────
 * Returns the child pid on success, 0 on fork failure.  The exec-failure
 * code is 127 so init can tell "could not exec" from a service that ran
 * and returned 127 itself. */
static uix_pid_t start_app(uios_app_t *a)
{
    uix_pid_t pid = sys_fork();

    if (pid == 0) {
        char *argv[] = { (char *)a->ua_path, (char *)0 };

        apply_policy(sys_getpid(), a->ua_policy, a->ua_priority);

        /* sys_execv → SYS_EXECV = 250, the number the kernel dispatch
         * handles (uiox_kix_scpcs_dispatch.c:605, and
         * uiox_kix_scpcs_numbers.h:51).
         *
         * sys_execve sends SYS_EXECVE = 340 (uix_sys.h:275, wrapper
         * :620), which has NO case in that table — the trap falls to
         * `default:` and returns -1.  Two names, one letter apart, two
         * different numbers. */
        sys_execv(a->ua_path, argv);

        sys_exit(127);          /* execv returned — it failed */
        //sys_execve(a->ua_path, argv, (char *const *)0);

        //sys_exit(127);          /* execve returned — it failed */
    }

    if (pid > 0)
        apply_policy(pid, a->ua_policy, a->ua_priority);

    return pid > 0 ? pid : 0;
}

/* ── dependency gate ─────────────────────────────────────────────────
 * A dependent starts only once its prerequisite has been started AND has
 * survived UIOS_READY_TICKS passes.  A prerequisite that has exited or
 * churned through restarts never unlocks its dependents — which is the
 * honest answer: if the fileserver cannot stay up, the fstest has
 * nothing to test and starting it would only produce a confusing
 * failure three layers away. */
static int dep_ready(const uios_app_t *a)
{
    const uios_app_t *d;

    if (a->ua_dep == UIOS_NO_DEP) return 1;
    if (a->ua_dep >= UIOS_MAX_APPS) return 1;   /* bad table entry: do not
                                                 * hold the system hostage */
    d = &g_apps[a->ua_dep];

    return d->ua_pid > 0 && d->ua_ticks >= UIOS_READY_TICKS;
}

void uios_init(void)
{
    int i;
    for (i = 0; i < UIOS_MAX_APPS; i++) {
        g_apps[i].ua_pid       = -1;    /* idle — never started        */
        g_apps[i].ua_exit_code = -1;
        g_apps[i].ua_restarts  = 0;
        g_apps[i].ua_ticks     = 0;
    }
}

/* Start everything whose dependency is satisfied.  Called once, then
 * again from the reaper loop for anything still idle. */
void uios_launch_apps(void)
{
    int i;

    for (i = 0; i < UIOS_MAX_APPS; i++) {
        uios_app_t *a = &g_apps[i];

        if (a->ua_pid != -1) continue;      /* running, or already reaped */
        if (!dep_ready(a))   continue;      /* prerequisite not up yet    */

        a->ua_pid = start_app(a);
        if (a->ua_pid > 0)
            a->ua_ticks = 0;
        /* start_app returning 0 means fork failed.  ua_pid becomes 0,
         * which the loop below treats as "not running" — see the
         * fork-failure branch there. */
    }
}

/* ── the reaper ──────────────────────────────────────────────────────
 * Polls every live child, collects exits, applies the restart policy,
 * and starts any dependent whose prerequisite has now proven stable.
 * Never returns: PID 1 outlives every service it starts. */
void uios_run(void)
{
    int i;

    for (;;) {
        int    alive    = 0;
        int    todo     = 0;

        for (i = 0; i < UIOS_MAX_APPS; i++) {
            uios_app_t *a = &g_apps[i];

            /* still idle: dependency may have become ready */
            if (a->ua_pid == -1) {
                if (dep_ready(a)) { a->ua_pid = -1; todo++; }
                continue;
            }

            /* pid == 0: gone.  Apply the restart policy. */
            if (a->ua_pid == 0) {
                const uios_restart_t pol = a->ua_restart;
                const int failed = (a->ua_exit_code != 0);

                if (pol == UIOS_RESTART || (pol == UIOS_RESTART_CR && failed)) {
                    a->ua_restarts++;
                    a->ua_pid    = start_app(a);
                    a->ua_ticks  = 0;
                    if (a->ua_pid > 0) { alive++; }
                    else               { a->ua_pid = 0; }
                }
                continue;
            }

            /* live: poll it */
            {
                int       status = 0;
                uix_pid_t r = sys_wait4(a->ua_pid, &status,
                                        UIX_WNOHANG, (void *)0);

                if (r == a->ua_pid) {
                    a->ua_exit_code = status;
                    a->ua_pid       = 0;
                } else if (r == 0) {
                    a->ua_ticks++;
                    alive++;
                } else {
                    a->ua_pid = 0;      /* names nothing reapable */
                }
            }
        }

        if (alive == 0 && todo == 0)
            return;                      /* nothing running, nothing left to
                                          * start — a deliberate stop */

        /* start anything that became eligible this pass */
        if (todo) uios_launch_apps();

        sys_sched_yield();
    }
}

/* Stop every live app, then reap it — bounded, so a child that ignores
 * the signal cannot hang init. */
void uios_shutdown(void)
{
    int i, tries;

    for (i = 0; i < UIOS_MAX_APPS; i++) {
        if (g_apps[i].ua_pid <= 0) continue;

        if (sys_kill(g_apps[i].ua_pid, 15) != 0) {
            /* no delivery (ENOSYS, or no such process) — still try to
             * reap; the child may have exited on its own. */
        }

        for (tries = 0; tries < UIOS_SHUTDOWN_TRIES; tries++) {
            int       st = 0;
            uix_pid_t r  = sys_wait4(g_apps[i].ua_pid, &st,
                                     UIX_WNOHANG, (void *)0);
            if (r == g_apps[i].ua_pid) {
                g_apps[i].ua_exit_code = st;
                break;
            }
            if (r < 0) break;
            sys_sched_yield();
        }

        g_apps[i].ua_pid = 0;
    }
}

/* One trap, no syscall path in between: proves EL0 -> EL1 -> router
 * assembles and executes.  Keep until the boot is trusted. */
__attribute__((noinline)) void uios_syscall_selftest(void)
{
    volatile long r = my_syscall0(SYS_GETPID);
    (void)r;
}

int main(void)
{
    uios_syscall_selftest();
    uios_init();
    uios_launch_apps();
    uios_run();
    uios_shutdown();

    /* PID 1 does not exit.  A deliberate halt or reboot is an action,
     * not a fall-through. */
    for (;;) {
        sys_sched_yield();
    }
}
