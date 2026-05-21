#include "uix_uios.h"

static uios_app_t g_apps[UIOS_MAX_APPS] = {
    { "shell",     "/../21_apps_/app_shell",     0, 0,  -1, 0 },
    { "logger",    "/../21_apps_/app_logger",    0, 0,  -1, 0 },
    { "filesvr",   "/../21_apps_/app_fileserver",2, 5,  -1, 0 },
    { "netsvr",    "/../21_apps_/app_netserver", 2, 5,  -1, 0 },
    { "ipctest",   "/../21_apps_/app_ipctest",   0, 0,  -1, 0 },
    { "memtest",   "/../21_apps_/app_memtest",   0, 0,  -1, 0 },
    { "schedtest", "/../21_apps_/app_scheduler", 1, 10, -1, 0 },
    { "sigtest",   "/../21_apps_/app_sigtest",   0, 0,  -1, 0 },
    { "fstest",    "/../21_apps_/app_fstest",    0, 0,  -1, 0 },
    { "clocktest", "/../21_apps_/app_clocktest", 0, 0,  -1, 0 },
};

void uios_init(void)
{
    int i;
    for(i=0;i<UIOS_MAX_APPS;i++) {
        g_apps[i].ua_pid       = -1;
        g_apps[i].ua_exit_code = -1;
    }
}

void uios_launch_apps(void)
{
    int i;
    for(i=0;i<UIOS_MAX_APPS;i++) {
        uix_pid_t pid = sys_fork();
        if(pid == 0) {
            uix_sched_param_t p;
            p.sched_priority = g_apps[i].ua_priority;
            sys_sched_setscheduler(sys_getpid(), g_apps[i].ua_policy, &p);
            char *argv[] = { (char*)g_apps[i].ua_path, (char*)0 };
            sys_execve(g_apps[i].ua_path, argv, (char*const*)0);
            sys_exit(1);
        } else if(pid > 0) {
            g_apps[i].ua_pid = pid;
            sched_add_proc(pid, g_apps[i].ua_policy, g_apps[i].ua_priority);
        }
    }
}

void uios_run(void)
{
    int running = UIOS_MAX_APPS;
    int i;
    while(running > 0) {
        sched_tick();
        for(i=0;i<UIOS_MAX_APPS;i++) {
            if(g_apps[i].ua_pid <= 0) continue;
            int status = 0;
            uix_pid_t r = sys_wait4(g_apps[i].ua_pid, &status,
                                     UIX_WNOHANG, (void*)0);
            if(r == g_apps[i].ua_pid) {
                g_apps[i].ua_exit_code = status;
                g_apps[i].ua_pid       = 0;
                sched_remove_proc(r);
                running--;
            }
        }
    }
}

void uios_shutdown(void)
{
    int i;
    for(i=0;i<UIOS_MAX_APPS;i++) {
        if(g_apps[i].ua_pid > 0) {
            sys_kill(g_apps[i].ua_pid, 15);
            int st=0;
            sys_wait4(g_apps[i].ua_pid, &st, 0, (void*)0);
            g_apps[i].ua_pid=0;
        }
    }
}

int main(void)
{
    uios_init();
    uios_launch_apps();
    uios_run();
    uios_shutdown();
    return 0;
}
