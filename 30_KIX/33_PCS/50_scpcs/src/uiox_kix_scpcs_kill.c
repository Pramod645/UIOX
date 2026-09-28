/*
 * 33_PCS/50_scpcs/src/uiox_kix_scpcs_kill.c
 *
 * kill() — Group 2, Synchronization.
 *
 * kill sits with wait and signal because the kernel synchronizes exit and
 * wait THROUGH signals.  That is why the missing delivery step here is
 * not a local defect: it is one of the reasons wait cannot work.
 *
 * ── the delivery step is commented out ──────────────────────────────
 * The callee prints a line and returns, with the actual send commented
 * out.  Everything around it exists: the function that sets the pending
 * bit, the lookup that finds the entry by PID, and the issig/psig pair
 * that runs on the way back to user mode.
 *
 * ── what the wrapper does check, and it is real ─────────────────────
 * The target must exist.  A PID that names no live process returns ESRCH
 * from the table walk, which is a genuine answer rather than a
 * placeholder.
 *
 * ── the PID forms that cannot be expressed ──────────────────────────
 * POSIX kill(0, sig) signals the caller's whole process group, and
 * kill(-1, sig) signals every process the caller may signal.  Neither can
 * be built: the process table entry carries no p_pgrp, so there is no
 * group to enumerate, and there is no permission model to decide who may
 * signal whom.  They are refused with EINVAL.  Silently signalling only
 * self would be worse than refusing, because it would look like success.
 *
 * ── signal numbering ────────────────────────────────────────────────
 * Numbers come from the layer's own table, which places SIGCHLD at 17.
 * A second table elsewhere in the tree places it at 18.  Group 2 runs
 * through the signal implementation, so that table is what this follows
 * — and the disagreement is a live bug the moment both are in one
 * translation unit.
 *
 * @version 1.0.0  @date 2026-09-29
 */
#include "../include/uiox_kix_scpcs.h"

extern int                 kernel_kill(uiox_uint32_t pid, int signum);
extern uiox_kix_psa_proc_t *uiox_kix_psa_proc_find(uiox_uint32_t pid);

int64_t uiox_kix_scpcs_kill(uiox_uint64_t pid, uiox_uint64_t sig)
{
    if (!uiox_kix_scps_current()) return SCPCS_ESRCH;

    /* Signal 0 is POSIX's existence check, not a signal to deliver.
     * Refusing it keeps the return value meaningful rather than
     * quietly reporting success for a delivery that never happens. */
    if (sig == 0u) return SCPCS_EINVAL;
    if (sig > (uiox_uint64_t)UIOX_KIX_PSA_NSIG) return SCPCS_EINVAL;

    if (pid == 0u || pid > 0x7FFFFFFFu) return SCPCS_EINVAL;

    if (!uiox_kix_psa_proc_find((uiox_uint32_t)pid)) return SCPCS_ESRCH;

    (void)kernel_kill;   /* present, does not deliver — see banner */
    return SCPCS_ENOSYS;
}
