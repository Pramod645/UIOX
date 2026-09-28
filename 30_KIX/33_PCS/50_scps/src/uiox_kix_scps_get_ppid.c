/*
 * 33_PCS/50_scps/src/uiox_kix_scps_get_ppid.c
 *
 * getppid() — REAL from p_ppid, but see fork.
 *
 * SCPS Algorithm 1 says "output: to parent process, child PID number;
 * to child process 0".  kernel_fork() returns 0 for BOTH, so p_ppid is
 * populated from a parent that was never told it had a child.  The
 * field is right; the value is not yet.  This call exposes that defect
 * rather than causing it.
 */
#include "../include/uiox_kix_scps.h"

int64_t uiox_kix_scps_get_ppid(void)
{
    proc_t *p = uiox_kix_scps_current();
    if (!p) return SCPS_ESRCH;
    return (int64_t)p->p_ppid;
}
