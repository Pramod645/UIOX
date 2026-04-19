#include "../include/swapper.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* ── Globals ────────────────────────────────────────────────── */
swap_device_t swap_device;
phys_mem_t    phys_mem;
res_map_t     swap_map;
int           swapper_sleep = 0;

/* ─────────────────────────────────────────────────────────────
 * 1. Algorithm malloc (map_malloc)
 *    input : map pointer, number of units requested
 *    output: base address if successful, 0 otherwise
 *
 *    First-fit allocation on a resource map.
 */
uint32_t map_malloc(res_map_t *map, uint32_t units)
{
    if (!map || units == 0) return 0;

    for (int i = 0; i < map->rm_nentries; i++) {
        map_entry_t *e = &map->rm_entries[i];
        if (!e->me_valid) continue;

        if (e->me_size >= units) {
            uint32_t addr = e->me_addr;

            if (e->me_size == units) {
                /* Exact fit — delete entry from map */
                e->me_valid = 0;
                e->me_size  = 0;
                e->me_addr  = 0;
            } else {
                /* Partial fit — adjust start address of entry */
                e->me_addr += units;
                e->me_size -= units;
            }

            printf("[malloc] map='%s' allocated %u units "
                   "at addr=%u\n",
                   map->rm_name, units, addr);
            return addr;
        }
    }

    printf("[malloc] map='%s' no space for %u units\n",
           map->rm_name, units);
    return 0;   /* allocation failed */
}

/* ── map_free ────────────────────────────────────────────────
 * Return units at addr back to the map.
 * Merges with adjacent free entries to reduce fragmentation.
 */
void map_free(res_map_t *map, uint32_t addr, uint32_t units)
{
    if (!map || units == 0) return;

    /* Find a free slot */
    int slot = -1;
    for (int i = 0; i < map->rm_nentries; i++) {
        if (!map->rm_entries[i].me_valid) {
            slot = i; break;
        }
    }
    if (slot < 0 && map->rm_nentries < MAP_MAX_ENTRIES)
        slot = map->rm_nentries++;

    if (slot < 0) {
        fprintf(stderr, "[map_free] map '%s' full\n",
                map->rm_name);
        return;
    }

    map->rm_entries[slot].me_valid = 1;
    map->rm_entries[slot].me_addr  = addr;
    map->rm_entries[slot].me_size  = units;

    /* Merge with left neighbour */
    for (int i = 0; i < map->rm_nentries; i++) {
        map_entry_t *e = &map->rm_entries[i];
        if (!e->me_valid || i == slot) continue;
        if (e->me_addr + e->me_size == addr) {
            e->me_size += units;
            map->rm_entries[slot].me_valid = 0;
            slot = i; addr = e->me_addr; units = e->me_size;
        }
    }

    /* Merge with right neighbour */
    for (int i = 0; i < map->rm_nentries; i++) {
        map_entry_t *e = &map->rm_entries[i];
        if (!e->me_valid || i == slot) continue;
        if (addr + units == e->me_addr) {
            map->rm_entries[slot].me_size += e->me_size;
            e->me_valid = 0;
        }
    }

    printf("[map_free] map='%s' freed %u units at %u\n",
           map->rm_name, units, addr);
}

/* ── map_init ────────────────────────────────────────────────
 * Initialise a resource map with one large free block.
 */
void map_init(res_map_t *map, const char *name,
              uint32_t start, uint32_t total_units)
{
    if (!map) return;
    memset(map, 0, sizeof(res_map_t));
    map->rm_name               = name;
    map->rm_entries[0].me_valid = 1;
    map->rm_entries[0].me_addr  = start;
    map->rm_entries[0].me_size  = total_units;
    map->rm_nentries            = 1;
    printf("[map_init] '%s' start=%u units=%u\n",
           name, start, total_units);
}

/* ── enough_memory_for ───────────────────────────────────────
 * Return 1 if enough free physical pages exist for process p.
 */
int enough_memory_for(proc_entry_t *p)
{
    return phys_mem.pm_free_pages >= (int)p->pe_size;
}

/* ── swap_out_process ────────────────────────────────────────
 * Write process image to swap device and mark it swapped.
 */
int swap_out_process(proc_entry_t *p)
{
    if (!p) return -1;

    uint32_t blk = map_malloc(&swap_map, p->pe_size);
    if (!blk) {
        fprintf(stderr, "[swapper] no swap space for pid=%u\n",
                p->pe_pid);
        return -1;
    }

    /* Simulate writing pages to swap device */
    for (uint32_t i = blk; i < blk + p->pe_size &&
                           i < SWAP_DEVICE_BLOCKS; i++)
        swap_device.sd_used[i] = 1;

    swap_device.sd_free_blocks -= p->pe_size;
    phys_mem.pm_free_pages     += (int)p->pe_size;

    /* Mark physical pages free */
    for (uint32_t i = 0; i < p->pe_size &&
                         i < (uint32_t)PHYS_PAGES; i++)
        phys_mem.pm_used[i] = 0;

    if (p->pe_state == SCHED_READY)
        p->pe_state = SCHED_READY_SWAP;
    else if (p->pe_state == SCHED_SLEEP)
        p->pe_state = SCHED_SLEEP_SWAP;

    p->pe_in_memory  = 0;
    p->pe_swap_time  = (uint32_t)clock_ticks;
    p->pe_locked     = 0;

    printf("[swapper] swapped OUT pid=%u size=%u "
           "blk=%u\n", p->pe_pid, p->pe_size, blk);
    return 0;
}

/* ── swap_in_process ─────────────────────────────────────────
 * Read process image back from swap device into memory.
 */
int swap_in_process(proc_entry_t *p)
{
    if (!p) return -1;
    if (!enough_memory_for(p)) return -1;

    /* Allocate physical pages */
    int allocated = 0;
    for (int i = 0; i < PHYS_PAGES &&
                    (uint32_t)allocated < p->pe_size; i++) {
        if (!phys_mem.pm_used[i]) {
            phys_mem.pm_used[i] = 1;
            allocated++;
        }
    }

    phys_mem.pm_free_pages -= (int)p->pe_size;

    /* Free swap blocks */
    /* map_free(&swap_map, p->swap_blk, p->pe_size); */
    swap_device.sd_free_blocks += p->pe_size;

    if (p->pe_state == SCHED_READY_SWAP)
        p->pe_state = SCHED_READY;
    else if (p->pe_state == SCHED_SLEEP_SWAP)
        p->pe_state = SCHED_SLEEP;

    p->pe_in_memory = 1;

    printf("[swapper] swapped IN  pid=%u size=%u\n",
           p->pe_pid, p->pe_size);
    return 0;
}

/* ── pick_longest_swapped_out ────────────────────────────────
 * Find the SCHED_READY_SWAP process that was swapped out
 * the longest ago.
 */
proc_entry_t *pick_longest_swapped_out(void)
{
    proc_entry_t *best = NULL;
    for (int i = 0; i < NPROC; i++) {
        proc_entry_t *p = &proc_table[i];
        if (p->pe_state != SCHED_READY_SWAP) continue;
        if (!best || p->pe_swap_time < best->pe_swap_time)
            best = p;
    }
    return best;
}

/* ── pick_swap_out_candidate ─────────────────────────────────
 * From in-memory, non-zombie, non-locked processes choose the
 * best candidate to swap out.
 *
 * Sleeping process: maximise (priority + residence_time)
 * Ready process:    maximise (residence_time + nice)
 */
proc_entry_t *pick_swap_out_candidate(void)
{
    proc_entry_t *best  = NULL;
    int           best_score = -1;
    int           has_sleeping = 0;

    /* Check if any sleeping process is in memory */
    for (int i = 0; i < NPROC; i++) {
        if (proc_table[i].pe_state == SCHED_SLEEP &&
            proc_table[i].pe_in_memory)
            has_sleeping = 1;
    }

    for (int i = 0; i < NPROC; i++) {
        proc_entry_t *p = &proc_table[i];

        if (p->pe_state == SCHED_UNUSED ||
            p->pe_state == SCHED_ZOMBIE) continue;
        if (!p->pe_in_memory) continue;
        if (p->pe_locked)     continue;

        int score;
        if (has_sleeping && p->pe_state == SCHED_SLEEP) {
            /* Priority + residence time (higher = better victim) */
            score = p->pe_sched.sp_priority +
                    (int)p->pe_sched.sp_residence;
        } else if (!has_sleeping) {
            /* Residence time + nice */
            score = (int)p->pe_sched.sp_residence +
                    p->pe_sched.sp_nice;
        } else
            continue;

        if (score > best_score) {
            best_score = score;
            best       = p;
        }
    }
    return best;
}

/* ── wakeup_swapper ──────────────────────────────────────────
 * Called by the clock interrupt when memory pressure warrants.
 */
void wakeup_swapper(void)
{
    if (swapper_sleep &&
        phys_mem.pm_free_pages < (PHYS_PAGES / 4)) {
        swapper_sleep = 0;
        printf("[swapper] woken by memory pressure "
               "free_pages=%d\n", phys_mem.pm_free_pages);
    }
}

/* ─────────────────────────────────────────────────────────────
 * 2. Algorithm swapper
 *    input : none
 *    output: none
 *
 *    Runs as process 0.  Swaps out processes to make room,
 *    and swaps in ready processes when memory is available.
 */
void swapper(void)
{
    printf("[swapper] swapper process started\n");

    for (;;) {   /* outer loop */

        /* ── Find the swapped-out process ready longest ────── */
        proc_entry_t *swap_in_proc = pick_longest_swapped_out();

        if (!swap_in_proc) {
            /* No process needs swapping in — sleep */
            printf("[swapper] no swap-in candidate, sleeping\n");
            swapper_sleep = 1;
            /* proc_sleep(SWAP_IN_EVENT, PSWP, 0); */
            /* Simulated: just return until woken               */
            return;
        }

        /* ── Enough room in main memory? ─────────────────── */
        if (enough_memory_for(swap_in_proc)) {
            swap_in_process(swap_in_proc);
            run_queue_add(swap_in_proc);
            continue;   /* goto loop */
        }

        /* ── Not enough room — choose a process to swap out ─ */
        proc_entry_t *victim = pick_swap_out_candidate();

        if (!victim ||
            victim->pe_state != SCHED_SLEEP ||
            victim->pe_sched.sp_residence < MIN_RESIDENCE) {
            /* Chosen process not sleeping or residency not met */
            printf("[swapper] no suitable swap-out "
                   "candidate, sleeping\n");
            swapper_sleep = 1;
            /* proc_sleep(SWAP_IN_EVENT, PSWP, 0); */
            return;
        }

        /* ── Swap out the chosen process ─────────────────── */
        swap_out_process(victim);

        /* Loop back and try again */
    }
}
