#ifndef SIGNAL_H
#define SIGNAL_H
#include "uiox_klibc.h"

#define NSIG            19
#define SIG_NONE        0
#define SIGHUP          1
#define SIGINT          2
#define SIGQUIT         3
#define SIGILL          4
#define SIGTRAP         5
#define SIGABRT         6
#define SIGEMT          7
#define SIGFPE          8
#define SIGKILL         9
#define SIGBUS          10
#define SIGSEGV         11
#define SIGSYS          12
#define SIGPIPE         13
#define SIGALRM         14
#define SIGTERM         15
#define SIGUSR1         16
#define SIGUSR2         17
#define SIGCHLD         18
#define SIGSTOP         19

#define SIG_CLASS_TERM      1
#define SIG_CLASS_EXCEPT    2
#define SIG_CLASS_UNRECOV   3
#define SIG_CLASS_SYSERR    4
#define SIG_CLASS_USER      5
#define SIG_CLASS_TERM_IO   6
#define SIG_CLASS_TRACE     7

typedef void (*sig_handler_t)(int signum);
#define SIG_DFL  ((sig_handler_t)0)
#define SIG_IGN  ((sig_handler_t)1)

#define SA_RESTART    0x01
#define SA_NOCLDWAIT  0x02
#define SA_SIGINFO    0x04

typedef struct sigaction {
    sig_handler_t sa_handler;
    uint32_t      sa_mask;
    int           sa_flags;
} sigaction_t;

typedef struct sig_desc {
    int         sd_num;
    int         sd_class;
    int         sd_core;
    const char *sd_name;
} sig_desc_t;

extern sig_desc_t sig_table[NSIG + 1];

struct proc;
struct u_area;

int  issig        (struct proc *p);
void psig         (struct proc *p, struct u_area *u);
int  kernel_kill  (uint32_t pid, int signum);
int  kernel_signal(int signum, sig_handler_t handler);
void send_signal  (struct proc *p, int signum);
int  signal_ignored(struct proc *p, int signum);
int  signal_caught (struct proc *p, int signum, struct u_area *u);
void dump_core    (struct proc *p, struct u_area *u);
int  get_signal_class(int signum);

#endif /* SIGNAL_H */
