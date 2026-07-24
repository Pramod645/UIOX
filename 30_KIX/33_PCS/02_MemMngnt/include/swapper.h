#ifndef SWAPPER_H
#define SWAPPER_H

#include "uiox_klibc.h"
#include "scheduler.h"

/* ── Memory map entry (in-core free space table) ────────────── */
#define MAP_MAX_ENTRIES 512

typedef struct map_entry {
    uint32_t me_addr;       /* start address / block number     */
    uint32_t me_size;       /* number of contiguous units       */
    int      me_valid;      /* entry is in use                  */
} map_entry_t;

/* ── Resource map descriptor ────────────────────────────────── */
typedef struct res_map {
    map_entry_t rm_entries[MAP_MAX_ENTRIES];
    int         rm_nentries;
    const char *rm_name;
} res_map_t;

/* ── Swap device descriptor ─────────────────────────────────── */
#define SWAP_DEVICE_BLOCKS  8192    /* simulated swap capacity   */

typedef struct swap_device {
    uint8_t  sd_used[SWAP_DEVICE_BLOCKS]; /* 1 = block in use   */
    uint32_t sd_free_blocks;
    res_map_t sd_map;
} swap_device_t;

/* ── Main-memory free page tracking ────────────────────────── */
#define PHYS_PAGES      2048        /* simulated physical pages  */

typedef struct phys_mem {
    int      pm_free_pages;
    uint8_t  pm_used[PHYS_PAGES];   /* 1 = page in use          */
} phys_mem_t;

/* ── Swapper constants ──────────────────────────────────────── */
#define SWAP_IN_EVENT   1
#define MIN_RESIDENCE   5           /* ticks before eligible to swap out */

/* ── Globals ────────────────────────────────────────────────── */
extern swap_device_t swap_device;
extern phys_mem_t    phys_mem;
extern res_map_t     swap_map;
extern int           swapper_sleep; /* 1 = swapper is sleeping   */

/* ── Function prototypes ────────────────────────────────────── */

/* Algorithm malloc — first-fit allocation on a resource map */
uint32_t map_malloc(res_map_t *map, uint32_t units);
void     map_free  (res_map_t *map, uint32_t addr, uint32_t units);
void     map_init  (res_map_t *map, const char *name,
                    uint32_t start, uint32_t total_units);

/* Swap I/O */
int  swap_out_process(proc_entry_t *p);
int  swap_in_process (proc_entry_t *p);
int  enough_memory_for(proc_entry_t *p);

/* Algorithm swapper */
void swapper(void);
void wakeup_swapper(void);

/* Swap-out candidate selection helpers */
proc_entry_t *pick_longest_swapped_out(void);
proc_entry_t *pick_swap_out_candidate(void);

#endif /* SWAPPER_H */
