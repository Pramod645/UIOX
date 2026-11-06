/*
 * 33_PCS/50_scpcs/src/uiox_kix_scpcs_sig_action.c
 *
 * sigaction() — Group 2, Synchronization.
 *
 * ── where the disposition lives ─────────────────────────────────────
 * On the u area: u_signal[] in uiox_kix_psa_u_area_t, indexed by signal
 * number.  Reached through uiox_kix_psa_uarea_for(slot) — no cast, the
 * same accessor brk.c uses.
 *
 * The earlier note here said dispositions sat on "a PRIVATE structure of
 * its own, reached by casting a process pointer to it".  That was wrong
 * twice: the array is on the u area (proc_algo.h:88, one field among
 * u_ofile and u_pregs), and uarea_for() reaches it by index.
 *
 * ── what is stored, and what the values mean ────────────────────────
 * One int per signal — POSIX's handler argument, not its struct
 * sigaction:
 *
 *     0   SIG_DFL   default: fatal terminates, non-fatal ignored
 *    -1   SIG_IGN   the process asked not to be disturbed
 *     x   a handler address
 *
 * The u area is in .bss and cleared at boot, so every entry starts as
 * SIG_DFL by construction.
 *
 * ── the one rule not duplicated here ────────────────────────────────
 * SIGKILL and SIGSTOP cannot be caught or ignored, and the callee
 * enforces that.  The wrapper does not repeat the check: two copies of a
 * rule are two things to keep in step.
 *
 * ── oldact is the disposition BEFORE the change ─────────────────────
 * Captured before act is applied, so a caller can save and restore —
 * the same ordering sigprocmask uses for oldset.
 *
 * @version 1.0.0  @date 2026-09-29
 */

#include "../include/uiox_kix_scpcs.h"
#include "uiox_kix_psa_proc_algo.h"

int64_t uiox_kix_scpcs_sig_action(uiox_uint64_t sig, uiox_uintptr_t act,
    uiox_uintptr_t oldact)
{
uiox_kix_psa_proc_t   *p = uiox_kix_scpcs_current();
uiox_kix_psa_u_area_t *u;
uint32_t               slot;
int                    newact, old;

if (!p) return SCPCS_ESRCH;
if (sig == 0u || sig > (uiox_uint64_t)UIOX_KIX_PSA_NSIG)
return SCPCS_EINVAL;

/* the two dispositions nothing may change */
if (sig == (uiox_uint64_t)UIOX_KIX_PSA_SIGKILL ||
sig == (uiox_uint64_t)UIOX_KIX_PSA_SIGSTOP)
return SCPCS_EINVAL;

slot = (uint32_t)(p - uiox_kix_psa_proc_table);
u    = uiox_kix_psa_uarea_for(slot);
if (!u) return SCPCS_ESRCH;

old = u->u_signal[sig];                 /* capture BEFORE the change */

if (act != 0u) {
if (uiox_kix_scpcs_copy_from_user(&newact, act, sizeof(newact)) != 0)
return SCPCS_EFAULT;
u->u_signal[sig] = newact;
}

if (oldact != 0u) {
if (uiox_kix_scpcs_copy_to_user(oldact, &old, sizeof(old)) != 0)
return SCPCS_EFAULT;
}

return SCPCS_EOK;
}

