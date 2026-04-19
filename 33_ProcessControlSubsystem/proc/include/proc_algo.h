#ifndef PROC_ALGO_H
#define PROC_ALGO_H

#include "process.h"
#include "region.h"
#include "context.h"
#include <stdint.h>

/* ── U Area ─────────────────────────────────────────────────── */
#define NOFILE      20
#define NSIG        32

typedef struct file file_t;
typedef struct inode inode_t;

typedef struct io_params {
    char     *io_base;          /* user buffer address         */
    uint32_t  io_count;         /* bytes to transfer           */
    uint32_t  io_offset;        /* file offset                 */
    int       io_seg;           /* 0=user space, 1=kernel      */
} io_params_t;

typedef struct timer_record {
    clock_t  tr_utime;          /* time in user mode           */
    clock_t  tr_stime;          /* time in kernel mode         */
} timer_record_t;

typedef struct u_area {
    proc_t        *u_procp;                 /* process table ptr */
    uint16_t       u_uid;                   /* effective uid     */
    uint16_t       u_gid;                   /* effective gid     */
    timer_record_t u_timer;                 /* user/kernel times */
    void          (*u_signal[NSIG])(int);   /* signal handlers   */
    int            u_ttyp;                  /* control terminal  */
    int            u_error;                 /* last error code   */
    intptr_t       u_rval;                  /* syscall return    */
    io_params_t    u_io;                    /* I/O parameters    */
    inode_t       *u_cdir;                  /* current directory */
    inode_t       *u_rdir;                  /* root directory    */
    file_t        *u_ofile[NOFILE];         /* open file table   */
    uint32_t       u_limit_proc;            /* max process size  */
    uint32_t       u_limit_file;            /* max file size     */
    uint16_t       u_umask;                 /* file creation mask*/
    pregion_t      u_pregs[MAX_REG_PER_PROC]; /* per-proc regions*/
    jmp_buf        u_qsave;                 /* for longjmp abort */
    reg_context_t  u_saved_regs;            /* saved user regs   */
    sys_context_t  u_sysctx;               /* system context    */
} u_area_t;

/* ── System Memory Map ──────────────────────────────────────── */
typedef struct sys_mem_map {
    uintptr_t  smm_page_table_addr;   /* address of page table */
    uintptr_t  smm_virt_addr;         /* virtual address start */
    uint32_t   smm_npages;            /* pages in page table   */
} sys_mem_map_t;

/* ── System Call Table Entry ────────────────────────────────── */
typedef int (*syscall_fn_t)(u_area_t *u, uintptr_t *args);

typedef struct syscall_entry {
    syscall_fn_t  se_fn;        /* function to invoke          */
    int           se_nargs;     /* number of arguments         */
    const char   *se_name;      /* system call name            */
} syscall_entry_t;

#define NSYSCALL    256
extern syscall_entry_t syscall_table[NSYSCALL];

/* ── Global U Area (current process) ───────────────────────── */
extern u_area_t u;

/* ── Sleep Hash Queue ───────────────────────────────────────── */
typedef struct sleep_queue {
    proc_t  *sq_head;           /* first process on queue      */
    proc_t  *sq_tail;           /* last process on queue       */
} sleep_queue_t;

extern sleep_queue_t sleep_hash[SLEEP_HASH_SZ];
extern int           scheduler_flag;   /* reschedule needed     */
extern int           proc_level;       /* processor intr level  */

/* ── Algorithm Prototypes ───────────────────────────────────── */
/* Interrupt & syscall */
void inthand  (int vec, reg_context_t *regs);
int  syscall  (int callnum, uintptr_t *args);

/* Sleep / wakeup */
int  proc_sleep (uintptr_t wchan, int priority, int interruptible);
void proc_wakeup(uintptr_t wchan);

/* Scheduler helpers */
void  sched_enqueue (proc_t *p);
proc_t *sched_pick  (void);
void  context_switch_proc(proc_t *from, proc_t *to);

/* Error codes */
#define EPERM    1
#define ENOENT   2
#define ENOMEM  12
#define EACCES  13
#define EFAULT  14
#define EINVAL  22
#define ENFILE  23
#define EMFILE  24
#define ENOSPC  28

#endif /* PROC_ALGO_H */
