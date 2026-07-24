/*
 * 30_KIX/33_PCS/02_MemMngnt/src/swapper.c
 *
 * Freestanding fixes (v2.0)
 * ─────────────────────────
 *   REMOVED: implicit <stdio.h> dependency
 *            (swapper.h already includes uiox_klibc.h which provides printf)
 *
 *   FIXED: fprintf(stderr, "[map_free] map '%s' full\n", ...)
 *       → printf("[map_free] ERROR: map '%s' full\n", ...)
 *
 *   FIXED: fprintf(stderr, "[swapper] no swap space for pid=%u\n", ...)
 *       → printf("[swapper] ERROR: no swap space for pid=%u\n", ...)
 *
 *   FIXED: for (int i ...) loop variable declarations moved before loop
 *          for strict C11 freestanding compliance.
 *
 * No algorithm changes.
 *
 * @version 2.0.0  @date 2026-07-23
 */

 #include "../include/swapper.h"

 /* ── Globals ────────────────────────────────────────────────── */
 swap_device_t swap_device;
 phys_mem_t    phys_mem;
 res_map_t     swap_map;
 int           swapper_sleep = 0;
 
 /* ── Algorithm malloc (map_malloc) ──────────────────────────── */
 uint32_t map_malloc(res_map_t *map, uint32_t units)
 {
     int i;
     if (!map || units == 0) return 0;
 
     for (i = 0; i < map->rm_nentries; i++) {
         map_entry_t *e = &map->rm_entries[i];
         if (!e->me_valid) continue;
 
         if (e->me_size >= units) {
             uint32_t addr = e->me_addr;
 
             if (e->me_size == units) {
                 e->me_valid = 0;
                 e->me_size  = 0;
                 e->me_addr  = 0;
             } else {
                 e->me_addr += units;
                 e->me_size -= units;
             }
 
             printf("[malloc] map='%s' allocated %u units at addr=%u\n",
                    map->rm_name, units, addr);
             return addr;
         }
     }
 
     printf("[malloc] map='%s' no space for %u units\n",
            map->rm_name, units);
     return 0;
 }
 
 /* ── map_free ────────────────────────────────────────────────── */
 void map_free(res_map_t *map, uint32_t addr, uint32_t units)
 {
     int i, slot = -1;
     if (!map || units == 0) return;
 
     for (i = 0; i < map->rm_nentries; i++) {
         if (!map->rm_entries[i].me_valid) { slot = i; break; }
     }
     if (slot < 0 && map->rm_nentries < MAP_MAX_ENTRIES)
         slot = map->rm_nentries++;
 
     if (slot < 0) {
         printf("[map_free] ERROR: map '%s' full\n",  /* was: fprintf(stderr,...) */
                map->rm_name);
         return;
     }
 
     map->rm_entries[slot].me_valid = 1;
     map->rm_entries[slot].me_addr  = addr;
     map->rm_entries[slot].me_size  = units;
 
     /* Merge with left neighbour */
     for (i = 0; i < map->rm_nentries; i++) {
         map_entry_t *e = &map->rm_entries[i];
         if (!e->me_valid || i == slot) continue;
         if (e->me_addr + e->me_size == addr) {
             e->me_size += units;
             map->rm_entries[slot].me_valid = 0;
             slot = i; addr = e->me_addr; units = e->me_size;
         }
     }
 
     /* Merge with right neighbour */
     for (i = 0; i < map->rm_nentries; i++) {
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
 
 /* ── map_init ────────────────────────────────────────────────── */
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
 
 /* ── enough_memory_for ───────────────────────────────────────── */
 int enough_memory_for(proc_entry_t *p)
 {
     return phys_mem.pm_free_pages >= (int)p->pe_size;
 }
 
 /* ── swap_out_process ────────────────────────────────────────── */
 int swap_out_process(proc_entry_t *p)
 {
     uint32_t i, blk;
     if (!p) return -1;
 
     blk = map_malloc(&swap_map, p->pe_size);
     if (!blk) {
         printf("[swapper] ERROR: no swap space for pid=%u\n",  /* was: fprintf(stderr,...) */
                p->pe_pid);
         return -1;
     }
 
     for (i = blk; i < blk + p->pe_size && i < SWAP_DEVICE_BLOCKS; i++)
         swap_device.sd_used[i] = 1;
 
     swap_device.sd_free_blocks -= p->pe_size;
     phys_mem.pm_free_pages     += (int)p->pe_size;
 
     for (i = 0; i < p->pe_size && i < (uint32_t)PHYS_PAGES; i++)
         phys_mem.pm_used[i] = 0;
 
     if (p->pe_state == SCHED_READY)
         p->pe_state = SCHED_READY_SWAP;
     else if (p->pe_state == SCHED_SLEEP)
         p->pe_state = SCHED_SLEEP_SWAP;
 
     p->pe_in_memory = 0;
     p->pe_swap_time = (uint32_t)clock_ticks;
     p->pe_locked    = 0;
 
     printf("[swapper] swapped OUT pid=%u size=%u blk=%u\n",
            p->pe_pid, p->pe_size, blk);
     return 0;
 }
 
 /* ── swap_in_process ─────────────────────────────────────────── */
 int swap_in_process(proc_entry_t *p)
 {
     int i, allocated = 0;
     if (!p) return -1;
     if (!enough_memory_for(p)) return -1;
 
     for (i = 0; i < PHYS_PAGES && (uint32_t)allocated < p->pe_size; i++) {
         if (!phys_mem.pm_used[i]) {
             phys_mem.pm_used[i] = 1;
             allocated++;
         }
     }
 
     phys_mem.pm_free_pages -= (int)p->pe_size;
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
 
 /* ── pick_longest_swapped_out ────────────────────────────────── */
 proc_entry_t *pick_longest_swapped_out(void)
 {
     int i;
     proc_entry_t *best = (proc_entry_t *)0;
     for (i = 0; i < NPROC; i++) {
         proc_entry_t *p = &proc_table[i];
         if (p->pe_state != SCHED_READY_SWAP) continue;
         if (!best || p->pe_swap_time < best->pe_swap_time)
             best = p;
     }
     return best;
 }
 
 /* ── pick_swap_out_candidate ─────────────────────────────────── */
 proc_entry_t *pick_swap_out_candidate(void)
 {
     int i, has_sleeping = 0, best_score = -1;
     proc_entry_t *best = (proc_entry_t *)0;
 
     for (i = 0; i < NPROC; i++) {
         if (proc_table[i].pe_state == SCHED_SLEEP &&
             proc_table[i].pe_in_memory)
             has_sleeping = 1;
     }
 
     for (i = 0; i < NPROC; i++) {
         int score;
         proc_entry_t *p = &proc_table[i];
         if (p->pe_state == SCHED_UNUSED ||
             p->pe_state == SCHED_ZOMBIE) continue;
         if (!p->pe_in_memory) continue;
         if (p->pe_locked)     continue;
 
         if (has_sleeping && p->pe_state == SCHED_SLEEP) {
             score = p->pe_sched.sp_priority +
                     (int)p->pe_sched.sp_residence;
         } else if (!has_sleeping) {
             score = (int)p->pe_sched.sp_residence +
                     p->pe_sched.sp_nice;
         } else
             continue;
 
         if (score > best_score) { best_score = score; best = p; }
     }
     return best;
 }
 
 /* ── wakeup_swapper ──────────────────────────────────────────── */
 void wakeup_swapper(void)
 {
     if (swapper_sleep &&
         phys_mem.pm_free_pages < (PHYS_PAGES / 4)) {
         swapper_sleep = 0;
         printf("[swapper] woken: free_pages=%d\n",
                phys_mem.pm_free_pages);
     }
 }
 
 /* ── Algorithm swapper ───────────────────────────────────────── */
 void swapper(void)
 {
     proc_entry_t *swap_in_proc, *victim;
     printf("[swapper] started\n");
 
     for (;;) {
         swap_in_proc = pick_longest_swapped_out();
 
         if (!swap_in_proc) {
             printf("[swapper] no swap-in candidate, sleeping\n");
             swapper_sleep = 1;
             return;
         }
 
         if (enough_memory_for(swap_in_proc)) {
             swap_in_process(swap_in_proc);
             run_queue_add(swap_in_proc);
             continue;
         }
 
         victim = pick_swap_out_candidate();
 
         if (!victim ||
             victim->pe_state != SCHED_SLEEP ||
             victim->pe_sched.sp_residence < MIN_RESIDENCE) {
             printf("[swapper] no suitable swap-out candidate, sleeping\n");
             swapper_sleep = 1;
             return;
         }
 
         swap_out_process(victim);
     }
 }
 