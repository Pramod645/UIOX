#ifndef __UIX_UIOS__H
#define __UIX_UIOS__H


#include "../../40_SystemCallInterface/uix_sys.h"
#include "../../33_ProcessControlSubsystem/01_schedular/include/sched.h" // its shuld be PoStd/uix_sched.h/.c

#define UIOS_MAX_APPS 10

typedef struct {
    const char *ua_name;
    const char *ua_path;
    int         ua_policy;
    int         ua_priority;
    uix_pid_t   ua_pid;
    int         ua_exit_code;
} uios_app_t;

void uios_init        (void);
void uios_launch_apps (void);
void uios_run         (void);
void uios_shutdown    (void);

#endif