/*
 * 33_PCS/50_scps/src/uiox_kix_scps_get_egid.c
 *
 * getegid() — Miscellaneous/credentials.  REAL: reads proc_t.p_egid.
 *
 * effective group ID.
 *
 * This is the credential the exec path already consults — see
 * exec_handle_setuid in 40_procStruct/src/exec.c, which had to make a
 * setuid decision at exec time.  Making it READABLE from a syscall is
 * the first half of making it settable.
 */
#include "../include/uiox_kix_scps.h"

int64_t uiox_kix_scps_get_egid(void)
{
    proc_t *p = uiox_kix_scps_current();
    if (!p) return SCPS_ESRCH;
    return (int64_t)p->p_egid;
}
