/*
 * 33_PCS/50_scpcs/src/uiox_kix_scpcs_signal.c
 *
 * Group 2 — System calls Dealing with Synchronization.
 *
 *   waitpid()      PSA Algorithm 5
 *   kill()         sends a signal by PID
 *   raise()        sends a signal to self
 *   sigaction()    installs a disposition
 *   sigprocmask()  changes the blocked set
 *   sigpending()   reports signals pending AND blocked
 *
 * These sit together because the kernel synchronizes exit and wait
 * THROUGH signals — which is why the missing delivery step in kill is
 * not a local defect.  It is one of the reasons wait cannot work.
 *
 * @version 1.0.0  @date 2026-09-29
 */
#include "../include/uiox_kix_scpcs.h"

extern int                  kernel_wait(int *status_ptr);
extern int                  kernel_kill(uiox_uint32_t pid, int signum);
extern void                 send_signal(uiox_kix_psa_proc_t *p, int signum);
extern uiox_kix_psa_proc_t *uiox_kix_psa_proc_find(uiox_uint32_t pid);
extern uiox_kix_psa_proc_t *uiox_kix_psa_current_proc;

/* The three mask-operation codes.  Restated so this file needs no
 * second include path; they MUST match whatever the signal header
 * declares. */
#ifndef SIG_BLOCK
#define SIG_BLOCK   0
#endif
#ifndef SIG_UNBLOCK
#define SIG_UNBLOCK 1
#endif
#ifndef SIG_SETMASK
#define SIG_SETMASK 2
#endif

/* ────────────────────────────────────────────────────────────────────
 * waitpid() — PSA Algorithm 5
 *
 *   input: address of variable to store status of exiting process
 *   output: child ID, child exit code
 *   {
 *       if (waiting process has no child processes)
 *           return (error);
 *       for (;;)
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
 * Returns the reaped child's PID, or ECHILD when there is nothing to
 * reap.
 *
 * The callee declares a "has_children" flag, never sets it, then tests it
 * twice — so the first test always takes the error return and the second
 * is unreachable.  It therefore reports "no child processes"
 * unconditionally.  Paired with exit never becoming a zombie, wait has no
 * path to succeed; both halves are small and both are needed.
 *
 * A non-NULL status pointer is validated through the gate and then
 * REFUSED, because the kernel has no copy_to_user and writing through a
 * raw user address from kernel mode is the bug that layer exists to
 * prevent.  NULL is legal per POSIX.  Anything other than a specific PID
 * is unrepresentable: there is no group field, so "any child in the
 * group" cannot be named.
 * ──────────────────────────────────────────────────────────────────── */
int64_t uiox_kix_scpcs_wait_pid(uiox_uint64_t pid, uiox_uintptr_t status_out,
                                uiox_uint64_t options)
{
    int status = 0;
    int id;

    (void)options;   /* WNOHANG and WUNTRACED are not modelled */

    if (!uiox_kix_scpcs_current()) return SCPCS_ESRCH;
    if (pid > 0x7FFFFFFFu)        return SCPCS_EINVAL;

    if (status_out != 0u) {
        if (uiox_kix_scpcs_check_user_ptr(status_out, sizeof(int), 4u) != 0)
            return SCPCS_EFAULT;
        return SCPCS_EFAULT;   /* no copy_to_user to complete it */
    }

    id = kernel_wait(&status);
    if (id < 0) return SCPCS_ECHILD;

    return (int64_t)id;
}

/* ────────────────────────────────────────────────────────────────────
 * kill() — signal a process by PID
 *
 * Returns 0 on success, ESRCH when the PID names no live process.
 * The lookup is real, so ESRCH is a genuine answer rather than a
 * placeholder.
 *
 * ENOSYS for delivery: the send is commented out in the signal
 * implementation, though everything around it exists — the function that
 * sets the pending bit, the PID lookup, and the issig/psig pair that runs
 * on the way back to user mode.
 *
 * Signal 0 is POSIX's existence check, not a signal to deliver, so it is
 * refused rather than reported as success.  The group forms kill(0, ...)
 * and kill(-1, ...) cannot be expressed at all: there is no group field
 * to enumerate and no permission model to decide who may signal whom.
 * Refusing is honest; signalling only self would look like success.
 *
 * Signal numbers come from this layer's own table, which places SIGCHLD
 * at 17.  A second table elsewhere puts it at 18, and the disagreement is
 * a live bug the moment both are in one translation unit.
 * ──────────────────────────────────────────────────────────────────── */
int64_t uiox_kix_scpcs_kill(uiox_uint64_t pid, uiox_uint64_t sig)
{
    if (!uiox_kix_scpcs_current()) return SCPCS_ESRCH;

    if (sig == 0u) return SCPCS_EINVAL;
    if (sig > (uiox_uint64_t)UIOX_KIX_PSA_NSIG) return SCPCS_EINVAL;

    if (pid == 0u || pid > 0x7FFFFFFFu) return SCPCS_EINVAL;

    if (!uiox_kix_psa_proc_find((uiox_uint32_t)pid)) return SCPCS_ESRCH;

    (void)kernel_kill;   /* present, does not deliver — see banner */
    return SCPCS_ENOSYS;
}

/* ────────────────────────────────────────────────────────────────────
 * raise() — signal the calling process
 *
 * Returns 0 on success, EINVAL for an out-of-range signal.
 *
 * raise(sig) is kill(getpid(), sig), but written directly it inherits
 * neither of kill's dependencies:
 *
 *   1. no PID LOOKUP — the process is already in hand, so this does not
 *      depend on the process table being searchable
 *   2. no PID RANGE CHECK — no PID is named, so nothing to validate
 *
 * The delivery primitive is the same one kill would use, and that is
 * implemented.  So once delivery is uncommented, raise works before kill
 * does — which makes it the cheapest first test of the signal path.
 *
 * If the signal is uncaught and fatal the process terminates, and the
 * signal implementation reaches that by way of the exit algorithm — which
 * is why exit's zombie transition matters to a call that looks unrelated.
 * ──────────────────────────────────────────────────────────────────── */
int64_t uiox_kix_scpcs_raise(uiox_uint64_t sig)
{
    uiox_kix_psa_proc_t *p = uiox_kix_scpcs_current();

    if (!p) return SCPCS_ESRCH;
    if (sig == 0u || sig > (uiox_uint64_t)UIOX_KIX_PSA_NSIG)
        return SCPCS_EINVAL;

    (void)send_signal;                 /* direct-to-self delivery */
    (void)uiox_kix_psa_current_proc;
    return SCPCS_ENOSYS;
}

/* ────────────────────────────────────────────────────────────────────
 * sigaction() — install a signal disposition
 *
 * Returns 0 on success, EINVAL for an invalid signal or an attempt to
 * change SIGKILL or SIGSTOP.
 *
 * ENOSYS because the process table entry carries no handler array.  The
 * dispositions live on a PRIVATE structure inside the signal
 * implementation, reached by casting a process pointer to it — so once
 * that cast is removed there is nowhere for a handler to be stored.
 *
 * The SIGKILL/SIGSTOP refusal is enforced by the callee and is NOT
 * duplicated here: two copies of a rule are two things to keep in step.
 *
 * TO FINISH: add a disposition array to the process table entry in
 * 40psa, delete the cast so issig/psig read that entry directly, and
 * uncomment the handler assignment.  With copy_in/copy_out for act and
 * oldact this becomes complete.
 * ──────────────────────────────────────────────────────────────────── */
int64_t uiox_kix_scpcs_sig_action(uiox_uint64_t sig, uiox_uintptr_t act,
                                  uiox_uintptr_t oldact)
{
    (void)act;      /* no copy in  — see banner */
    (void)oldact;   /* no copy out */

    if (!uiox_kix_scpcs_current()) return SCPCS_ESRCH;

    if (sig == 0u || sig > (uiox_uint64_t)UIOX_KIX_PSA_NSIG)
        return SCPCS_EINVAL;

    if (sig == (uiox_uint64_t)UIOX_KIX_PSA_SIGKILL ||
        sig == (uiox_uint64_t)UIOX_KIX_PSA_SIGSTOP)
        return SCPCS_EINVAL;

    return SCPCS_ENOSYS;
}

/* ────────────────────────────────────────────────────────────────────
 * sigprocmask() — change the blocked-signal set
 *
 * Returns 0 on success, EINVAL for an unknown operation code.
 *
 * The STORAGE is real — the table entry has a blocked mask — so unlike
 * sigaction this call has somewhere to put its result.  It is still
 * ENOSYS for two reasons:
 *
 *   1. set and oldset are user addresses and there is no copy_to_user,
 *      so a caller cannot hand in a mask at all
 *   2. the signal implementation tests a CAST mask, not the real field.
 *      Until that cast is removed, setting the real field would have no
 *      effect on the code that decides whether a signal is deliverable —
 *      which is worse than not implementing it, because it would LOOK
 *      like it worked while signals continued to arrive
 *
 * A caller passing nonsense hears EINVAL, not ENOSYS: EINVAL blames the
 * argument, ENOSYS reports the kernel, and the two mean different things.
 * ──────────────────────────────────────────────────────────────────── */
int64_t uiox_kix_scpcs_sig_procmask(uiox_uint64_t how, uiox_uintptr_t set,
                                    uiox_uintptr_t oldset)
{
    (void)set;
    (void)oldset;

    if (!uiox_kix_scpcs_current()) return SCPCS_ESRCH;

    if (how != (uiox_uint64_t)SIG_BLOCK   &&
        how != (uiox_uint64_t)SIG_UNBLOCK &&
        how != (uiox_uint64_t)SIG_SETMASK)
        return SCPCS_EINVAL;

    return SCPCS_ENOSYS;
}

/* ────────────────────────────────────────────────────────────────────
 * sigpending() — signals both pending and blocked
 *
 * Returns the mask directly, because there is no copy_to_user to write it
 * to set.  A non-NULL set is refused with EFAULT rather than ignored:
 * accepting it would tell the caller a write happened.
 *
 * The INTERSECTION is the meaning.  A pending signal that is not blocked
 * is about to be delivered, so it is waiting for nothing — returning the
 * pending mask alone would overstate the set.
 *
 * Both fields are real, which makes this the one call in Group 2 that
 * answers today.
 * ──────────────────────────────────────────────────────────────────── */
int64_t uiox_kix_scpcs_sig_pending(uiox_uintptr_t set)
{
    uiox_kix_psa_proc_t *p = uiox_kix_scpcs_current();

    if (!p) return SCPCS_ESRCH;
    if (set != 0u) return SCPCS_EFAULT;   /* no copy_to_user */

    return (int64_t)(p->p_sig & p->p_sigmask);
}
