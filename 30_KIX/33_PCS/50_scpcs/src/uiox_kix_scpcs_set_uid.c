/*
 * 33_PCS/50_scpcs/src/uiox_kix_scpcs_set_uid.c
 *
 * setuid() — Group 3, Miscellaneous.
 *
 * ── this is the callable form of something already happening ────────
 * The exec path makes a setuid transition at exec time by writing the
 * effective user ID directly.  So the WRITE is known to work; what this
 * adds is a caller-supplied, permission-checked version of it.
 *
 * ── the permission rule ─────────────────────────────────────────────
 *   effective ID is 0                   set both real and effective
 *   requested equals the real ID        set effective only
 *   requested equals the effective ID   set effective only
 *   otherwise                           EPERM
 *
 * The privileged case is the one that may take an identity it does not
 * already hold.  Both other cases are permitted because a process may
 * always re-assert who it already is — which is how setuid is used to
 * drop privilege permanently after it has been used.
 *
 * Every field that rule needs exists, which is why this returns EPERM
 * rather than ENOSYS: a refusal here is a real decision, not a gap.
 *
 * ── what cannot be expressed ────────────────────────────────────────
 * There is no saved-ID field, so the POSIX "restore a previous identity"
 * half cannot be built.  Setting is the half that can.
 *
 * ── the two fields, deliberately ────────────────────────────────────
 * The privileged path sets BOTH IDs.  Setting only the effective one
 * would leave a process able to step back up to its privileged real ID
 * after dropping, which is a privilege leak rather than a convenience.
 *
 * @version 1.0.0  @date 2026-09-29
 */
#include "../include/uiox_kix_scpcs.h"

int64_t uiox_kix_scpcs_set_uid(uiox_uint64_t uid)
{
    uiox_kix_psa_proc_t *p = uiox_kix_scps_current();
    uiox_uint32_t        u;

    if (!p) return SCPCS_ESRCH;
    if (uid > 0xFFFFFFFFu) return SCPCS_EINVAL;
    u = (uiox_uint32_t)uid;

    if (p->p_euid == 0u) {                  /* privileged */
        p->p_uid  = (uiox_uint16_t)u;
        p->p_euid = (uiox_uint16_t)u;
        return SCPCS_EOK;
    }

    if (u == (uiox_uint32_t)p->p_uid ||
        u == (uiox_uint32_t)p->p_euid) {    /* re-asserting self */
        p->p_euid = (uiox_uint16_t)u;
        return SCPCS_EOK;
    }

    return SCPCS_EPERM;                      /* may not take a new identity */
}
