/*
 * 33_PCS/50_scpcs/src/uiox_kix_scpcs_exit.c
 *
 * exit() — Group 1, memory Management.
 *
 * ── the algorithm, verbatim ─────────────────────────────────────────
 *   {
 *       ignore all signals;
 *       if (process group leader with associated control terminal)
 *       { send hangup signal to all members of process group;
 *         reset process group for all members to 0; }
 *       close all open files (internal version of algorithm close);
 *       release current directory (algorithm iput);
 *       release current (changed) root, if exists (algorithm iput);
 *       free regions, memory associated with process (algorithm freereg);
 *       write accounting record;
 *       make process state zombie
 *       assign parent process ID of all child processes to be init (1);
 *           if any children were zombie, send death of child signal to init;
 *       send death of child signal to parent process;
 *       context switch;
 *   }
 *
 * The grouping lists exit as: detachreg — uiox_kix_psa_detachreg(),
 * PSA Algorithm 8, implemented.
 *
 * ── the two steps that block wait() ─────────────────────────────────
 * The callee prints those steps and returns 0.  It does NOT set the state
 * to ZOMBIE and does NOT send SIGCHLD.  Those are precisely the two
 * conditions wait's loop tests for, which is why every wait currently
 * fails: nothing ever becomes a zombie, so nothing is ever reapable.
 *
 * Note the state transition needed here — KERNEL_RUNNING to ZOMBIE — IS
 * drawn in the 40psa state diagram and IS accepted by
 * uiox_kix_psa_proc_set_state.  The table is ready; only the call is
 * missing.
 *
 * ── status encoding ─────────────────────────────────────────────────
 * The low byte stays clear for a terminating signal number, so an exit
 * code occupies the upper bits.  A caller passing more than 8 bits is
 * refused rather than truncated: silently dropping the high bits would
 * deliver a status the caller never asked for.
 *
 * @version 1.0.0  @date 2026-09-29
 */
#include "../include/uiox_kix_scps.h"

extern int kernel_exit(int status);

int64_t uiox_kix_scpcs_exit(uiox_uint64_t exit_code)
{
    uiox_kix_psa_proc_t *p = uiox_kix_scps_current();

    if (!p) return SCPS_ESRCH;
    if (exit_code > 0xFFu) return SCPS_EINVAL;   /* status is 8 bits */

    /* A process already in the terminal state has nothing left to do.
     * The state diagram refuses every transition out of ZOMBIE, so
     * falling through to the callee would fail there anyway. */
    if (p->p_state == UIOX_KIX_PSA_PROC_ZOMBIE) return SCPS_EOK;

    (void)kernel_exit((int)exit_code);
    return SCPS_EOK;
}
