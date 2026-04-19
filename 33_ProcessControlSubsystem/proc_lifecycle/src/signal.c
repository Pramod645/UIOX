#include "../include/signal.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* ── Signal descriptor table ─────────────────────────────────── */
sig_desc_t sig_table[NSIG + 1] = {
    { 0,  0,            0, "NONE"   },
    { SIGHUP,   SIG_CLASS_TERM_IO,  0, "SIGHUP"  },
    { SIGINT,   SIG_CLASS_TERM_IO,  0, "SIGINT"  },
    { SIGQUIT,  SIG_CLASS_TERM_IO,  1, "SIGQUIT" },
    { SIGILL,   SIG_CLASS_EXCEPT,   1, "SIGILL"  },
    { SIGTRAP,  SIG_CLASS_TRACE,    1, "SIGTRAP" },
    { SIGABRT,  SIG_CLASS_EXCEPT,   1, "SIGABRT" },
    { SIGEMT,   SIG_CLASS_EXCEPT,   1, "SIGEMT"  },
    { SIGFPE,   SIG_CLASS_EXCEPT,   1, "SIGFPE"  },
    { SIGKILL,  SIG_CLASS_TERM,     0, "SIGKILL" },
    { SIGBUS,   SIG_CLASS_EXCEPT,   1, "SIGBUS"  },
    { SIGSEGV,  SIG_CLASS_EXCEPT,   1, "SIGSEGV" },
    { SIGSYS,   SIG_CLASS_SYSERR,   1, "SIGSYS"  },
    { SIGPIPE,  SIG_CLASS_SYSERR,   0, "SIGPIPE" },
    { SIGALRM,  SIG_CLASS_USER,     0, "SIGALRM" },
    { SIGTERM,  SIG_CLASS_TERM,     0, "SIGTERM" },
    { SIGUSR1,  SIG_CLASS_USER,     0, "SIGUSR1" },
    { SIGUSR2,  SIG_CLASS_USER,     0, "SIGUSR2" },
    { SIGCHLD,  SIG_CLASS_TERM,     0, "SIGCHLD" },
    { SIGSTOP,  SIG_CLASS_TERM_IO,  0, "SIGSTOP" }
};

/* ── Simulated process signal state ─────────────────────────── */
typedef struct sim_proc {
    uint32_t      sp_pid;
    uint32_t      sp_sig;           /* pending signal bitmask   */
    uint32_t      sp_sigmask;       /* blocked signal bitmask   */
    uint32_t      sp_pgid;          /* process group ID         */
    sigaction_t   sp_sigact[NSIG+1];/* per-signal dispositions  */
    int           sp_zombie_children;
} sim_proc_t;

/* ── signal_ignored ──────────────────────────────────────────
 * Return 1 if the process is ignoring the given signal.
 */
int signal_ignored(struct proc *p, int signum)
{
    if (!p || signum <= 0 || signum > NSIG) return 0;
    sim_proc_t *sp = (sim_proc_t *)p;
    return sp->sp_sigact[signum].sa_handler == SIG_IGN;
}

/* ── signal_caught ───────────────────────────────────────────
 * Return 1 if the process has installed a custom handler.
 */
int signal_caught(struct proc *p, int signum,
                  struct u_area *u)
{
    (void)u;
    if (!p || signum <= 0 || signum > NSIG) return 0;
    sim_proc_t *sp = (sim_proc_t *)p;
    sig_handler_t h = sp->sp_sigact[signum].sa_handler;
    return (h != SIG_DFL && h != SIG_IGN);
}

/* ── get_signal_class ────────────────────────────────────────
 * Return the classification of a signal number.
 */
int get_signal_class(int signum)
{
    if (signum <= 0 || signum > NSIG) return 0;
    return sig_table[signum].sd_class;
}

/* ── send_signal ─────────────────────────────────────────────
 * Set the signal bit in the process's pending signal field.
 */
void send_signal(struct proc *p, int signum)
{
    if (!p || signum <= 0 || signum > NSIG) return;
    sim_proc_t *sp = (sim_proc_t *)p;
    sp->sp_sig |= (1u << signum);
    printf("[signal] sent %s (%d) to pid=%u\n",
           sig_table[signum].sd_name, signum, sp->sp_pid);
}

/* ─────────────────────────────────────────────────────────────
 * 2. Algorithm issig
 *    input : process pointer
 *    output: 1 if process has unignored pending signal, 0 otherwise
 */
int issig(struct proc *p)
{
    if (!p) return 0;
    sim_proc_t *sp = (sim_proc_t *)p;

    /* While received signal field is not zero */
    while (sp->sp_sig != 0) {

        /* Find a signal number sent to the process */
        int signum = 0;
        for (int i = 1; i <= NSIG; i++) {
            if (sp->sp_sig & (1u << i)) {
                signum = i;
                break;
            }
        }
        if (signum == 0) break;

        if (signum == SIGCHLD) {
            /* Signal is death of child */
            if (signal_ignored(p, SIGCHLD)) {
                /* Ignoring SIGCHLD: free zombie children */
                printf("[issig] ignoring SIGCHLD, "
                       "freeing zombie children\n");
                sp->sp_zombie_children = 0;
                /* Turn off signal bit */
                sp->sp_sig &= ~(1u << signum);
                continue;
            } else if (signal_caught(p, SIGCHLD, NULL)) {
                return 1;
            }
        } else {
            /* Not ignoring the signal */
            if (!signal_ignored(p, signum))
                return 1;
        }

        /* Turn off signal bit in received signal field */
        sp->sp_sig &= ~(1u << signum);
    }

    return 0;
}

/* ── dump_core ───────────────────────────────────────────────
 * Create a file named "core" in the current directory and
 * write the user-level context into it.
 */
void dump_core(struct proc *p, struct u_area *u)
{
    (void)p; (void)u;
    FILE *f = fopen("core", "wb");
    if (!f) {
        fprintf(stderr, "[psig] could not create core file\n");
        return;
    }
    /* In real kernel: write u area + register context to file */
    fprintf(f, "CORE DUMP\n");
    fclose(f);
    printf("[psig] core dump written\n");
}

/* ─────────────────────────────────────────────────────────────
 * 3. Algorithm psig
 *    input : process pointer, u area pointer
 *    output: none
 *
 *    Handle a signal after issig has confirmed one is pending.
 */
void psig(struct proc *p, struct u_area *u)
{
    if (!p || !u) return;
    sim_proc_t *sp = (sim_proc_t *)p;

    /* Get signal number set in process table entry */
    int signum = 0;
    for (int i = 1; i <= NSIG; i++) {
        if (sp->sp_sig & (1u << i)) {
            signum = i;
            break;
        }
    }
    if (signum == 0) return;

    /* Clear signal number in process table entry */
    sp->sp_sig &= ~(1u << signum);
    printf("[psig] handling signal %s (%d) for pid=%u\n",
           sig_table[signum].sd_name, signum, sp->sp_pid);

    /* If user called signal() to ignore this signal — done */
    if (signal_ignored(p, signum)) {
        printf("[psig] signal ignored, returning\n");
        return;
    }

    /* If user specified a function to handle the signal */
    if (signal_caught(p, signum, u)) {
        sig_handler_t handler =
            sp->sp_sigact[signum].sa_handler;

        printf("[psig] dispatching to user handler at %p\n",
               (void *)(uintptr_t)handler);

        /*
         * Get user virtual address of signal catcher from u area.
         * Clear u area entry (one-shot unless SA_RESTART).
         * Artificially create user stack frame to mimic a call
         * to the signal catcher function.
         * Modify system-level context: write signal catcher
         * address into program counter field of saved register
         * context so the catcher runs on return to user mode.
         */

        /* Clear handler (System V default: reset to SIG_DFL) */
        sp->sp_sigact[signum].sa_handler = SIG_DFL;

        /* Simulate: call the handler directly */
        handler(signum);
        return;
    }

    /* Default action: dump core if appropriate */
    if (sig_table[signum].sd_core) {
        printf("[psig] default action: dump core\n");
        dump_core(p, u);
    } else {
        printf("[psig] default action: terminate process\n");
    }

    /* Invoke exit algorithm immediately */
    kernel_exit(EXIT_SIGNAL(signum));
}

/* ─────────────────────────────────────────────────────────────
 * kernel_kill
 * Send signal signum to process with given PID.
 * Called by the kill(2) system call.
 */
int kernel_kill(uint32_t pid, int signum)
{
    if (signum <= 0 || signum > NSIG) return -1;

    /* Find target process (simulated) */
    printf("[kill] sending %s (%d) to pid=%u\n",
           sig_table[signum].sd_name, signum, pid);
    /* send_signal(proc_find(pid), signum); */
    return 0;
}

/* ─────────────────────────────────────────────────────────────
 * kernel_signal
 * Set the disposition of signal signum for the current process.
 */
int kernel_signal(int signum, sig_handler_t handler)
{
    if (signum <= 0 || signum > NSIG) return -1;
    if (signum == SIGKILL || signum == SIGSTOP) {
        fprintf(stderr, "[signal] cannot catch/ignore "
                "SIGKILL or SIGSTOP\n");
        return -1;
    }
    printf("[signal] setting handler for %s (%d)\n",
           sig_table[signum].sd_name, signum);
    /* current_proc sigact[signum].sa_handler = handler; */
    return 0;
}
