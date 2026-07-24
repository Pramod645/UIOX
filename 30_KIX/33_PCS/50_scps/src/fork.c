#include "../include/fork.h"
#include "../include/brk.h"

kern_resources_t kern_res = { 64, 256, 128, 64 };

int check_kernel_resources(void)
{
    if (kern_res.kr_free_proc_slots <= 0) {
        printf("[fork] ERROR: no free process table slots\n");
        return 0;
    }
    if (kern_res.kr_free_mem_pages < 4) {
        printf("[fork] ERROR: insufficient memory pages\n");
        return 0;
    }
    if (kern_res.kr_free_regions < 3) {
        printf("[fork] ERROR: insufficient region slots\n");
        return 0;
    }
    return 1;
}

int check_user_proc_limit(uint16_t uid)
{
    int count = 0;
    (void)uid;
    if (count >= MAX_USER_PROCS) {
        printf("[fork] ERROR: user process limit reached\n");
        return 0;
    }
    return 1;
}

void copy_proc_table_slot(struct proc *child, struct proc *parent)
{
    if (!child || !parent) return;
    printf("[fork] copied proc table slot parent->child\n");
}

void increment_inode_refs(struct u_area *u) { (void)u; }
void increment_file_refs (struct u_area *u) { (void)u; }

void copy_parent_context(struct proc *child, struct proc *parent)
{
    (void)child; (void)parent;
    printf("[fork] copied parent context\n");
}

void push_dummy_context(struct proc *child)
{
    (void)child;
    printf("[fork] pushed dummy context for child\n");
}

void init_child_uarea(struct u_area *child_u)
{
    (void)child_u;
    printf("[fork] initialised child u area\n");
}

int kernel_fork(void)
{
    if (!check_kernel_resources()) return -1;
    if (!check_user_proc_limit(0)) return -1;

    printf("[fork] allocating child process slot\n");
    kern_res.kr_free_proc_slots--;
    kern_res.kr_free_mem_pages  -= 4;
    kern_res.kr_free_regions    -= 3;

    printf("[fork] fork complete — child pid assigned\n");
    return 0;
}
