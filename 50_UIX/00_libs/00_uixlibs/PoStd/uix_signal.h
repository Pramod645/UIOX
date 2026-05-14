
#ifndef __UIX_SIGNAL__H
#define __UIX_SIGNAL__H
/*
signal.h header is one of the core UNIX/POSIX headers.  
It defines constants, types, and functions used to handle asynchronous signals — events like interrupts, 
segmentation faults, or user-triggered actions (e.g. pressing Ctrl‑C).  

*/
/* This is for only POXIS */

//#include "features.h"

#include "../sys/uix_types.h"

#define UIX_SIGHUP    1   // Hangup — sent when terminal disconnects
#define UIX_SIGINT    2   // Interrupt — sent by Ctrl+C
#define UIX_SIGQUIT   3   // Quit — sent by Ctrl+\ , generates core dump
#define UIX_SIGILL    4    // Kill — cannot be caught or ignored
#define UIX_SIGTRAP   5
#define UIX_SIGABRT   6
#define UIX_SIGBUS    7
#define UIX_SIGFPE    8
#define UIX_SIGKILL   9
#define UIX_SIGUSR1   10
#define UIX_SIGSEGV   11   // Segmentation fault — invalid memory access
#define UIX_SIGUSR2   12
#define UIX_SIGPIPE   13   // Broken pipe — write to closed reader
#define UIX_SIGALRM   14   // Alarm — sent by alarm() or setitimer()
#define UIX_SIGTERM   15   // Termination request — default kill signal
#define UIX_SIGSTKFLT 16
#define UIX_SIGCHLD   17  // Child stopped or exited
#define UIX_SIGCONT   18
#define UIX_SIGSTOP   19
#define UIX_SIGTSTP   20
#define UIX_SIGTTIN   21
#define UIX_SIGTTOU   22
#define UIX_SIGURG    23
#define UIX_SIGXCPU   24
#define UIX_SIGXFSZ   25
#define UIX_SIGVTALRM 26
#define UIX_SIGPROF   27
#define UIX_SIGWINCH  28
#define UIX_SIGIO     29
#define UIX_SIGPWR    30
#define UIX_SIGSYS    31
#define UIX_NSIG      32

typedef void (*uix_sighandler_t)(int);

#define UIX_SIG_DFL ((uix_sighandler_t)0)   // Default signal action
#define UIX_SIG_IGN ((uix_sighandler_t)1)   // Ignore signal
#define UIX_SIG_ERR ((uix_sighandler_t)-1)

#define UIX_SA_NOCLDSTOP  0x00000001
#define UIX_SA_NOCLDWAIT  0x00000002
#define UIX_SA_SIGINFO    0x00000004
#define UIX_SA_ONSTACK    0x08000000
#define UIX_SA_RESTART    0x10000000  // Restart interrupted syscalls — important for robustness
#define UIX_SA_NODEFER    0x40000000
#define UIX_SA_RESETHAND  0x80000000

#define UIX_SIG_BLOCK   0  // Add signals to blocked mask
#define UIX_SIG_UNBLOCK 1  // Remove signals from blocked mask
#define UIX_SIG_SETMASK 2  // Replace blocked mask entirely

typedef uix_uint64_t uix_sigset_t;

typedef struct uix_sigaction {
    uix_sighandler_t sa_handler;
    uix_sigset_t     sa_mask;
    int              sa_flags;
} uix_sigaction_t;

uix_sighandler_t uix_signal      (int signum, uix_sighandler_t handler); // Install signal handler — POSIX (use sigaction() for portability)
int              uix_sigaction   (int signum, const uix_sigaction_t *act,
                                   uix_sigaction_t *oldact);  // Full signal handler installation — POSIX.1-2001
int              uix_kill        (uix_pid_t pid, int sig);  // Send signal to process or group
int              uix_raise       (int sig);   // Send signal to current process
int              uix_sigemptyset (uix_sigset_t *set);   // Initializes signal set to empty
int              uix_sigfillset  (uix_sigset_t *set);   // Initializes signal set to full
int              uix_sigaddset   (uix_sigset_t *set, int signum);  // Adds signal to set
int              uix_sigdelset   (uix_sigset_t *set, int signum);
int              uix_sigismember (const uix_sigset_t *set, int signum);
int              uix_sigprocmask (int how, const uix_sigset_t *set,
                                   uix_sigset_t *oldset);  // Block/unblock signals for current thread
int              uix_sigpending  (uix_sigset_t *set);
int              uix_sigsuspend  (const uix_sigset_t *mask);  // Atomically replaces mask and waits for signal



#endif /* End of __UIX_SIGNAL__H */
/* ***This is End of file, there is no more line should be added after this line*** */
