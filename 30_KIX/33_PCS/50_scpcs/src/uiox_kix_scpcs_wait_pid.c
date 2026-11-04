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

int64_t uiox_kix_scpcs_wait_pid(uiox_uint64_t pid, uiox_uintptr_t status_out,
                                uiox_uint64_t options)
{
    uiox_kix_psa_proc_t *child;
    uiox_kix_psa_proc_t *self;
    int                  status;
    
    (void)options;   /* WNOHANG and WUNTRACED are not modelled */

    self = uiox_kix_scpcs_current();
    if (!self) return SCPCS_ESRCH;
    if (pid > 0x7FFFFFFFu) return SCPCS_EINVAL;

    /* no copy_to_user exists, so a non-NULL status pointer is refused */
    if (status_out != 0u) return SCPCS_EFAULT;

    child = uiox_kix_psa_proc_find((uint32_t)pid);
    if (!child) return SCPCS_ECHILD;
    if (child->p_state != UIOX_KIX_PSA_PROC_ZOMBIE) return SCPCS_ECHILD;
    if (child->p_ppid != self->p_pid) return SCPCS_ECHILD;
    status = child->p_exit_code;                        /* ← add */
    uiox_kix_psa_proc_free(child);

    return (int64_t)pid;   /* NOT child->p_pid -- that memory is freed */
}
