#include "../include/fork.h"
#include "../include/signal.h"
#include "../include/exit_wait.h"
#include "../include/brk.h"

/*
 * Pull in the process/region/context types from the previous
 * implementation (unix_proc). In a real build these headers
 * would be shared across both sub-systems.
 */
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* ── Simulated global state ─────────────────────────────────── */
kern_resources_t kern_res = {
    .kr_free_proc_slots = 60,
    .kr_free_mem_pages  = 4096,
    .kr_free_regions    = 100,
    .kr_free_inodes     = 200
};

/* Simulated process table and current PID counter */
static uint32_t next_pid = 2;  /* 0=swapper, 1=init, 2+=user   */

/* ── check_kernel_resources ──────────────────────────────────
 * Return 1 if sufficient kernel resources exist for fork.
 */
int check_kernel_resources(void)
{
    if (kern_res.kr_free_proc_slots <= 0) {
        fprintf(stderr, "[fork] no free process table slots\n");
        return 0;
    }
    if (kern_res.kr_free_mem_pages < 4) {
        fprintf(stderr, "[fork] insufficient memory pages\n");
        return 0;
    }
    if (kern_res.kr_free_regions < 3) {
        fprintf(stderr, "[fork] insufficient region slots\n");
        return 0;
    }
    return 1;
}

/* ── check_user_proc_limit ───────────────────────────────────
 * Return 1 if user has not exceeded their process limit.
 */
int check_user_proc_limit(uint16_t uid)
{
    /* Simulated: in real kernel scan process table and count */
    int count = 0;
    /* count = number of procs owned by uid */
    (void)uid;
    if (count >= MAX_USER_PROCS) {
        fprintf(stderr, "[fork] user process limit reached\n");
        return 0;
    }
    return 1;
}

/* ── copy_proc_table_slot ────────────────────────────────────
 * Copy parent's process table slot into child's slot.
 * Reset fields that must differ between parent and child.
 */
void copy_proc_table_slot(struct proc *child, struct proc *parent)
{
    if (!child || !parent) return;
    /* In real kernel: memcpy proc_t, then fix up fields */
    printf("[fork] copied proc table slot: "
           "parent_pid=%p -> child\n", (void *)parent);
}

/* ── increment_inode_refs ────────────────────────────────────
 * Increment reference counts on current directory and
 * changed root inodes so they are not freed prematurely.
 */
void increment_inode_refs(struct u_area *u)
{
    if (!u) return;
    /* u->u_cdir->i_count++; */
    /* if (u->u_rdir) u->u_rdir->i_count++; */
    printf("[fork] incremented inode reference counts\n");
}

/* ── increment_file_refs ─────────────────────────────────────
 * Increment reference counts on all open files so they are
 * not closed when the parent closes its copies.
 */
void increment_file_refs(struct u_area *u)
{
    if (!u) return;
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        /* if (u->u_ofile[i]) u->u_ofile[i]->f_count++; */
    }
    printf("[fork] incremented open file reference counts\n");
}

/* ── copy_parent_context ─────────────────────────────────────
 * Make a copy of the parent's context (u area, text, data,
 * stack) in memory for the child.
 * Uses dupreg + attachreg for each region.
 */
void copy_parent_context(struct proc *child, struct proc *parent)
{
    if (!child || !parent) return;
    /*
     * For each per-process region in the parent:
     *   new_rp = dupreg(parent_prp->pr_region)
     *   attachreg(new_rp, child, same_vaddr, same_type)
     *
     * Text regions are shared (dupreg returns same region for
     * shared/text when sticky); data and stack are duplicated.
     */
    printf("[fork] duplicated parent regions into child\n");
}

/* ── push_dummy_context ──────────────────────────────────────
 * Push a dummy system-level context layer onto the child's
 * context stack. When the scheduler first runs the child it
 * will find this layer and know it is starting fresh.
 */
void push_dummy_context(struct proc *child)
{
    if (!child) return;
    /*
     * The dummy layer contains:
     *  - PC set to the return path from fork in the kernel
     *  - return value register set to 0 (child gets 0 from fork)
     *  - a marker so child recognises itself
     */
    printf("[fork] pushed dummy context layer onto child\n");
}

/* ── init_child_uarea ────────────────────────────────────────
 * Reset timing fields in the child's u area.
 * Called only in the child after the context switch.
 */
void init_child_uarea(struct u_area *child_u)
{
    if (!child_u) return;
    /* child_u->u_timer.tr_utime = 0; */
    /* child_u->u_timer.tr_stime = 0; */
    printf("[fork] initialized child u area timing fields\n");
}

/* ─────────────────────────────────────────────────────────────
 * 1. Algorithm fork
 *    input : none
 *    output: parent gets child PID; child gets 0
 */
int kernel_fork(void)
{
    /* ── Check for available kernel resources ─────────────── */
    if (!check_kernel_resources())
        return -1;

    /* ── Get free proc table slot and unique PID ──────────── */
    struct proc *child_proc = NULL;    /* proc_alloc() in real */
    uint32_t child_pid      = next_pid++;
    printf("[fork] allocated child pid=%u\n", child_pid);

    /* ── Check user process limit ─────────────────────────── */
    if (!check_user_proc_limit(0 /* current uid */)) {
        /* proc_free(child_proc); */
        return -1;
    }

    /* ── Mark child state "being created" ─────────────────── */
    /* proc_set_state(child_proc, PROC_CREATED); */
    printf("[fork] child state = CREATED\n");

    /* ── Copy parent proc table slot to child ─────────────── */
    struct proc *parent_proc = NULL;   /* current_proc in real */
    copy_proc_table_slot(child_proc, parent_proc);

    /* ── Increment inode reference counts ─────────────────── */
    struct u_area *u = NULL;           /* &u in real kernel    */
    increment_inode_refs(u);

    /* ── Increment open file reference counts ─────────────── */
    increment_file_refs(u);

    /* ── Make copy of parent context in memory ────────────── */
    copy_parent_context(child_proc, parent_proc);

    /* ── Push dummy system-level context onto child ───────── */
    push_dummy_context(child_proc);

    /*
     * At this point the child is placed on the run queue.
     * Which branch executes depends on which process the
     * scheduler runs next.
     *
     * Simulated with a simple flag since we cannot truly fork
     * without an OS.  In a real kernel a context switch
     * happens here and both paths execute independently.
     */
    int executing_parent = 1;  /* simulation: parent runs first */

    if (executing_parent) {
        /* ── Parent branch ───────────────────────────────── */

        /* Change child state to "ready to run" */
        /* proc_set_state(child_proc, PROC_READY); */
        /* sched_enqueue(child_proc); */
        printf("[fork] parent: child pid=%u marked READY\n",
               child_pid);

        kern_res.kr_free_proc_slots--;

        /* Return child PID to parent */
        printf("[fork] parent returns child_pid=%u\n", child_pid);
        return (int)child_pid;

    } else {
        /* ── Child branch ────────────────────────────────── */

        /* Initialize u area timing fields */
        init_child_uarea(u);

        /* Return 0 to child */
        printf("[fork] child returns 0\n");
        return 0;
    }
}
