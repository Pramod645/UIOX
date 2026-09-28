/*
 * 33_PCS/50_scps/src/uiox_kix_scps_current.c
 *
 * THE ONE PLACE a running-process pointer is obtained.
 *
 * Four process models coexist in 30_KIX:
 *   1. proc_t            40_procStruct/include/process.h   <- this layer
 *   2. uiox_task_t       40_procStruct/include/uiox_task.h
 *   3. sim_proc_t        a private typedef inside signal.c
 *   4. pe_* struct       02_MemMngnt/src/page_fault.c uses
 *                        current_proc->pe_pid, while 02_MemMngnt/src/
 *                        clock.c uses p_* on the same symbol
 *
 * current_proc is declared somewhere and its SPELLING is not settled.
 * Every entry point calls this accessor, so that one question has one
 * place to be corrected instead of twenty-five.
 *
 * TO SETTLE: whichever header declares current_proc decides.  If it is
 * proc_t, this file is already correct.  If it is the pe_* struct, then
 * 40_procStruct and 02_MemMngnt are describing different objects under
 * one name and THAT is the bug to fix — not this accessor.
 *
 * @version 2.0.0  @date 2026-09-28
 */
#include "../include/uiox_kix_scps.h"

extern proc_t *current_proc;   /* declared in process.h */

proc_t *uiox_kix_scps_current(void)
{
    return current_proc;
}
