/*
 * 33_PCS/50_scpcs/src/uiox_kix_scpcs_get_pid.c
 *
 * get_pid() — Group 3, Miscellaneous.
 *
 * Returns the process ID, read from p_pid.
 *
 * Real.  The field is on the process table entry and the allocator assigns it.
 *
 * A missing current process yields ESRCH rather than 0.  Zero is a
 * legal identifier — the swapper holds PID 0 — so returning it for "no
 * process" would make a caller believe it was the swapper.
 *
 * @version 1.0.0  @date 2026-09-29
 */
#include "../include/uiox_kix_scpcs.h"

int64_t uiox_kix_scpcs_get_pid(void)
{
    uiox_kix_psa_proc_t *p = uiox_kix_scpcs_current();

    if (!p) return SCPCS_ESRCH;
    return (int64_t)p->p_pid;
}
