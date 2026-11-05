/*
* 33_PCS/50_scpcs/src/uiox_kix_scpcs_sig_procmask.c
*
 * sigprocmask() — Group 2, Synchronization.
 *
 * ── why this works now ──────────────────────────────────────────────
 * Both blockers that made this return ENOSYS are gone:
 *
 *   1. set and oldset are user addresses.  copy_from_user and
 *      copy_to_user now exist beside the gate, so a caller can hand in a
 *      mask and read the previous one back.
 *   2. The reader that decides deliverability tests the REAL fields.
 *      uiox_kix_psa_proc_sig_pending() computes p_sig & ~p_sigmask, and
 *      the syscall-side reader that had the mask inverted (sigpending)
 *      has been fixed — so a mask set here is honoured rather than
 *      ignored.
 *
 * ── the bit convention ──────────────────────────────────────────────
 *   bit number == signal number
 *
 * send_signal() sets p_sig with (1u << signum); this writes p_sigmask
 * with the same shift, because the two are masked against each other
 * bit-for-bit.  A different shift in either field makes every signal
 * report as deliverable, or none, silently.
 *
 * ── oldset is the mask BEFORE the change ────────────────────────────
 * Captured before `set` is applied, which is what makes the standard
 * save-modify-restore idiom work.  Because the write happens after the
 * mask has changed, a failing oldset copy leaves the new mask in place —
 * POSIX's own ordering, not an oversight.
*
* @version 1.0.0  @date 2026-09-29
*/
#include "../include/uiox_kix_scpcs.h"

#ifndef SIG_BLOCK
#define SIG_BLOCK   0
#endif
#ifndef SIG_UNBLOCK
#define SIG_UNBLOCK 1
#endif
#ifndef SIG_SETMASK
#define SIG_SETMASK 2
#endif

int64_t uiox_kix_scpcs_sig_procmask(uiox_uint64_t how, uiox_uintptr_t set,
                                    uiox_uintptr_t oldset)
{
    uiox_kix_psa_proc_t *p = uiox_kix_scpcs_current();
    uiox_uint32_t prev;
    uiox_uint32_t newmask;
    int           m;

    if (!p) return SCPCS_ESRCH;

    if (how != (uiox_uint64_t)SIG_BLOCK   &&
        how != (uiox_uint64_t)SIG_UNBLOCK &&
        how != (uiox_uint64_t)SIG_SETMASK)
        return SCPCS_EINVAL;

    /* captured BEFORE the change: oldset is defined to return the mask
     * the caller had, which is what makes save-modify-restore work */
    prev = p->p_sigmask;

    if (set != 0u) {
        if (uiox_kix_scpcs_copy_from_user(&m, set, sizeof(m)) != 0)
            return SCPCS_EFAULT;
        newmask = (uiox_uint32_t)m;

        /* bit number == signal number — the convention send_signal sets,
         * so a mask bit means the signal of the same index */
        switch ((int)how) {
        case SIG_BLOCK:   p->p_sigmask |=  newmask;  break;
        case SIG_UNBLOCK: p->p_sigmask &= ~newmask;  break;
        case SIG_SETMASK: p->p_sigmask  =  newmask;  break;
        }
    }

    if (oldset != 0u) {
        if (uiox_kix_scpcs_copy_to_user(oldset, &prev, sizeof(prev)) != 0)
            return SCPCS_EFAULT;
    }

    return SCPCS_EOK;
}
