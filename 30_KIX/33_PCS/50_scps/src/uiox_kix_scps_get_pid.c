/*
 * 33_PCS/50_scps/src/uiox_kix_scps_get_pid.c
 *
 * getpid() — Miscellaneous.  REAL: p_pid is on proc_t and proc_alloc()
 * assigns it.  This is the one POSIX call the layer fully supports.
 */
#include "../include/uiox_kix_scps.h"

int64_t uiox_kix_scps_get_pid(void)
{
    proc_t *p = uiox_kix_scps_current();
    if (!p) return SCPS_ESRCH;
    return (int64_t)p->p_pid;
}
