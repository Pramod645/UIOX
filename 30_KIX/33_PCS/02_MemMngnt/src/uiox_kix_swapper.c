/*
 * 30_KIX/33_PCS/02_MemMngnt/src/swapper.c
 *
 * Bach's Algorithm 1 (malloc, on a resource map) and Algorithm 2
 * (swapper) — swap out other processes to make room, swap in those that
 * have waited longest.
 *
 * ── bound to 40_psa ─────────────────────────────────────────────────
 * Every process argument below is uiox_kix_psa_proc_t — the ONE process
 * type.  An earlier version used a proc_entry_t from a rival scheduler
 * header with its own pe_* fields and its own eight-value state enum.
 * That pair is gone; the state mapping that replaced it is:
 *
 *   SCHED_UNUSED      -> UIOX_KIX_PSA_PROC_UNUSED
 *   SCHED_READY       -> UIOX_KIX_PSA_PROC_READY
 *   SCHED_READY_SWAP  -> UIOX_KIX_PSA_PROC_READY_SWAPPED
 *   SCHED_SLEEP       -> UIOX_KIX_PSA_PROC_SLEEP_MEM
 *   SCHED_SLEEP_SWAP  -> UIOX_KIX_PSA_PROC_SLEEP_SWAPPED
 *   SCHED_ZOMBIE      -> UIOX_KIX_PSA_PROC_ZOMBIE
 *   SCHED_CREATED     -> UIOX_KIX_PSA_PROC_CREATED
 *
 * SCHED_RUNNING has no single counterpart: a process on the CPU is
 * either USER_RUNNING or KERNEL_RUNNING depending on where the trap
 * caught it, so the two are tested together wherever "is it running"
 * is asked.
 *
 * ── what is real and what is not ────────────────────────────────────
 * The BOOKKEEPING is real: blocks are claimed through the map, frames
 * are freed and re-claimed, states move.  The DATA TRANSFER is not —
 * nothing writes a process image to the device or reads it back.  A
 * swapped process's contents do not survive this build, and the banner
 * says so rather than leaving it to be discovered.
 *
 * ── the wakeup contract ─────────────────────────────────────────────
 * Bach's swapper SLEEPS on "event must swap in".  This one RETURNS, so
 * that the caller — the clock interrupt, or whoever noticed memory
 * running short — can decide when to re-enter.  That makes re-entry the
 * caller's job: uiox_kix_wakeup_swapper() clears the flag AND calls
 * uiox_kix_swapper(), because clearing alone would be a stall dressed
 * as a wakeup.  See that function's note.
 *
 * @version 3.0.0  @date 2026-09-29
 */
#include "../include/uiox_kix_swap.h"

/* ── Globals ────────────────────────────────────────────────────────── */
uiox_kix_swap_device_t uiox_kix_swap_device;
uiox_kix_phys_mem_t    uiox_kix_phys_mem;
uiox_kix_res_map_t     uiox_kix_swap_map;
int                    uiox_kix_swapper_sleep = 0;

/* ── Algorithm 1 — uiox_kix_map_malloc ───────────────────────────────
 * Bach, verbatim:
 *
 *   {
 *       for (every map entry)
 *       {
 *           if (current map entry can fit requested units)
 *           {
 *               if (requested units == number of units in entry)
 *                   delete entry from map;
 *               else
 *                   adjust start address of entry;
 *               return (original address of entry);
 *           }
 *       }
 *       return (0);
 *   }
 *
 * FIRST-FIT: the first entry that fits wins, without looking for a
 * tighter one.  That is deliberate here even though best-fit would
 * waste less — Bach's note explains why: swap allocation is transitory
 * and speed is critical, so the kernel takes "contiguous space without
 * regard for fragmentation".  The first fit is found sooner.
 *
 * An exact fit CLEARS the entry rather than shrinking it to nothing, so
 * the slot can be reused without a compaction pass.
 *
 * Returns the original address, or 0.  Zero is safe as failure because
 * a map's first unit is never handed out. */
uiox_uint32_t uiox_kix_map_malloc(uiox_kix_res_map_t *map, uiox_uint32_t units)
{
    int i;

    if (!map || units == 0u) return 0u;

    for (i = 0; i < map->rm_nentries; i++) {
        uiox_kix_map_entry_t *e = &map->rm_entries[i];
        uiox_uint32_t         addr;

        if (!e->me_valid) continue;
        if (e->me_size < units) continue;

        addr = e->me_addr;

        if (e->me_size == units) {
            /* Exact fit — the entry disappears. */
            e->me_valid = 0;
            e->me_size  = 0u;
            e->me_addr  = 0u;
        } else {
            /* Partial — the extent moves forward past what was taken. */
            e->me_addr += units;
            e->me_size -= units;
        }
        return addr;
    }
    return 0u;
}

/* ── uiox_kix_map_free ───────────────────────────────────────────────
 * Return units to a map, then MERGE with neighbours.
 *
 * The merge is what makes the map usable over time.  Without it a
 * sequence of swap-out/swap-in cycles leaves a map full of small
 * extents, and a process needing a large contiguous run cannot be
 * swapped out even though the total free space is ample — which is
 * exactly the fragmentation Bach says the swap device cannot afford.
 *
 * Two passes, left then right, and each is a full scan: a released
 * extent may touch either side, and merging is not order-dependent
 * because a merged extent is re-examined through the updated slot
 * variables. */
void uiox_kix_map_free(uiox_kix_res_map_t *map, uiox_uint32_t addr,
                       uiox_uint32_t units)
{
    int i;
    int slot = -1;

    if (!map || units == 0u) return;

    /* Prefer a dead slot; only grow the array if none is free. */
    for (i = 0; i < map->rm_nentries; i++) {
        if (!map->rm_entries[i].me_valid) { slot = i; break; }
    }
    if (slot < 0 && map->rm_nentries < UIOX_KIX_MAP_MAX_ENTRIES)
        slot = map->rm_nentries++;

    if (slot < 0) return;    /* map full — the extent is dropped */

    map->rm_entries[slot].me_valid = 1;
    map->rm_entries[slot].me_addr  = addr;
    map->rm_entries[slot].me_size  = units;

    /* Merge with a neighbour on the LEFT (one that ends where we start). */
    for (i = 0; i < map->rm_nentries; i++) {
        uiox_kix_map_entry_t *e = &map->rm_entries[i];

        if (!e->me_valid || i == slot) continue;
        if (e->me_addr + e->me_size == addr) {
            e->me_size += units;
            map->rm_entries[slot].me_valid = 0;
            slot  = i;
            addr  = e->me_addr;
            units = e->me_size;
        }
    }

    /* Merge with a neighbour on the RIGHT (one that starts where we end). */
    for (i = 0; i < map->rm_nentries; i++) {
        uiox_kix_map_entry_t *e = &map->rm_entries[i];

        if (!e->me_valid || i == slot) continue;
        if (addr + units == e->me_addr) {
            map->rm_entries[slot].me_size += e->me_size;
            e->me_valid = 0;
            units = map->rm_entries[slot].me_size;
        }
    }
}

/* ── uiox_kix_map_init ───────────────────────────────────────────────
 * Seed a map with one extent covering the whole resource. */
void uiox_kix_map_init(uiox_kix_res_map_t *map, const char *name,
                       uiox_uint32_t start, uiox_uint32_t total_units)
{
    int i;

    if (!map) return;

    for (i = 0; i < UIOX_KIX_MAP_MAX_ENTRIES; i++) {
        map->rm_entries[i].me_valid = 0;
        map->rm_entries[i].me_addr  = 0u;
        map->rm_entries[i].me_size  = 0u;
    }

    map->rm_name                    = name;
    map->rm_entries[0].me_valid     = 1;
    map->rm_entries[0].me_addr      = start;
    map->rm_entries[0].me_size      = total_units;
    map->rm_nentries                = 1;
}

/* ── uiox_kix_enough_memory_for ──────────────────────────────────────
 * Bach's "if (enough room in main memory for process)".
 *
 * Compares the process's page count against the free frame count.  Note
 * p_size is a count of PAGES, not of bytes — the shared header says so,
 * and a caller passing bytes would get a wrong answer rather than a
 * compile error. */
int uiox_kix_enough_memory_for(uiox_kix_psa_proc_t *p)
{
    if (!p) return 0;
    return uiox_kix_phys_mem.pm_free_pages >= (int)p->p_size;
}

/* ── uiox_kix_swap_out_process ───────────────────────────────────────
 * Write a process out to the swap device.
 *
 * Claims CONTIGUOUS blocks through the map — Bach: "the kernel allocates
 * contiguous space on the swap device without regard for fragmentation"
 * — marks each in the per-block truth array, frees the frames it held,
 * and moves the state.
 *
 * The blocks are marked used AND the count adjusted because the two
 * views must agree: sd_used says which blocks, sd_free_blocks says how
 * many.  A mismatch between them is the failure to look for first.
 *
 * Returns 0, or -1 when the map cannot supply a contiguous run. */
int uiox_kix_swap_out_process(uiox_kix_psa_proc_t *p)
{
    uiox_uint32_t i, blk;

    if (!p) return -1;

    blk = uiox_kix_map_malloc(&uiox_kix_swap_map, p->p_size);
    if (blk == 0u) return -1;          /* no contiguous run available */

    /* RECORD it.  Without this the map is the only thing that knows the
     * extent, and swap-in cannot ask for it back — which is how the map
     * came to empty while the device still reported free space. */
    p->p_swap_blk = blk;

    for (i = blk; i < blk + p->p_size && i < UIOX_KIX_SWAP_BLOCKS; i++)
        uiox_kix_swap_device.sd_used[i] = 1u;

    uiox_kix_swap_device.sd_free_blocks -= p->p_size;
    uiox_kix_phys_mem.pm_free_pages     += (int)p->p_size;

    for (i = 0; i < p->p_size && i < (uiox_uint32_t)UIOX_PHYS_PAGES; i++)
        uiox_kix_phys_mem.pm_used[i] = 0u;

    /* Bach's two state moves out:
     *   READY -> READY_SWAPPED  (ready, but not resident)
     *   SLEEP -> SLEEP_SWAPPED  (asleep, and not resident)
     * Both go through 40_psa's transition table, so an invalid move is
     * refused rather than forced. */
    if (p->p_state == UIOX_KIX_PSA_PROC_READY)
        (void)uiox_kix_psa_proc_set_state(p, UIOX_KIX_PSA_PROC_READY_SWAPPED);
    else if (p->p_state == UIOX_KIX_PSA_PROC_SLEEP_MEM)
        (void)uiox_kix_psa_proc_set_state(p, UIOX_KIX_PSA_PROC_SLEEP_SWAPPED);

    p->p_swap_time = uiox_kix_clock_ticks;
    p->p_flag     &= ~UIOX_KIX_PSA_P_LOADED;

    return 0;
}

/* ── uiox_kix_swap_in_process ────────────────────────────────────────
 * Read a process back in: claim frames, release its swap blocks, and
 * move it to the matching resident state.
 *
 * The frames are taken from the low end of pm_used, which is the same
 * order uiox_phys_alloc.c hands them out — so the two allocators agree
 * on which frame is "next" rather than racing for it.
 *
 * Returns 0, or -1 when memory is short. */
int uiox_kix_swap_in_process(uiox_kix_psa_proc_t *p)
{
    int           i;
    int           allocated = 0;

    if (!p) return -1;
    if (!uiox_kix_enough_memory_for(p)) return -1;

    for (i = 0; i < (int)UIOX_PHYS_PAGES && allocated < (int)p->p_size; i++) {
        if (!uiox_kix_phys_mem.pm_used[i]) {
            uiox_kix_phys_mem.pm_used[i] = 1u;
            allocated++;
        }
    }

    /* Return the blocks to the map AND clear the per-block truth.
     * Both, because the two views must agree — a map that believes a run
     * is free while sd_used still marks it is the mismatch this file's
     * banner says to look for first. */
    if (p->p_swap_blk != 0u) {
        uiox_uint32_t b;

        for (b = p->p_swap_blk;
             b < p->p_swap_blk + p->p_size && b < UIOX_KIX_SWAP_BLOCKS; b++)
            uiox_kix_swap_device.sd_used[b] = 0u;

        uiox_kix_map_free(&uiox_kix_swap_map, p->p_swap_blk, p->p_size);
        p->p_swap_blk = 0u;      /* no longer on the device */
    }

    uiox_kix_phys_mem.pm_free_pages     -= (int)p->p_size;
    uiox_kix_swap_device.sd_free_blocks += p->p_size;

    if (p->p_state == UIOX_KIX_PSA_PROC_READY_SWAPPED)
        (void)uiox_kix_psa_proc_set_state(p, UIOX_KIX_PSA_PROC_READY);
    else if (p->p_state == UIOX_KIX_PSA_PROC_SLEEP_SWAPPED)
        (void)uiox_kix_psa_proc_set_state(p, UIOX_KIX_PSA_PROC_SLEEP_MEM);

    p->p_flag |= UIOX_KIX_PSA_P_LOADED;

    return 0;
}

/* ── uiox_kix_pick_longest_swapped_out ───────────────────────────────
 * Bach: "for (all swapped out processes that are ready to run) pick
 * process swapped out longest".
 *
 * READY_SWAPPED only — a process that is ASLEEP and swapped out is not
 * a swap-in candidate, however long it has been out.  Bringing it in
 * would occupy memory for a process that cannot run. */
uiox_kix_psa_proc_t *uiox_kix_pick_longest_swapped_out(void)
{
    int                  i;
    uiox_kix_psa_proc_t *best = (uiox_kix_psa_proc_t *)0;

    for (i = 0; i < UIOX_KIX_PSA_NPROC; i++) {
        uiox_kix_psa_proc_t *p = &uiox_kix_psa_proc_table[i];

        if (p->p_state != UIOX_KIX_PSA_PROC_READY_SWAPPED) continue;

        /* OLDEST swap time wins — a smaller tick value. */
        if (!best || p->p_swap_time < best->p_swap_time) best = p;
    }
    return best;
}

/* ── uiox_kix_pick_swap_out_candidate ────────────────────────────────
 * The process to evict, scored by BACH'S TWO RULES — and the choice
 * between them is a property of the SYSTEM, not of the candidate:
 *
 *   if (there is a sleeping process)
 *       choose process such that priority + residence time is highest
 *   else
 *       choose process such that residence time + nice is highest
 *
 * So the first pass asks whether ANY process is sleeping, and only then
 * does it score.  A reader implementing from the scoring alone would
 * pick the wrong rule roughly half the time — which is why the test is
 * separated here and commented.
 *
 * Two exclusions, both from Bach: "not zombie and not locked in memory".
 * A zombie has no image left to write out; a locked process must stay.
 *
 * Returns NULL when nothing is eligible. */
uiox_kix_psa_proc_t *uiox_kix_pick_swap_out_candidate(void)
{
    int                  i;
    int                  has_sleeping = 0;
    int                  best_score   = -1;
    uiox_kix_psa_proc_t *best         = (uiox_kix_psa_proc_t *)0;

    /* First pass: is anything asleep?  Decides which scoring rule. */
    for (i = 0; i < UIOX_KIX_PSA_NPROC; i++) {
        uiox_kix_psa_proc_t *p = &uiox_kix_psa_proc_table[i];

        if (p->p_state == UIOX_KIX_PSA_PROC_SLEEP_MEM &&
            (p->p_flag & UIOX_KIX_PSA_P_LOADED))
            has_sleeping = 1;
    }

    /* Second pass: score the eligible ones. */
    for (i = 0; i < UIOX_KIX_PSA_NPROC; i++) {
        uiox_kix_psa_proc_t *p = &uiox_kix_psa_proc_table[i];
        int                  score;

        if (p->p_state == UIOX_KIX_PSA_PROC_UNUSED)  continue;
        if (p->p_state == UIOX_KIX_PSA_PROC_ZOMBIE)  continue;
        if (!(p->p_flag & UIOX_KIX_PSA_P_LOADED))    continue;  /* not resident */
        if (p->p_flag & UIOX_KIX_PSA_P_STICKY)       continue;  /* locked in    */

        if (has_sleeping && p->p_state == UIOX_KIX_PSA_PROC_SLEEP_MEM) {
            /* Rule 1: priority + residence.  Smaller p_pri is a HIGHER
             * priority, so the numerics are INVERTED here — adding a
             * larger priority must LOWER the score, not raise it. */
            score = (int)p->p_sched.p_time - (int)p->p_sched.p_pri;
        } else if (!has_sleeping) {
            /* Rule 2: residence + nice.  Same inversion for nice. */
            score = (int)p->p_sched.p_time + (int)p->p_sched.p_nice;
        } else {
            continue;   /* asleep case did not match — not a candidate */
        }

        if (score > best_score) { best_score = score; best = p; }
    }
    return best;
}

/* ── uiox_kix_wakeup_swapper ─────────────────────────────────────────
 * Bach's kernel wakes the swapper when memory runs short.
 *
 * ── why this CALLS the swapper rather than only clearing a flag ─────
 * Bach's swapper sleeps and is woken; this one RETURNS, because a kernel
 * with no real sleep primitive cannot block the interrupt that would
 * otherwise be doing the waking.  So re-entry is the caller's job, and
 * this function does it:
 *
 *     clear the flag, then RUN the swapper
 *
 * Clearing alone would set the flag to zero and change nothing else —
 * a wakeup that wakes nothing.  That is the difference between this
 * function and the version it replaces.
 *
 * The guard is the memory condition the wakeup is FOR: waking the
 * swapper when memory is plentiful would swap a process out for no
 * reason. */
void uiox_kix_wakeup_swapper(void)
{
    if (!uiox_kix_swapper_sleep) return;
    if (uiox_kix_phys_mem.pm_free_pages >= ((int)UIOX_PHYS_PAGES / 4))
        return;                       /* still room — nothing to do */

    uiox_kix_swapper_sleep = 0;
    uiox_kix_swapper();               /* the actual wakeup */
}

/* ── Algorithm 2 — uiox_kix_swapper ──────────────────────────────────
 * Bach's loop, with this build's difference stated: where the algorithm
 * says "sleep (event must swap in)", this RETURNS.
 *
 * Returning rather than spinning is deliberate — a loop here would
 * occupy the CPU that the next interrupt needs, and on a uniprocessor
 * that is a hang.  The cost is that re-entry must be arranged, which
 * uiox_kix_wakeup_swapper() does.
 *
 * The iteration order is Bach's:
 *   1. a swapped-out READY process, oldest first
 *   2. if memory fits it, swap it in and go round again
 *   3. otherwise find a victim to evict
 *   4. if no victim, or the victim has not been resident long enough,
 *      stop — swapping out a process that arrived recently is thrashing
 */
void uiox_kix_swapper(void)
{
    uiox_kix_psa_proc_t *swap_in_proc;
    uiox_kix_psa_proc_t *victim;

    for (;;) {
        swap_in_proc = uiox_kix_pick_longest_swapped_out();

        if (!swap_in_proc) {
            /* Nothing to bring in.  Bach sleeps on "must swap in". */
            uiox_kix_swapper_sleep = 1;
            return;
        }

        if (uiox_kix_enough_memory_for(swap_in_proc)) {
            if (uiox_kix_swap_in_process(swap_in_proc) == 0) {
                /* Resident again and runnable: hand it to the scheduler.
                 * 40_psa's enqueue is the queue the schduler walks —
                 * there is no second run queue to add it to. */
                uiox_kix_psa_sched_enqueue(swap_in_proc);
            }
            continue;
        }

        victim = uiox_kix_pick_swap_out_candidate();

        if (!victim) {
            uiox_kix_swapper_sleep = 1;
            return;
        }

        /* Residency guard: a process that has not been in long enough is
         * not evicted, however good its score.  Without this the swapper
         * can evict the process it just brought in. */
        if (victim->p_sched.p_time < UIOX_KIX_MIN_RESIDENCE) {
            uiox_kix_swapper_sleep = 1;
            return;
        }

        (void)uiox_kix_swap_out_process(victim);
    }
}
