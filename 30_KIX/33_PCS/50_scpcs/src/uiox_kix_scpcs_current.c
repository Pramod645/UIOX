/*
 * 33_PCS/50_scpcs/src/uiox_kix_scpcs_current.c
 *
 * THE ONE PLACE a running-process pointer is obtained.
 *
 * ── why this file exists at all ─────────────────────────────────────
 * Several shapes of "the current process" coexist in 30_KIX:
 *
 *   1. uiox_kix_psa_proc_t   40psa's process table entry — what the
 *                            40psa algorithms are written against
 *   2. uiox_task_t           a second descriptor in the same tree
 *   3. sim_proc_t            a private typedef inside 40psa's signal.c
 *   4. a pe_* shaped struct  02_MemMngnt/src/page_fault.c reads
 *                            ->pe_pid, while 02_MemMngnt/src/clock.c
 *                            reads p_* on the SAME symbol
 *
 * Every entry point in this layer calls this accessor, so the question
 * "what type is the current process" has one answer to correct rather
 * than twenty-five.
 *
 * ── the single symbol it resolves ───────────────────────────────────
 * uiox_kix_psa_current_proc, defined in 40psa's uiox_kix_psa_process.c
 * and typed there as uiox_kix_psa_proc_t *.  If the declaration ever
 * disagrees with this file, that disagreement IS the bug — not something
 * to paper over with a cast here.
 *
 * @version 1.0.0  @date 2026-09-29
 */
#include "../include/uiox_kix_scps.h"

extern uiox_kix_psa_proc_t *uiox_kix_psa_current_proc;

uiox_kix_psa_proc_t *uiox_kix_scps_current(void)
{
    return uiox_kix_psa_current_proc;
}
