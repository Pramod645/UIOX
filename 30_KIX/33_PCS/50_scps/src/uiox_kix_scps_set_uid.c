/*
 * 33_PCS/50_scps/src/uiox_kix_scps_set_uid.c
 *
 * setuid() — Miscellaneous, named in the SCPS text.
 *
 * This is the call the EXEC PATH already needed: exec.c's
 * exec_handle_setuid makes a setuid transition at exec time by writing
 * p_euid directly.  So the write is known-good; what is missing is the
 * caller-supplied, permission-checked version of it.
 *
 * ── the permission rule (POSIX) ───────────────────────────────────────
 *   euid == 0                    -> set both p_uid and p_euid
 *   uid == p_uid || uid == p_euid-> set p_euid only
 *   otherwise                    -> EPERM
 *
 * A privileged process is the one that may take an identity it does not
 * already hold.  That test is implementable today from fields that
 * exist, and is why this file returns EPERM rather than ENOSYS.
 *
 * ── what is NOT implementable yet ─────────────────────────────────────
 * There is no saved-uid (p_suid) on proc_t, so the "restore a previous
 * identity" half of POSIX setuid cannot be expressed.  Setting is the
 * half that can.
 *
 * @version 2.0.0  @date 2026-09-28
 */
#include "../include/uiox_kix_scps.h"

int64_t uiox_kix_scps_set_uid(uiox_uint64_t uid)
{
    proc_t *p = uiox_kix_scps_current();
    uiox_uint32_t u;

    if (!p) return SCPS_ESRCH;
    if (uid > 0xFFFFFFFFu) return SCPS_EINVAL;
    u = (uiox_uint32_t)uid;

    if (p->p_euid == 0u) {                 /* privileged */
        p->p_uid  = u;
        p->p_euid = u;
        return SCPS_EOK;
    }
    if (u == p->p_uid || u == p->p_euid) { /* same identity */
        p->p_euid = u;
        return SCPS_EOK;
    }
    return SCPS_EPERM;                     /* may not take a new identity */
}
