#include "uix_wait.h"
#include "uix_errno.h"

uix_pid_t uix_wait(int *wstatus)
{
    extern long sys_wait4(uix_pid_t, int *, int, void *)
        __attribute__((weak));
    if (sys_wait4) return (uix_pid_t)sys_wait4(-1, wstatus, 0, NULL);
    uix_errno = UIX_ECHILD;
    return -1;
}

uix_pid_t uix_waitpid(uix_pid_t pid, int *wstatus, int options)
{
    extern long sys_wait4(uix_pid_t, int *, int, void *)
        __attribute__((weak));
    if (sys_wait4) return (uix_pid_t)sys_wait4(pid, wstatus,
                                               options, NULL);
    uix_errno = UIX_ECHILD;
    return -1;
}

uix_pid_t uix_wait3(int *wstatus, int options, void *rusage)
{
    extern long sys_wait4(uix_pid_t, int *, int, void *)
        __attribute__((weak));
    if (sys_wait4) return (uix_pid_t)sys_wait4(-1, wstatus,
                                               options, rusage);
    uix_errno = UIX_ECHILD;
    return -1;
}

uix_pid_t uix_wait4(uix_pid_t pid, int *wstatus,
                    int options, void *rusage)
{
    extern long sys_wait4(uix_pid_t, int *, int, void *)
        __attribute__((weak));
    if (sys_wait4) return (uix_pid_t)sys_wait4(pid, wstatus,
                                               options, rusage);
    uix_errno = UIX_ECHILD;
    return -1;
}

/* ***This is End of file, there is no more line should be added after this line*** */
