#include "../../40_SystemCallInterface/uix_sys.h"

static volatile int g_caught = 0;

static void handler(int sig) { (void)sig; g_caught = 1; }

int main(void)
{
    /* register handler for SIGUSR1 (16) */
    sys_signal(16, handler);

    /* send signal to self */
    sys_kill(sys_getpid(), 16);

    if (!g_caught) sys_exit(1);

    /* SIGCHLD via fork/exit */
    uix_pid_t pid = sys_fork();
    if (pid == 0) {
        sys_exit(42);
    } else if (pid > 0) {
        int st = 0;
        sys_wait4(pid, &st, 0, (void*)0);
    }
    return 0;
}
