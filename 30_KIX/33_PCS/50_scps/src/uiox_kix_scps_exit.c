/*
 * 33_PCS/50_scps/src/uiox_kix_scps_exit.c
 *
 * exit() — Group 1, Memory Management.
 *
 * ── SCPS Algorithm 4, verbatim ────────────────────────────────────────
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
 * The SCPS grouping lists exit as: detachreg  (region.c, PSA Algorithm 8)
 *
 * ── the two lines that block wait ─────────────────────────────────────
 * kernel_exit() prints the steps and returns 0.  It does NOT set
 * p_state = PROC_ZOMBIE, and does NOT send SIGCHLD.  Those are exactly
 * the two conditions Algorithm 5's loop tests for, which is why every
 * wait() currently fails: nothing ever becomes a zombie.
 *
 * The wrapper is correct and can be committed as-is.  The body it calls
 * needs the zombie transition and the signal.
 *
 * ── status encoding ───────────────────────────────────────────────────
 * EXIT_NORMAL(code) = code << 8, leaving the low byte clear for a
 * terminating signal number.  Passed straight through.
 *
 * @version 2.0.0  @date 2026-09-28
 */
#include "../include/uiox_kix_scps.h"

extern int kernel_exit(int status);   /* 40_procStruct/src/exit_wait.c */

int64_t uiox_kix_scps_exit(uiox_uint64_t exit_code)
{
    proc_t *p = uiox_kix_scps_current();

    if (!p) return SCPS_ESRCH;
    if (exit_code > 0xFFu) return SCPS_EINVAL;   /* status is 8 bits */
    if (!(p->p_state != PROC_ZOMBIE)) return SCPS_EOK;  /* already dead */

    (void)kernel_exit((int)exit_code);
    return SCPS_EOK;
}
