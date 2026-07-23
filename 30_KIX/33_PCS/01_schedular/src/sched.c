#include "../include/sched.h"

static sched_proc_t g_procs[MAX_PROCS];
static int          g_current = -1;

int kernel_sched_setscheduler(uix_pid_t pid, int policy,
                                const uix_sched_param_t *p)
{
    (void)p; int i;
    for(i=0;i<MAX_PROCS;i++)
        if(g_procs[i].sp_in_use&&g_procs[i].sp_pid==pid)
            { g_procs[i].sp_policy=policy; return 0; }
    return -1;
}

int kernel_sched_getscheduler(uix_pid_t pid)
{
    int i;
    for(i=0;i<MAX_PROCS;i++)
        if(g_procs[i].sp_in_use&&g_procs[i].sp_pid==pid)
            return g_procs[i].sp_policy;
    return 0;
}

int kernel_sched_yield(void)
{
    if(g_current>=0) g_procs[g_current].sp_ticks_left=0;
    return 0;
}

int sched_add_proc(uix_pid_t pid, int policy, int priority)
{
    int i;
    for(i=0;i<MAX_PROCS;i++) {
        if(!g_procs[i].sp_in_use) {
            g_procs[i].sp_in_use=1;
            g_procs[i].sp_pid=pid;
            g_procs[i].sp_policy=policy;
            g_procs[i].sp_priority=priority;
            g_procs[i].sp_ticks_left=TIME_SLICE;
            g_procs[i].sp_state=PROC_READY;
            return 0;
        }
    }
    return -1;
}

int sched_remove_proc(uix_pid_t pid)
{
    int i;
    for(i=0;i<MAX_PROCS;i++)
        if(g_procs[i].sp_in_use&&g_procs[i].sp_pid==pid)
            { g_procs[i].sp_in_use=0; return 0; }
    return -1;
}

int sched_tick(void)
{
    if(g_current<0) return 0;
    g_procs[g_current].sp_ticks_left--;
    if(g_procs[g_current].sp_ticks_left<=0) {
        g_procs[g_current].sp_state=PROC_READY;
        g_procs[g_current].sp_ticks_left=TIME_SLICE;
        sched_next();
    }
    return 0;
}

uix_pid_t sched_next(void)
{
    int start=(g_current+1)%MAX_PROCS;
    int i;
    for(i=0;i<MAX_PROCS;i++) {
        int idx=(start+i)%MAX_PROCS;
        if(g_procs[idx].sp_in_use&&g_procs[idx].sp_state==PROC_READY) {
            g_current=idx;
            g_procs[idx].sp_state=PROC_RUNNING;
            return g_procs[idx].sp_pid;
        }
    }
    return -1;
}
