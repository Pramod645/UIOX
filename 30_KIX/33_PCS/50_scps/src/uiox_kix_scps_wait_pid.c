/*
 * 33_PCS/50_scps/src/uiox_kix_scps_wait_pid.c
 *
 * waitpid() — Group 2, Synchronization.
 *
 * ── SCPS Algorithm 5, verbatim ────────────────────────────────────────
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
 * ── the defect, and it is a one-line one ──────────────────────────────
 * kernel_wait() has:
 *
 *     int has_children = 0;                    <- never set
 *     if (!has_children) { ...return -1; }     <- always taken
 *     if (!has_children) { ...return -1; }     <- unreachable
 *
 * It always reports "no child processes".  Combined with kernel_exit()
 * never becoming a zombie, wait() has no path to succeed.  Fixing the
 * zombie transition AND walking the proc table for p_ppid == self is
 * the whole job — no new mechanism needed.
 *
 * ── the user status pointer ───────────────────────────────────────────
 * Status must be written to a user address.  Since kernel_wait() takes
 * a KERNEL int*, passing a user pointer through would make the kernel
 * write wherever the caller said.  So a non-NULL status_out is verified
 * through the shared gate and then refused until a real copy_to_user
 * exists.  A NULL status_out is legal per POSIX and is accepted.
 *
 * @version 2.0.0  @date 2026-09-28
 */
#include "../include/uiox_kix_scps.h"

extern int kernel_wait(int *status_ptr);   /* 40_procStruct/src/exit_wait.c */

int64_t uiox_kix_scps_wait_pid(uiox_uint64_t pid, uiox_uintptr_t status_out,
                               uiox_uint64_t options)
{
    int status = 0;
    int id;

    if (!uiox_kix_scps_current()) return SCPS_ESRCH;
    if (pid > 0x7FFFFFFFu) return SCPS_EINVAL;
    (void)options;                 /* WNOHANG / WUNTRACED not modelled */

    if (status_out != 0u) {
        if (uiox_kix_scps_check_user_ptr(status_out, sizeof(int), 4u) != 0)
            return SCPS_EFAULT;
        return SCPS_EFAULT;        /* no copy_to_user to finish it with */
    }

    id = kernel_wait(&status);
    if (id < 0) return SCPS_ECHILD;

    return (int64_t)id;
}
