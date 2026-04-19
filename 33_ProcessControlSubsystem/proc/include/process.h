#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>
#include <setjmp.h>
#include <time.h>

/* ── Constants ──────────────────────────────────────────────── */
#define NPROC           64      /* max processes in system      */
#define NOFILE          20      /* max open files per process   */
#define NSIG            32      /* number of signals            */
#define KERNEL_STACK_SZ 4096    /* kernel stack size in bytes   */
#define SLEEP_HASH_SZ   64      /* sleep hash queue buckets     */
#define MAX_PROC_SIZE   (256 * 1024 * 1024)  /* 256 MB limit   */

/* ── Process States ─────────────────────────────────────────── */
typedef enum proc_state {
    PROC_UNUSED         = 0,  /* slot is free                  */
    PROC_USER_RUNNING   = 1,  /* executing in user mode        */
    PROC_KERNEL_RUNNING = 2,  /* executing in kernel mode      */
    PROC_READY          = 3,  /* ready to run, in memory       */
    PROC_SLEEP_MEM      = 4,  /* sleeping, in memory           */
    PROC_READY_SWAPPED  = 5,  /* ready to run, swapped out     */
    PROC_SLEEP_SWAPPED  = 6,  /* sleeping, swapped out         */
    PROC_PREEMPTED      = 7,  /* preempted, returning to user  */
    PROC_CREATED        = 8,  /* newly created (fork)          */
    PROC_ZOMBIE         = 9   /* exited, waiting for parent    */
} proc_state_t;

/* ── Signal Numbers ─────────────────────────────────────────── */
typedef enum signal_num {
    SIG_NONE    =  0,
    SIGHUP      =  1,   /* hangup                              */
    SIGINT      =  2,   /* interrupt                           */
    SIGQUIT     =  3,   /* quit                                */
    SIGILL      =  4,   /* illegal instruction                 */
    SIGTRAP     =  5,   /* trace/breakpoint trap               */
    SIGABRT     =  6,   /* abort                               */
    SIGKILL     =  9,   /* kill (unblockable)                  */
    SIGSEGV     = 11,   /* segmentation fault                  */
    SIGPIPE     = 13,   /* broken pipe                         */
    SIGALRM     = 14,   /* alarm clock                         */
    SIGTERM     = 15,   /* termination                         */
    SIGCHLD     = 17,   /* child status changed                */
    SIGSTOP     = 19,   /* stop process                        */
    SIGCONT     = 18    /* continue if stopped                 */
} signal_num_t;

/* ── Scheduling Parameters ──────────────────────────────────── */
typedef struct sched_param {
    int   p_pri;        /* scheduling priority (lower = higher)*/
    int   p_cpu;        /* CPU usage for priority calculation  */
    int   p_nice;       /* user-set priority offset            */
    int   p_time;       /* residence time for scheduling       */
} sched_param_t;

/* ── Timer Record ───────────────────────────────────────────── */
typedef struct proc_timer {
    clock_t  p_utime;   /* user mode CPU time used             */
    clock_t  p_stime;   /* kernel mode CPU time used           */
    clock_t  p_cutime;  /* sum of children's user time         */
    clock_t  p_cstime;  /* sum of children's kernel time       */
    uint32_t p_alarm;   /* alarm timer countdown (seconds)     */
} proc_timer_t;

/* ── Process Table Entry ────────────────────────────────────── */
typedef struct proc {
    proc_state_t  p_state;          /* current process state   */
    uint32_t      p_pid;            /* process ID              */
    uint32_t      p_ppid;           /* parent process ID       */
    uint16_t      p_uid;            /* real user ID            */
    uint16_t      p_euid;           /* effective user ID       */
    uint16_t      p_gid;            /* real group ID           */
    uint16_t      p_egid;           /* effective group ID      */
    uint32_t      p_size;           /* process size in bytes   */
    void         *p_addr;           /* address of u area / swapped loc */
    uintptr_t     p_wchan;          /* sleep event address     */
    sched_param_t p_sched;          /* scheduling parameters   */
    uint32_t      p_sig;            /* pending signal bitmask  */
    uint32_t      p_sigmask;        /* blocked signal bitmask  */
    proc_timer_t  p_timers;         /* process timers          */
    int           p_flag;           /* process flags           */
    int           p_exit_code;      /* exit status for zombie  */
    struct proc  *p_next;           /* next in run/sleep queue */
    struct proc  *p_prev;           /* prev in run/sleep queue */
} proc_t;

/* ── Process Flags ──────────────────────────────────────────── */
#define P_LOADED    0x0001  /* process is in memory            */
#define P_STICKY    0x0002  /* sticky bit on text region       */
#define P_INTERR    0x0004  /* sleep is interruptible          */
#define P_SIGCATCH  0x0008  /* process catches signals         */
#define P_SWAPPED   0x0010  /* process has been swapped out    */
#define P_TRACED    0x0020  /* process is being traced         */
#define P_WAITED    0x0040  /* process waited on by parent     */

/* ── Sleep Priority Levels ──────────────────────────────────── */
#define PSWP        0       /* swapper priority (highest)      */
#define PINOD       10      /* inode I/O priority              */
#define PRIBIO      20      /* block I/O priority              */
#define PZERO       25      /* no signals below this priority  */
#define PWAIT       30      /* wait system call priority       */
#define PSLEP       40      /* general sleep priority          */
#define PUSER       50      /* user process base priority      */

/* ── Global Process Table ───────────────────────────────────── */
extern proc_t proc_table[NPROC];
extern proc_t *current_proc;       /* currently running process */
extern proc_t *run_queue_head;     /* head of run queue         */

/* ── Process Table Operations ───────────────────────────────── */
proc_t *proc_alloc(void);
void    proc_free(proc_t *p);
proc_t *proc_find(uint32_t pid);
void    proc_set_state(proc_t *p, proc_state_t state);
int     proc_signal_pending(proc_t *p);

#endif /* PROCESS_H */
