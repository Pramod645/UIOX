#ifndef SIGNAL_H
#define SIGNAL_H

#include <stdint.h>

/* ── Signal numbers (19 signals in System V) ────────────────── */
#define NSIG            19

#define SIG_NONE        0
#define SIGHUP          1   /* hangup (terminal disconnect)     */
#define SIGINT          2   /* interrupt (Ctrl+C)               */
#define SIGQUIT         3   /* quit (core dump)                 */
#define SIGILL          4   /* illegal instruction              */
#define SIGTRAP         5   /* trace/breakpoint trap            */
#define SIGABRT         6   /* abort (process exception)        */
#define SIGEMT          7   /* emulator trap                    */
#define SIGFPE          8   /* floating point exception         */
#define SIGKILL         9   /* kill (unblockable)               */
#define SIGBUS          10  /* bus error                        */
#define SIGSEGV         11  /* segmentation fault               */
#define SIGSYS          12  /* bad system call                  */
#define SIGPIPE         13  /* broken pipe (no reader)          */
#define SIGALRM         14  /* alarm clock                      */
#define SIGTERM         15  /* software termination             */
#define SIGUSR1         16  /* user-defined signal 1            */
#define SIGUSR2         17  /* user-defined signal 2            */
#define SIGCHLD         18  /* child status changed             */
#define SIGSTOP         19  /* stop process                     */

/* ── Signal classifications ─────────────────────────────────── */
/* Class 1: process termination */
#define SIG_CLASS_TERM      1
/* Class 2: process-induced exceptions */
#define SIG_CLASS_EXCEPT    2
/* Class 3: unrecoverable conditions during syscall */
#define SIG_CLASS_UNRECOV   3
/* Class 4: unexpected error during syscall */
#define SIG_CLASS_SYSERR    4
/* Class 5: user-mode originated signals */
#define SIG_CLASS_USER      5
/* Class 6: terminal interaction */
#define SIG_CLASS_TERM_IO   6
/* Class 7: tracing */
#define SIG_CLASS_TRACE     7

/* ── Signal dispositions ────────────────────────────────────── */
#define SIG_DFL     ((sig_handler_t)0)    /* default action     */
#define SIG_IGN     ((sig_handler_t)1)    /* ignore signal      */

/* ── Signal flags ───────────────────────────────────────────── */
#define SA_RESTART  0x01    /* restart syscall on signal        */
#define SA_NOCLDWAIT 0x02   /* don't create zombie on child exit*/
#define SA_SIGINFO  0x04    /* use sa_sigaction instead         */

/* ── Type definitions ───────────────────────────────────────── */
typedef void (*sig_handler_t)(int signum);

typedef struct sigaction
 {
    sig_handler_t sa_handler;   /* handler function or SIG_DFL/IGN */
    uint32_t      sa_mask;      /* signals to block during handler  */
    int           sa_flags;     /* signal flags                     */
} sigaction_t;

/* ── Per-signal descriptor ──────────────────────────────────── */
typedef struct sig_desc {
    int   sd_num;               /* signal number                */
    int   sd_class;             /* signal class                 */
    int   sd_core;              /* 1 = dump core on default     */
    const char *sd_name;        /* signal name string           */
} sig_desc_t;

extern sig_desc_t sig_table[NSIG + 1];

/* ── Function prototypes ────────────────────────────────────── */
struct proc;
struct u_area;

int  issig(struct proc *p);
void psig(struct proc *p, struct u_area *u);
int  kernel_kill(uint32_t pid, int signum);
int  kernel_signal(int signum, sig_handler_t handler);
void send_signal(struct proc *p, int signum);
int  signal_ignored(struct proc *p, int signum);
int  signal_caught(struct proc *p, int signum,
                   struct u_area *u);
void dump_core(struct proc *p, struct u_area *u);
int  get_signal_class(int signum);

#endif /* SIGNAL_H */
