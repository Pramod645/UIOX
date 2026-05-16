#include "uix_sched.h"
#include "uix_errno.h"


#include "../uix_sys.h"



int uix_sched_setscheduler(uix_pid_t pid, int policy,
                            const uix_sched_param_t *param)
{
    //extern int sys_sched_setscheduler(uix_pid_t,int,
     //                                 const uix_sched_param_t*)
    //    __attribute__((weak));
    if (SYS_SCHED_SETSCHEDULER)
        return sys_sched_setscheduler(pid, policy, param);
    (void)pid; (void)policy; (void)param;
    return 0;

}

int uix_sched_getscheduler(uix_pid_t pid)
{
    //extern int sys_sched_getscheduler(uix_pid_t) __attribute__((weak));
    if (SYS_SCHED_GETSCHEDULER) return sys_sched_getscheduler(pid);
    (void)pid; return UIX_SCHED_OTHER;

}

int uix_sched_setparam(uix_pid_t pid, const uix_sched_param_t *p)
    { (void)pid; (void)p; return 0; }
int uix_sched_getparam(uix_pid_t pid, uix_sched_param_t *p)
    { (void)pid; if (p) p->sched_priority=0; return 0; }

int uix_sched_get_priority_max(int policy)
{
    switch (policy) {
    case UIX_SCHED_FIFO: case UIX_SCHED_RR: return 99;
    default: return 0;
    }
}

int uix_sched_get_priority_min(int policy)
{
    switch (policy) {
    case UIX_SCHED_FIFO: case UIX_SCHED_RR: return 1;
    default: return 0;
    }
}

int uix_sched_rr_get_interval(uix_pid_t pid, uix_timespec_t *tp)
{
    (void)pid;
    if (tp) { tp->tv_sec = 0; tp->tv_nsec = 10000000; /* 10ms */ }
    return 0;
}

int uix_sched_yield(void)
{
    //extern int sys_sched_yield(void) __attribute__((weak));
    return sys_sched_yield ? sys_sched_yield() : 0;

}

/* ***This is End of file, there is no more line should be added after this line*** */
