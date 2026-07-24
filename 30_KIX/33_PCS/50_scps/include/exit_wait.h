#ifndef EXIT_WAIT_H
#define EXIT_WAIT_H
#include "uiox_klibc.h"

#define EXIT_NORMAL(code)  ((code) << 8)
#define EXIT_SIGNAL(sig)   ((sig) & 0xFF)
#define EXIT_CORE          0x80

typedef struct wait_status {
    uint32_t ws_child_pid;
    int      ws_exit_code;
    uint32_t ws_utime;
    uint32_t ws_stime;
} wait_status_t;

struct proc;
int kernel_exit(int status);
int kernel_wait(int *status_ptr);

#endif
