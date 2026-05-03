
#ifndef __UIX_SIGNAL__H
#define __UIX_SIGNAL__H
/*
signal.h header is one of the core UNIX/POSIX headers.  
It defines constants, types, and functions used to handle asynchronous signals — events like interrupts, 
segmentation faults, or user-triggered actions (e.g. pressing Ctrl‑C).  

*/
/* This is for only POXIS */

#include "features.h"

#include <sys/types.h>
#include <time.h>


#if  (define __POSIX  || define __GLIBC)

#ifdef _cplusplus
extern "C" {
#endif

// Type for signal numbers /
typedef int sigatomict;

// Common signal numbers (POSIX-compliant systems define more) /
#define SIGHUP    1   // Hangup (control terminal closed) /
#define SIGINT    2   // Interrupt (Ctrl-C) /
#define SIGQUIT   3   // Quit from keyboard /
#define SIGILL    4   // Illegal instruction /
#define SIGABRT   6   // Abort signal /
#define SIGFPE    8   // Floating-point exception /
#define SIGKILL   9   // Kill signal /
#define SIGSEGV   11  // Invalid memory reference /
#define SIGPIPE   13  // Broken pipe /
#define SIGALRM   14  // Timer signal from alarm() /
#define SIGTERM   15  // Termination signal /
#define SIGUSR1   10  // User-defined signal 1 /
#define SIGUSR2   12  // User-defined signal 2 /
#define SIGCHLD   17  // Child process stopped or terminated /
#define SIGCONT   18  // Continue if stopped /
#define SIGSTOP   19  // Stop process /
#define SIGTSTP   20  // Stop (Ctrl+Z) /
#define SIGTTIN   21  // Background read attempt /
#define SIGTTOU   22  // Background write attempt /

// Signal handler actions /
#define SIGDFL ((void ()(int))0)   // Default action /
#define SIGIGN ((void ()(int))1)   // Ignore signal /
#define SIGERR ((void ()(int))-1)  // Error return /

// Struct used with sigaction /
struct sigaction {
    void     (sahandler)(int);       // simple handler /
    void     (sasigaction)(int, siginfot , void ); // advanced handler /
    sigsett  samask;                 // signals to block during handler /
    int       saflags;                // behavior flags /
    void    (sarestorer)(void);
};

// Key functions /
typedef void (sighandlert)(int);

sighandlert signal(int signum, sighandlert handler);
int raise(int sig);
int kill(pidt pid, int sig);
int sigaction(int signum, const struct sigaction act, struct sigaction oldact);
int sigprocmask(int how, const sigsett set, sigsett oldset);
int sigemptyset(sigsett set);
int sigaddset(sigsett set, int signum);
int sigdelset(sigsett set, int signum);
int sigismember(const sigsett set, int signum);

#ifdef cplusplus
}
#endif


#endif /* End  of POXIS and STDLIB*/

#endif /* End of __UIX_SIGNAL__H */
/* ***This is End of file, there is no more line should be added after this line*** */