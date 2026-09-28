/*
 * 33_PCS/50_scpcs/src/uiox_kix_scpcs_get_euid.c
 *
 * get_euid() — Group 3, Miscellaneous.
 *
 * Returns the effective user ID, read from p_euid.
 *
 * Real, and this is the one that decides access.  The exec path already writes it when a setuid image runs; making it readable is half of making it settable.
 *
 * A missing current process yields ESRCH rather than 0.  Zero is a
 * legal identifier — the swapper holds PID 0 — so returning it for "no
 * process" would make a caller believe it was the swapper.
 *
 * @version 1.0.0  @date 2026-09-29
 */
#include "../include/uiox_kix_scpcs.h"

int64_t uiox_kix_scpcs_get_euid(void)
{
    uiox_kix_psa_proc_t *p = uiox_kix_scpcs_current();

    if (!p) return SCPCS_ESRCH;
    return (int64_t)p->p_euid;
}
