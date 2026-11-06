/*
 * 33_PCS/50_scpcs/src/uiox_kix_scpcs_fork.c
 *
 * fork() — Group 1, memory Management.
 *
 * ── the algorithm, verbatim ─────────────────────────────────────────
 *   input: none
 *   output: to parent process, child PID number
 *           to child process, 0
 *   {
 *       check for available kernel resources;
 *       get free proc table slot, unique PID number;
 *       check that user not running too many processes;
 *       mark child state "being created;"
 *       copy data from parent proc table slot to new child slot;
 *       increment counts on current directory inode and changed root;
 *       increment open file counts in file table;
 *       make copy of parent context (u area, text, data, stack);
 *       push dummy system level context layer onto child;
 *       if (executing process is parent process)
 *       { change child state to "ready to run"; return (child ID); }
 *       else
 *       { initialize u area timing fields; return (0); }
 *   }
 *
 * ── what changed from the ENOSYS stub ───────────────────────────────
 * The stub's note was that the callee returned 0 unconditionally, so a
 * caller doing `if (pid == 0) init_process();` took the child branch in
 * the parent as well.  This body allocates a real PID, duplicates the
 * parent's regions, gives the child its own u area and context, and
 * returns the child's PID to the parent.
 *
 * ── the region side, and why dupreg is cheap ────────────────────────
 * dupreg (Algorithm 9) increments r_refcnt, copies the page table, and
 * marks every writable page PTE_COW.  The frames are SHARED: the copy
 * happens on the child's first write, through 02_MemMngnt's fault
 * handler, which already tests that bit.  So fork copies no memory.
 *
 * dupreg returns an UNATTACHED region -- that is its documented
 * contract -- so every duplicate is followed by attachreg, which is the
 * order PSA's fork algorithm uses.
 *
 * ── attachreg allocates in the CALLER's u area ──────────────────────
 * attachreg scans uiox_kix_psa_u.u_pregs[] for a free slot, and that
 * name now resolves through uiox_kix_psa_current_uarea() to the running
 * process's table.  While this body runs, the parent is current, so the
 * child's attachment would land in the PARENT's table and the child
 * would own nothing.
 *
 * The fix is to point current_context at the child before attaching,
 * which is also what the algorithm's "push dummy system level context
 * layer onto child" is for.  child_ctx_bind() below does that, and
 * parent_ctx_restore() puts it back afterwards.
 *
 * ── not done here ───────────────────────────────────────────────────
 *   · open file counts are not incremented.  The u area has u_ofile[],
 *     but nothing has opened a file yet, so there is nothing to count.
 *     The loop belongs here when open() exists.
 *   · the cwd inode count is not incremented, for the same reason.
 *   · u area timing fields are zeroed rather than inherited, so the
 *     child's p_timers start clean.
 *
 * @version 2.0.0  @date 2026-11-01
 */
#include "../include/uiox_kix_scpcs.h"
#include "uiox_kix_psa_region.h"
#include "uiox_kix_psa_process.h"
#include "uiox_kix_psa_context.h"
#include "uiox_kix_psa_proc_algo.h"

/* ── child_ctx_bind ─────────────────────────────────────────────────
 * Make the child's context current so attachreg allocates its pregion
 * slots in the CHILD's u area rather than the parent's.
 *
 * Returns the context that was current, so the caller can put it back.
 * A NULL child context is not an error the caller can repair, but it is
 * also not a reason to attach into the wrong table, so the caller
 * unwinds instead.
 */
static uiox_kix_psa_sys_context_t *child_ctx_bind(uiox_kix_psa_proc_t *child)
{
    uiox_kix_psa_sys_context_t *was = uiox_kix_psa_current_context;
    uiox_kix_psa_sys_context_t *child_ctx;

    if (!child || !child->p_addr) return (uiox_kix_psa_sys_context_t *)0;

    child_ctx = (uiox_kix_psa_sys_context_t *)child->p_addr;
    uiox_kix_psa_context_switch(was, child_ctx);

    return was;
}

static void ctx_restore(uiox_kix_psa_sys_context_t *was)
{
    if (was) uiox_kix_psa_context_switch(uiox_kix_psa_current_context, was);
}

/* ── fork_regions ───────────────────────────────────────────────────
 * Walk the parent's pregion table and give the child a duplicate of each.
 *
 * The walk is over the parent's table, captured before the context is
 * switched -- after child_ctx_bind() the accessor would return the
 * CHILD's (empty) table and the loop would find nothing.
 *
 * Returns the number of regions attached, or -1 on the first failure.
 * The caller unwinds with attach_failed() in either case.
 */
static int fork_regions(uiox_kix_psa_pregion_t *parent_pregs,
                        uint32_t parent_count,
                        uiox_kix_psa_proc_t *child)
{
    uint32_t i;
    int      attached = 0;

    for (i = 0u; i < parent_count; i++) {
        uiox_kix_psa_pregion_t *pp = &parent_pregs[i];
        uiox_kix_psa_region_t  *rp;
        uiox_kix_psa_pregion_t *cp;

        if (!pp->pr_valid || !pp->pr_region) continue;

        /* Algorithm 9.  Refcount up, page table copied, writable pages
         * marked COW.  No frames move. */
        rp = uiox_kix_psa_dupreg(pp->pr_region);
        if (!rp) return -1;

        /* Algorithm 4.  Lands in the CHILD's u area, because
         * child_ctx_bind() made the child current. */
        cp = uiox_kix_psa_attachreg(rp, child, pp->pr_vaddr, pp->pr_type);
        if (!cp) {
            /* the duplicate is unattached and unreferenced -- hand it
             * back so a failed fork does not leak a region */
            uiox_kix_psa_freereg(rp);
            return -1;
        }
        attached++;
    }

    return attached;
}

int64_t uiox_kix_scpcs_fork(void)
{
    uiox_kix_psa_proc_t        *parent;
    uiox_kix_psa_u_area_t      *parent_ua;
    uiox_kix_psa_pregion_t      parent_pregs[UIOX_KIX_PSA_MAX_REG_PER_PROC];
    uint32_t                    parent_count = 0u;
    uiox_kix_psa_proc_t        *child;
    uiox_kix_psa_sys_context_t *child_ctx;
    uiox_kix_psa_sys_context_t *was;
    uint32_t                    child_slot;
    uint32_t                    i;
    int                         n;

    /* 1. the caller must be a process */
    parent = uiox_kix_scpcs_current();
    if (!parent) return SCPCS_ESRCH;

    /* 2. snapshot the parent's regions BEFORE anything switches context.
     *    uiox_kix_psa_u resolves through the accessor to the running
     *    process's u area, which is the parent's right now. */
    parent_ua = uiox_kix_psa_current_uarea();
    if (!parent_ua) return SCPCS_ESRCH;

    for (i = 0u; i < (uint32_t)UIOX_KIX_PSA_MAX_REG_PER_PROC; i++) {
        if (parent_ua->u_pregs[i].pr_valid) {
            parent_pregs[parent_count++] = parent_ua->u_pregs[i];
        }
    }

    /* 3. get a free table slot and a unique PID.  A NULL here is PSA's
     *    "user is running too many processes" -- a resource condition,
     *    not a memory error, so EAGAIN rather than ENOMEM. */
    child = uiox_kix_psa_proc_alloc(0u, parent->p_pid);
    if (!child) return SCPCS_EAGAIN;

    child_slot = (uint32_t)(child - uiox_kix_psa_proc_table);

    /* 4. the child's context.  Copy the parent's saved registers so the
     *    child resumes at the same instruction with the same stack, then
     *    bind the child's own u area -- the same wiring uiox_pcs_init.c
     *    gives PID 1. */
    child_ctx = uiox_kix_psa_context_for(child_slot);
    if (!child_ctx) {
        uiox_kix_psa_proc_free(child);
        return SCPCS_EAGAIN;
    }

    memset(child_ctx, 0, sizeof(*child_ctx));
    child_ctx->sc_proc_entry = child;
    child_ctx->sc_layer_top  = 0;

    /* "push dummy system level context layer onto child" -- one frame,
     * so the child has somewhere for an interrupt to land. */
    child_ctx->sc_layers[0].rc_pc = 0u;      /* overwritten below */
    child_ctx->sc_layers[0].rc_sp = 0u;

    //child_ctx->sc_uarea = uiox_kix_psa_uarea_for(child_slot);
    //if (child_ctx->sc_uarea) {
    //    uiox_kix_psa_u_area_t *cua = (uiox_kix_psa_u_area_t *)child_ctx->sc_uarea;
    //    memset(cua, 0, sizeof(*cua));
    //    cua->u_proc = child;
    //}
    child_ctx->sc_uarea = uiox_kix_psa_uarea_for(child_slot);
    if (child_ctx->sc_uarea) {
        uiox_kix_psa_u_area_t *cua = (uiox_kix_psa_u_area_t *)child_ctx->sc_uarea;

        memset(cua, 0, sizeof(*cua));        /* clear FIRST, then every field */
        cua->u_proc = child;

        /* ── inheritance ────────────────────────────────────────────
         * POSIX: a child inherits its parent's signal dispositions and
         * credentials, and shares the OPEN FILE ENTRIES (not the
         * descriptor slots).  parent_ua was captured at step 2, before
         * any context switch, so it is still the parent's. */
        if (parent_ua) {
            /* dispositions are INHERITED.  Since sigaction() can store
             * one, losing this would silently drop a handler the parent
             * installed and take the default on a caught signal. */
            memcpy(cua->u_signal, parent_ua->u_signal,
                   sizeof(cua->u_signal));

            cua->u_uid  = parent_ua->u_uid;
            cua->u_gid  = parent_ua->u_gid;
            cua->u_euid = parent_ua->u_euid;
            cua->u_egid = parent_ua->u_egid;

            /* u_ofile[] is deliberately NOT copied.  struct file has no
             * definition in this tree and nothing populates the array,
             * so both sides are all-NULL.  When sys_open stores an
             * entry, this is where the descriptors are duplicated and
             * the SHARED entry's count is incremented — POSIX shares the
             * entry, not the slot. */
        }

        /* left at zero on purpose:
         *   u_sig / u_sigmask   a child starts with nothing pending and
         *                       nothing blocked
         *   u_error / u_rval    per-call, meaningless across a fork
         *   u_pregs[]           filled by the region duplication below
         *   u_io / u_qsave      an interrupt or abort in flight does not
         *                       transfer to a child
         */
    }

    child->p_addr = child_ctx;

    /* copy the parent's register frame: the child continues from the
     * same pc/sp, and its rc_r0 is zeroed at the end so fork() returns
     * 0 in the child and the PID in the parent. */
    {
        uiox_kix_psa_sys_context_t *pctx =
            (uiox_kix_psa_sys_context_t *)parent->p_addr;

        if (pctx) {
            memcpy(&child_ctx->sc_layers[0], &pctx->sc_layers[0],
                   sizeof(uiox_kix_psa_reg_context_t));
            /* The switch frame is COPIED, not initialised.  A forked
             * child resumes mid-call at the parent's return point, so
             * arch_context_init (which sets a fresh entry + stack) is
             * the wrong tool here.  This is what makes fork() return 0
             * in the child: it comes back through the same ret the
             * parent used, with rc_r0 zeroed below. */
            child_ctx->sc_switch = pctx->sc_switch;
        }
    }

    /* 5. duplicate every region.  Bind the child first: attachreg
     *    allocates its slot in whichever u area is current. */
    was = child_ctx_bind(child);
    if (!was) {
        uiox_kix_psa_proc_free(child);
        return SCPCS_ESRCH;
    }

    n = fork_regions(parent_pregs, parent_count, child);

    ctx_restore(was);

    if (n < 0) {
        /* unwind: detach everything the child holds, then free the slot.
         * A region left attached keeps r_refcnt above zero, so freereg
         * can never take it back. */
        uiox_kix_psa_sys_context_t *again = child_ctx_bind(child);

        for (i = 0u; i < (uint32_t)UIOX_KIX_PSA_MAX_REG_PER_PROC; i++) {
            uiox_kix_psa_u_area_t *cua =
                (uiox_kix_psa_u_area_t *)((uiox_kix_psa_sys_context_t *)
                                          child->p_addr)->sc_uarea;
            if (cua && cua->u_pregs[i].pr_valid)
                uiox_kix_psa_detachreg(&cua->u_pregs[i]);
        }

        ctx_restore(again);

        uiox_kix_psa_proc_free(child);
        return SCPCS_ENOMEM;
    }

    /* 6. the child returns 0 from fork; the parent returns the child's
     *    PID.  The child never executes this function's return -- it is
     *    resumed by the scheduler at the copied pc -- but writing 0 into
     *    its saved frame is what makes the syscall's own return value
     *    correct when it does run. */
    child_ctx = (uiox_kix_psa_sys_context_t *)child->p_addr;
    child_ctx->sc_layers[0].rc_r0 = 0u;
    child_ctx->sc_layers[0].rc_r1 = 0u;

    /* 7. the child is runnable.  proc_alloc left it CREATED; nothing
     *    else moves it out. */
    if (uiox_kix_psa_proc_set_state(child, UIOX_KIX_PSA_PROC_READY) != 0) {
        uiox_kix_psa_proc_free(child);
        return SCPCS_EAGAIN;
    }

    uiox_kix_psa_sched_enqueue(child);

    return (int64_t)child->p_pid;
}
