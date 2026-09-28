/*
 * 33_PCS/50_scps/src/uiox_kix_scps_fork.c
 *
 * fork() — Group 1, Memory Management.
 *
 * ── SCPS Algorithm 1, verbatim ────────────────────────────────────────
 *   input: none
 *   output: to parent process, child PID number
 *           to child process, 0
 *   {
 *       check for available kernel resources;
 *       get free proc table slot, unique PID number;
 *       check that user not running too many processes;
 *       mark child state "being created;"
 *       copy data from parent proc table slot to new child slot;
 *       increment counts on current directory inode and changed root;
 *       increment open file counts in file table;
 *       make copy of parent context (u area, text, data, stack);
 *       push dummy system level context layer onto child;
 *       if (executing process is parent process)
 *       { change child state to "ready to run"; return (child ID); }
 *       else
 *       { initialize u area timing fields; return (0); }
 *   }
 *
 * ── SCPS grouping for fork ────────────────────────────────────────────
 *   dupreg     region.c — PSA Algorithm 9   (present in region.c)
 *   attachreg  region.c — PSA Algorithm 4   (present in region.c)
 *
 * ── why this cannot be repaired inside a wrapper ──────────────────────
 * kernel_fork() runs the resource checks and returns 0 UNCONDITIONALLY.
 * No PID is allocated, no region is duplicated.  In init.c that gives
 *
 *     pid = kernel_fork();
 *     if (pid == 0) kernel_init_process();   <- taken in the PARENT too
 *
 * A wrapper returning 0 tells every caller "you are the child".  A
 * wrapper inventing a PID is worse.  ENOSYS is the honest answer.
 *
 * ── the five wiring steps, all with existing ingredients ──────────────
 *   1. proc_alloc(next_free_pid(), parent->p_pid)
 *   2. copy the parent's proc table slot into the child's
 *   3. for each pregion: attachreg(dupreg(parent_region), child, ...)
 *   4. push a dummy system-level context onto the child
 *   5. return child->p_pid to the parent, 0 to the child
 *
 * Note p_pgid does not exist, so step 1 cannot inherit a process group
 * either — see getpgrp.
 *
 * @version 2.0.0  @date 2026-09-28
 */
#include "../include/uiox_kix_scps.h"

int64_t uiox_kix_scps_fork(void)
{
    if (!uiox_kix_scps_current()) return SCPS_ESRCH;
    return SCPS_ENOSYS;   /* kernel_fork returns 0 for parent AND child */
}
