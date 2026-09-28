/*
 * 33_PCS/50_scpcs/src/uiox_kix_scpcs_get_ppid.c
 *
 * get_ppid() — Group 3, Miscellaneous.
 *
 * Returns the parent's PID, read from p_ppid.
 *
 * Real from the field, but the value is not yet trustworthy: fork currently returns 0 to the parent as well as the child, so the parent recorded here never learned it had a child.
 *
 * A missing current process yields ESRCH rather than 0.  Zero is a
 * legal identifier — the swapper holds PID 0 — so returning it for "no
 * process" would make a caller believe it was the swapper.
 *
 * @version 1.0.0  @date 2026-09-29
 */
#include "../include/uiox_kix_scps.h"

int64_t uiox_kix_scps_get_ppid(void)
{
    uiox_kix_psa_proc_t *p = uiox_kix_scps_current();

    if (!p) return SCPS_ESRCH;
    return (int64_t)p->p_ppid;
}
