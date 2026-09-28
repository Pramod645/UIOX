/*
 * 33_PCS/50_scpcs/src/uiox_kix_scpcs_wait_pid.c
 *
 * waitpid() — Group 2, Synchronization.
 *
 * ── the algorithm, verbatim ─────────────────────────────────────────
 *   input: address of variable to store status of exiting process
 *   output: child ID, child exit code
 *   {
 *       if (waiting process has no child processes)
 *           return (error);
 *       for (;;)                          // loop until return from inside
 *       {
 *           if (waiting process has zombie child)
 *           {
 *               pick arbitrary zombie child;
 *               add child CPU usage to parent;
 *               free child process table entry;
 *               return (child ID, child exit code);
 *           }
 *           if (process has no children)
 *               return error;
 *           sleep at interruptible priority (event child process exits);
 *       }
 *   }
 *
 * ── the defect, and it is one line ──────────────────────────────────
 * The callee declares a flag meaning "has_children", never sets it, and
 * then tests it twice — so the first test always takes the error return
 * and the second is unreachable.  It therefore reports "no child
 * processes" unconditionally.
 *
 * Paired with exit never becoming a zombie, wait has no path to succeed.
 * Both halves are needed and both are small: set the flag by scanning the
 * table for p_ppid == self, and reap the first ZOMBIE found.
 *
 * The sleep at the end uses the interruptible path, so a signal during
 * the wait returns rather than hanging — and that path is implemented.
 *
 * ── the status pointer ──────────────────────────────────────────────
 * The callee takes a KERNEL int pointer.  Passing a user address through
 * would make the kernel write wherever the caller chose, so a non-NULL
 * status_out is validated through the shared gate and then refused for
 * want of a copy_to_user.  NULL is legal per POSIX and accepted.
 *
 * Anything other than a specific PID is unrepresentable: there is no
 * p_pgrp, so "any child in the group" cannot be named.
 *
 * @version 1.0.0  @date 2026-09-29
 */
#include "../include/uiox_kix_scpcs.h"

extern int kernel_wait(int *status_ptr);

int64_t uiox_kix_scpcs_wait_pid(uiox_uint64_t pid, uiox_uintptr_t status_out,
                                uiox_uint64_t options)
{
    int status = 0;
    int id;

    (void)options;   /* WNOHANG and WUNTRACED are not modelled */

    if (!uiox_kix_scps_current()) return SCPCS_ESRCH;
    if (pid > 0x7FFFFFFFu) return SCPCS_EINVAL;

    if (status_out != 0u) {
        if (uiox_kix_scps_check_user_ptr(status_out, sizeof(int), 4u) != 0)
            return SCPS_EFAULT;
        return SCPCS_EFAULT;   /* no copy_to_user to complete it */
    }

    id = kernel_wait(&status);
    if (id < 0) return SCPCS_ECHILD;

    return (int64_t)id;
}
