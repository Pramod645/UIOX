#ifndef FORK_H
#define FORK_H

#include <stdint.h>
#include <stddef.h>

/* ── Constants ──────────────────────────────────────────────── */
#define MAX_USER_PROCS  64      /* max processes per user       */
#define INIT_PID        1       /* PID of init process          */
#define SWAPPER_PID     0       /* PID of swapper (process 0)   */

/* ── Process resource limits ────────────────────────────────── */
#define MAX_OPEN_FILES  20
#define MAX_PROC_SIZE   (256 * 1024 * 1024)

/* ── Forward declarations ───────────────────────────────────── */
struct proc;
struct u_area;
struct region;
struct pregion;
struct file;
struct inode;

/* ── Fork result ────────────────────────────────────────────── */
typedef struct fork_result {
    int      fr_is_parent;  /* 1 = parent, 0 = child           */
    uint32_t fr_child_pid;  /* child PID (valid in parent)      */
} fork_result_t;

/* ── Kernel resource check ──────────────────────────────────── */
typedef struct kern_resources {
    int kr_free_proc_slots;  /* available process table slots   */
    int kr_free_mem_pages;   /* available memory pages          */
    int kr_free_regions;     /* available region table slots    */
    int kr_free_inodes;      /* available inode table slots     */
} kern_resources_t;

extern kern_resources_t kern_res;

/* ── Process context copy (for fork) ───────────────────────── */
typedef struct proc_context_copy {
    uint8_t  pcc_uarea[4096];    /* copy of u area              */
    uint8_t  pcc_kstack[4096];   /* copy of kernel stack        */
    uint32_t pcc_reg_context[16];/* copy of saved registers     */
} proc_context_copy_t;

/* ── Function prototypes ────────────────────────────────────── */
int  kernel_fork(void);
int  check_kernel_resources(void);
int  check_user_proc_limit(uint16_t uid);
void copy_proc_table_slot(struct proc *child, struct proc *parent);
void increment_inode_refs(struct u_area *u);
void increment_file_refs(struct u_area *u);
void copy_parent_context(struct proc *child, struct proc *parent);
void push_dummy_context(struct proc *child);
void init_child_uarea(struct u_area *child_u);

#endif /* FORK_H */
