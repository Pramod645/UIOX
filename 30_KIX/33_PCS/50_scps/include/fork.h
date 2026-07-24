#ifndef FORK_H
#define FORK_H
#include "uiox_klibc.h"

#define MAX_USER_PROCS  64
#define INIT_PID        1
#define SWAPPER_PID     0
#define MAX_OPEN_FILES  20
#define MAX_PROC_SIZE   (256 * 1024 * 1024)

struct proc; struct u_area; struct region;
struct pregion; struct file; struct inode;

typedef struct fork_result {
    int      fr_is_parent;
    uint32_t fr_child_pid;
} fork_result_t;

typedef struct kern_resources {
    int kr_free_proc_slots;
    int kr_free_mem_pages;
    int kr_free_regions;
    int kr_free_inodes;
} kern_resources_t;
extern kern_resources_t kern_res;

typedef struct proc_context_copy {
    uint8_t  pcc_uarea[4096];
    uint8_t  pcc_kstack[4096];
    uint32_t pcc_reg_context[16];
} proc_context_copy_t;

int  kernel_fork          (void);
int  check_kernel_resources(void);
int  check_user_proc_limit(uint16_t uid);
void copy_proc_table_slot (struct proc *child, struct proc *parent);
void increment_inode_refs (struct u_area *u);
void increment_file_refs  (struct u_area *u);
void copy_parent_context  (struct proc *child, struct proc *parent);
void push_dummy_context   (struct proc *child);
void init_child_uarea     (struct u_area *child_u);
#endif
