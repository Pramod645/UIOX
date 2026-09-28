/*
 * 33_PCS/50_scpcs/src/uiox_kix_scpcs_get_gid.c
 *
 * get_gid() — Group 3, Miscellaneous.
 *
 * Returns the real group ID, read from p_gid.
 *
 * Real.  Same shape as the user-ID pair.
 *
 * A missing current process yields ESRCH rather than 0.  Zero is a
 * legal identifier — the swapper holds PID 0 — so returning it for "no
 * process" would make a caller believe it was the swapper.
 *
 * @version 1.0.0  @date 2026-09-29
 */
#include "../include/uiox_kix_scps.h"

int64_t uiox_kix_scps_get_gid(void)
{
    uiox_kix_psa_proc_t *p = uiox_kix_scps_current();

    if (!p) return SCPS_ESRCH;
    return (int64_t)p->p_gid;
}
