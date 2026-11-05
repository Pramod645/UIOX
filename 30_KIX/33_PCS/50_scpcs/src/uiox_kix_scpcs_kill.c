/*
 * 33_PCS/50_scpcs/src/uiox_kix_scpcs_kill.c
 *
 * kill() — Group 2, Synchronization.
 *
 * kill sits with wait and signal because the kernel synchronizes exit and
 * wait THROUGH signals.
 *
 * ── what this does now ──────────────────────────────────────────────
 * It resolves the target PID and MARKS the signal pending on that
 * process, via uiox_kix_scpcs_send_signal().  Delivery is a separate
 * step that runs on the way back to EL0, from the per-arch signal check
 * hung off the SVC return — marking is what a sender can do without
 * reaching into the target's return path.
 *
 * ── who receives it ─────────────────────────────────────────────────
 * The TARGET, from the table walk — not the caller.  Sending to
 * uiox_kix_scpcs_current() would compile, link and build clean, and
 * signal the wrong process.
 *
 * ── the PID forms that cannot be expressed ──────────────────────────
 * POSIX kill(0, sig) signals the caller's process group and kill(-1,
 * sig) every process it may signal.  Neither can be built: no p_pgrp to
 * enumerate, no permission model to decide who may signal whom.  Both
 * are refused with EINVAL rather than silently signalling only self.
 *
 * ── signal numbering ────────────────────────────────────────────────
 * Numbers come from the layer's own table, which places SIGCHLD at 17.
 * A second table elsewhere places it at 18; that disagreement is a live
 * bug the moment both are in one translation unit.
 *
 * @version 1.0.0  @date 2026-09-29
 */
#include "../include/uiox_kix_scpcs.h"

extern uiox_kix_psa_proc_t *uiox_kix_psa_proc_find(uiox_uint32_t pid);

int64_t uiox_kix_scpcs_kill(uiox_uint64_t pid, uiox_uint64_t sig)
{
    uiox_kix_psa_proc_t *target;

    if (!uiox_kix_scpcs_current()) return SCPCS_ESRCH;

    /* Signal 0 is POSIX's existence check, not a signal to deliver. */
    if (sig == 0u) return SCPCS_EINVAL;
    if (sig > (uiox_uint64_t)UIOX_KIX_PSA_NSIG) return SCPCS_EINVAL;

    if (pid == 0u || pid > 0x7FFFFFFFu) return SCPCS_EINVAL;

    /* the TARGET, not the caller — see the banner */
    target = uiox_kix_psa_proc_find((uiox_uint32_t)pid);
    if (!target) return SCPCS_ESRCH;

    uiox_kix_scpcs_send_signal(target, (int)sig);
    return SCPCS_EOK;
}
