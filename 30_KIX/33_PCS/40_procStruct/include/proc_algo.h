/*
 * 30_KIX/33_PCS/40_procStruct/include/proc_algo.h
 *
 * Freestanding fixes (v2.0)
 *
 * @version 2.0.0  @date 2026-07-23
 */
#ifndef PROC_ALGO_H
#define PROC_ALGO_H
#include "process.h"
#include "region.h"
#include "context.h"
#include "uiox_klibc.h"
#ifndef EINVAL
#define EINVAL   22
#endif
#ifndef EINTR
#define EINTR     4
#endif
#ifndef ENOMEM
#define ENOMEM   12
#endif
#ifndef EFAULT
#define EFAULT   14
#endif
#ifndef ENOSYS
#define ENOSYS   38
#endif
#define SLEEP_HASH_SZ   64
#define UIOX_JMP_BUF_SLOTS  8u
typedef struct { uint64_t regs[UIOX_JMP_BUF_SLOTS]; } uiox_jmp_buf_t;
#define NSYSCALL  256
struct u_area;
typedef int (*syscall_fn_t)(struct u_area *u, uintptr_t *args);
typedef struct syscall_entry {
    syscall_fn_t se_fn; int se_nargs; const char *se_name;
} syscall_entry_t;
#undef NOFILE
#define NOFILE  20
#undef NSIG
#define NSIG    32
typedef struct file  file_t;
typedef struct inode inode_t;
typedef struct io_params {
    char *io_base; uint32_t io_count; uint32_t io_offset; int io_seg;
} io_params_t;
typedef struct u_area {
    proc_t        *u_proc;
    file_t        *u_ofile[NOFILE];
    int            u_signal[NSIG];
    int            u_error;
    int64_t        u_rval;
    int            u_uid;  int u_gid;
    int            u_euid; int u_egid;
    io_params_t    u_io;
    pregion_t      u_pregs[MAX_REG_PER_PROC];
    uiox_jmp_buf_t u_qsave;
    reg_context_t  u_saved_regs;
    sys_context_t  u_sysctx;
} u_area_t;
typedef struct sys_mem_map {
    uintptr_t smm_page_table_addr; uintptr_t smm_virt_addr; uint32_t smm_npages;
} sys_mem_map_t;
extern u_area_t         u;
extern syscall_entry_t  syscall_table[NSYSCALL];
typedef struct sleep_queue { proc_t *sq_head; proc_t *sq_tail; } sleep_queue_t;
extern sleep_queue_t sleep_hash[SLEEP_HASH_SZ];
extern int           scheduler_flag;
extern int           proc_level;
void inthand         (int vec, reg_context_t *regs);
int  syscall         (int callnum, uintptr_t *args);
void syscall_register(int num, syscall_fn_t fn, int nargs, const char *name);
int  proc_sleep      (uintptr_t wchan, int priority, int interruptible);
void proc_wakeup     (uintptr_t wchan);
void swtch           (void);
#endif /* PROC_ALGO_H */
