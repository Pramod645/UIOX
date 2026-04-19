#ifndef EXIT_WAIT_H
#define EXIT_WAIT_H

#include <stdint.h>

/* ── Exit status encoding ───────────────────────────────────── */
#define EXIT_NORMAL(code)   ((code) << 8)
#define EXIT_SIGNAL(sig)    ((sig) & 0xFF)
#define EXIT_CORE           0x80

/* ── Accounting record written on exit ──────────────────────── */
typedef struct acct_record {
    uint32_t ar_pid;        /* process ID                       */
    uint32_t ar_ppid;       /* parent process ID                */
    uint16_t ar_uid;        /* user ID                          */
    uint16_t ar_gid;        /* group ID                         */
    uint32_t ar_utime;      /* user CPU time (ticks)            */
    uint32_t ar_stime;      /* kernel CPU time (ticks)          */
    uint32_t ar_elapsed;    /* elapsed real time (ticks)        */
    int      ar_exit_code;  /* exit status                      */
    char     ar_comm[16];   /* command name                     */
} acct_record_t;

/* ── Wait status returned to parent ─────────────────────────── */
typedef struct wait_status {
    uint32_t ws_child_pid;  /* child that exited                */
    int      ws_exit_code;  /* child's exit code                */
    uint32_t ws_utime;      /* child's user CPU time            */
    uint32_t ws_stime;      /* child's kernel CPU time          */
} wait_status_t;

/* ── Function prototypes ────────────────────────────────────── */
struct proc;
struct u_area;

void kernel_exit(int status);
int  kernel_wait(int *status_ptr);
void write_acct_record(struct proc *p, struct u_area *u,
                       int exit_code);
void reparent_children(struct proc *exiting_proc);
void close_all_files(struct u_area *u);
void release_proc_regions(struct proc *p, struct u_area *u);

#endif /* EXIT_WAIT_H */
