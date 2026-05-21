#include "../../40_SystemCallInterface/uix_sys.h"

static void busy_loop(int n)
{
    volatile int i;
    for (i = 0; i < n; i++) {}
}

int main(void)
{
    int i;
    for (i = 0; i < 3; i++) {
        uix_pid_t pid = sys_fork();
        if (pid == 0) {
            uix_sched_param_t p;
            p.sched_priority = i * 5;
            int policy = (i == 0) ? 0   /* SCHED_OTHER */
                       : (i == 1) ? 2   /* SCHED_RR    */
                       :            1;  /* SCHED_FIFO  */
            sys_sched_setscheduler(sys_getpid(), policy, &p);
            busy_loop(100000);
            sys_sched_yield();
            sys_exit(0);
        }
    }
    for (i = 0; i < 3; i++) {
        int st = 0;
        sys_wait4(-1, &st, 0, (void*)0);
    }
    return 0;
}
