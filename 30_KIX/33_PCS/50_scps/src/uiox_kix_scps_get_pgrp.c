/*
 * 33_PCS/50_scps/src/uiox_kix_scps_get_pgrp.c
 *
 * getpgrp() — Miscellaneous.  setpgrp is named in the SCPS text's
 * Miscellaneous column, so the CONCEPT is in scope; what is missing is
 * anywhere to store it.
 *
 * proc_t has no process-group field.  signal.c's private sim_proc_t HAS
 * one (sp_pgid) — which is the shape of the problem: the field exists
 * on the model nobody uses and is absent from the model everything
 * binds to.
 *
 * TO FIX: add p_pgrp to proc_t (or to the u_area proc_t points at) and
 * set it in the fork path, then this becomes a one-field return like
 * getpid().  Until then, ENOSYS — not 0, which would claim the caller
 * is in process group 0 (the swapper).
 *
 * @version 2.0.0  @date 2026-09-28
 */
#include "../include/uiox_kix_scps.h"

int64_t uiox_kix_scps_get_pgrp(void)
{
    if (!uiox_kix_scps_current()) return SCPS_ESRCH;
    return SCPS_ENOSYS;     /* no p_pgrp on proc_t */
}
