#include "uix_signal.h"
#include "uix_errno.h"
#include "uix_string.h"

static uix_sigaction_t sig_handlers[UIX_NSIG + 1];

uix_sighandler_t uix_signal(int signum, uix_sighandler_t handler)
{
    if (signum <= 0 || signum > UIX_NSIG)
        { uix_errno = UIX_EINVAL; return UIX_SIG_ERR; }
    uix_sighandler_t old = sig_handlers[signum].sa_handler;
    sig_handlers[signum].sa_handler = handler;
    return old;
}

int uix_sigaction(int signum, const uix_sigaction_t *act,
                  uix_sigaction_t *oldact)
{
    if (signum <= 0 || signum > UIX_NSIG)
        { uix_errno = UIX_EINVAL; return -1; }
    if (signum == UIX_SIGKILL || signum == UIX_SIGSTOP)
        { uix_errno = UIX_EINVAL; return -1; }
    if (oldact) *oldact = sig_handlers[signum];
    if (act)    sig_handlers[signum] = *act;
    return 0;
}

int uix_kill(uix_pid_t pid, int sig)
{
    extern int sys_kill(uix_pid_t, int) __attribute__((weak));
    return sys_kill ? sys_kill(pid, sig)
                    : (uix_errno = UIX_EPERM, -1);
}

int uix_raise(int sig)
{
    extern uix_pid_t sys_getpid(void) __attribute__((weak));
    uix_pid_t pid = sys_getpid ? (uix_pid_t)sys_getpid() : 1;
    return uix_kill(pid, sig);
}

int uix_sigemptyset(uix_sigset_t *set)
    { if (!set) return -1; *set = 0; return 0; }

int uix_sigfillset(uix_sigset_t *set)
    { if (!set) return -1; *set = ~(uix_sigset_t)0; return 0; }

int uix_sigaddset(uix_sigset_t *set, int signum)
{
    if (!set || signum <= 0 || signum > UIX_NSIG)
        { uix_errno = UIX_EINVAL; return -1; }
    *set |= (uix_sigset_t)1 << signum;
    return 0;
}

int uix_sigdelset(uix_sigset_t *set, int signum)
{
    if (!set || signum <= 0 || signum > UIX_NSIG)
        { uix_errno = UIX_EINVAL; return -1; }
    *set &= ~((uix_sigset_t)1 << signum);
    return 0;
}

int uix_sigismember(const uix_sigset_t *set, int signum)
{
    if (!set || signum <= 0 || signum > UIX_NSIG)
        { uix_errno = UIX_EINVAL; return -1; }
    return (*set >> signum) & 1;
}

static uix_sigset_t current_sigmask = 0;

int uix_sigprocmask(int how, const uix_sigset_t *set,
                    uix_sigset_t *oldset)
{
    if (oldset) *oldset = current_sigmask;
    if (!set)   return 0;
    switch (how) {
    case 0: current_sigmask |=  *set; break;  /* SIG_BLOCK   */
    case 1: current_sigmask &= ~*set; break;  /* SIG_UNBLOCK */
    case 2: current_sigmask  =  *set; break;  /* SIG_SETMASK */
    default: uix_errno = UIX_EINVAL; return -1;
    }
    return 0;
}

int uix_sigpending(uix_sigset_t *set)
{
    if (!set) { uix_errno = UIX_EFAULT; return -1; }
    *set = 0;
    return 0;
}

int uix_sigsuspend(const uix_sigset_t *mask)
{
    (void)mask;
    uix_errno = UIX_EINTR;
    return -1;
}
