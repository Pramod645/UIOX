/*
 * 30_KIX/33_PCS/02_MemMngnt/include/uiox_kix_swap.h
 *
 * Swapping — the resource map, the swap device, and the swapper's view
 * of main memory.
 *
 * ── this header replaces swapper.h ──────────────────────────────────
 * swapper.h carried two things that are not this layer's to define:
 *
 *   1. proc_entry_t and sched_state_t — a RIVAL process entry and a
 *      rival eight-value state enum, from a second scheduler.  Both are
 *      gone.  Nothing here names a process; swapper.c binds to 40_psa's
 *      uiox_kix_psa_proc_t directly.
 *
 *   2. PHYS_PAGES and TIME_QUANTUM — page geometry now lives in
 *      uiox_kix_pagemap.h, and the quantum in 01_schedular.
 *
 * What is left is what only this subsystem needs.
 *
 * ── Algorithm 1: malloc, the resource map ───────────────────────────
 * Bach: "The kernel maintains free space for file systems in a linked
 * list of free blocks ... but it maintains the free space for the swap
 * device in an in-core table, called a map."
 *
 * A map is an array of (address, size) extents, and map_malloc does a
 * FIRST-FIT walk over them.  The contrast with the file system is worth
 * stating, because it explains why the map is not a free list:
 *
 *   file space is allocated STATICALLY — it lives for a long time, so
 *   the scheme tolerates fragmentation to keep reusable holes
 *
 *   swap space is allocated TRANSITORILY — "depending on the pattern of
 *   process scheduling", a swapped process comes back and frees what it
 *   held.  Speed is critical and one multiblock I/O beats several single
 *   block ones, so the kernel takes CONTIGUOUS space "without regard for
 *   fragmentation"
 *
 * That is why a map holds extents rather than individual blocks, and why
 * map_free merges neighbours: an allocation must be able to hand back one
 * contiguous run.
 *
 * ── what is NOT implemented here ────────────────────────────────────
 * There is no actual I/O.  swap_out_process marks blocks used and frees
 * the frames; swap_in_process reverses it.  A real kernel writes the
 * process image to the device between those two points.  The bookkeeping
 * is real, the transfer is not — and that is stated so nobody assumes a
 * swapped process's contents survived.
 *
 * @version 1.0.0  @date 2026-09-29
 */
#ifndef UIOX_KIX_SWAP_H
#define UIOX_KIX_SWAP_H

#include "uiox_klibc.h"
#include "uiox_kix_pagemap.h"   /* UIOX_PHYS_PAGES, UIOX_PAGE_SIZE */
#include "uiox_kix_psa_process.h"  /* uiox_kix_psa_proc_t */

/* ── Resource map ────────────────────────────────────────────────────
 * MAP_MAX_ENTRIES bounds the EXTENTS a map can describe, not the units
 * it can allocate.  A map with one entry spanning 8192 blocks is as
 * capable as one with 512 fragments, and cheaper to search — which is
 * why map_free merges. */
#define UIOX_KIX_MAP_MAX_ENTRIES 512

typedef struct uiox_kix_map_entry {
    uint32_t me_addr;    /* start address / first block number      */
    uint32_t me_size;    /* contiguous units from me_addr           */
    int      me_valid;   /* 1 = this entry describes free space     */
} uiox_kix_map_entry_t;

typedef struct uiox_kix_res_map {
    uiox_kix_map_entry_t rm_entries[UIOX_KIX_MAP_MAX_ENTRIES];
    int                  rm_nentries;   /* highest index in use       */
    const char          *rm_name;       /* for a diagnostic           */
} uiox_kix_res_map_t;

/* ── Swap device ─────────────────────────────────────────────────────
 * A block device in a configurable section of a disk.  Blocks are
 * tracked two ways on purpose:
 *
 *   sd_used[]  the per-block truth, so a leak is visible
 *   sd_map     the extents, so an allocation can be contiguous
 *
 * The two must agree; a mismatch means one of them was updated without
 * the other, which is the failure to look for first. */
#define UIOX_KIX_SWAP_BLOCKS 8192

typedef struct uiox_kix_swap_device {
    uint8_t             sd_used[UIOX_KIX_SWAP_BLOCKS];  /* 1 = in use   */
    uint32_t            sd_free_blocks;
    uiox_kix_res_map_t  sd_map;
} uiox_kix_swap_device_t;

/* ── Main memory, as the swapper sees it ─────────────────────────────
 * The swapper counts FRAMES, not bytes and not processes.  pm_used is
 * the per-frame truth beneath pm_free_pages, so a bookkeeping error
 * shows as a count that disagrees with the bits.
 *
 * UIOX_PHYS_PAGES comes from uiox_kix_pagemap.h — the same number the
 * fault handler sizes its table with.  An earlier version defined it
 * here as well as there, which is how the two came to disagree. */
typedef struct uiox_kix_phys_mem {
    int      pm_free_pages;
    uint8_t  pm_used[UIOX_PHYS_PAGES];   /* 1 = frame assigned          */
} uiox_kix_phys_mem_t;

/* ── Swapper constants ───────────────────────────────────────────────
 * MIN_RESIDENCE is the guard against thrashing: a process that arrived
 * recently is not a swap-out candidate however low its priority scores.
 * Without it the swapper can pick the process it just brought in. */
#define UIOX_KIX_MIN_RESIDENCE  5   /* ticks resident before eligible */

/* ── Globals ─────────────────────────────────────────────────────────
 * Defined in swapper.c. */
extern uiox_kix_swap_device_t uiox_kix_swap_device;
extern uiox_kix_phys_mem_t    uiox_kix_phys_mem;
extern uiox_kix_res_map_t     uiox_kix_swap_map;
extern int                    uiox_kix_swapper_sleep;  /* 1 = sleeping */

/* ── Algorithm 1: malloc — first-fit on a resource map ───────────────
 * Bach, verbatim:
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
 * Returns the starting unit, or 0 for failure.  Zero is safe as a
 * failure code because unit 0 of a swap device is never handed out —
 * the map starts at a non-zero address. */
uint32_t uiox_kix_map_malloc(uiox_kix_res_map_t *map, uint32_t units);

/* Return units to a map, merging with any neighbour they touch.  The
 * merge is what keeps the map from fragmenting into unusable slivers
 * after a sequence of swap-in/swap-out cycles. */
void uiox_kix_map_free(uiox_kix_res_map_t *map, uint32_t addr,
                       uint32_t units);

/* Seed a map with one extent spanning the whole resource. */
void uiox_kix_map_init(uiox_kix_res_map_t *map, const char *name,
                       uint32_t start, uint32_t total_units);

/* ── Swap I/O ────────────────────────────────────────────────────────
 * Both operate on a 40_psa process entry — the ONE process type.  A
 * rival proc_entry_t used to be named here; it is gone. */

/* Is there room in main memory for this process's image?  Reads the
 * process's page count against the swapper's free-frame count. */
int uiox_kix_enough_memory_for(uiox_kix_psa_proc_t *p);

/* Write a process out.  Claims contiguous swap blocks through the map,
 * marks them used, frees the frames it occupied, and moves the process
 * to a swapped state.
 * Returns 0 on success, -1 when there is no room on the device. */
int uiox_kix_swap_out_process(uiox_kix_psa_proc_t *p);

/* Read a process back in: claims frames, releases its swap blocks, and
 * moves it to the matching resident state.
 * Returns 0 on success, -1 when there is no room in memory. */
int uiox_kix_swap_in_process(uiox_kix_psa_proc_t *p);

/* ── Algorithm 2: swapper ────────────────────────────────────────────
 * Bach:
 *   loop:
 *       for (all swapped out processes that are ready to run)
 *           pick process swapped out longest;
 *       if (no such process) { sleep (event must swap in); goto loop; }
 *       if (enough room in main memory for process)
 *       { swap process in; goto loop; }
 *       for (all processes loaded in main memory, not zombie and not
 *            locked in memory)
 *       {
 *           if (there is a sleeping process)
 *               choose process such that priority + residence time is
 *                   numerically highest;
 *           else
 *               choose process such that residence time + nice is
 *                   numerically highest;
 *       }
 *       if (chosen process not sleeping or residency not satisfied)
 *           sleep (event must swap process in);
 *       else
 *           swap out process;
 *       goto loop;
 *
 * NOTE the two scoring rules are DIFFERENT, and the choice between them
 * is a property of the whole system, not of the candidate: if ANY
 * process is sleeping, every candidate is scored by priority+residence;
 * otherwise by residence+nice.  A reader implementing this from the
 * pick_* helpers alone would miss that. */
void uiox_kix_swapper(void);

/* ── Candidate selection ─────────────────────────────────────────────
 * Split out from the algorithm so each is testable on its own — Bach's
 * two scoring rules are the part most likely to be got wrong. */

/* The swapped-out READY process that has been out longest.  Bach:
 * "pick process swapped out longest" — oldest swap time wins. */
uiox_kix_psa_proc_t *uiox_kix_pick_longest_swapped_out(void);

/* The best swap-out victim, scored by Bach's two rules.  Returns NULL
 * when nothing is eligible — every candidate zombie, locked, or absent
 * from memory. */
uiox_kix_psa_proc_t *uiox_kix_pick_swap_out_candidate(void);

/* ── Waking the swapper ──────────────────────────────────────────────
 * Bach's algorithm SLEEPS on "event must swap in" and the kernel wakes it
 * when memory becomes short.  This build's swapper RETURNS instead of
 * sleeping, so the wakeup must re-enter it — otherwise the flag is
 * cleared and nothing runs, which is a stall rather than a wakeup.
 *
 * The re-entry is the caller's to arrange; see swapper.c for why the
 * call is not made here. */
void uiox_kix_wakeup_swapper(void);

#endif /* UIOX_KIX_SWAP_H */
