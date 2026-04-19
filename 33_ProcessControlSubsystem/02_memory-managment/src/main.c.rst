#include "../include/scheduler.h"
#include "../include/clock.h"
#include "../include/swapper.h"
#include "../include/page_fault.h"
#include <stdio.h>
#include <string.h>

/* ── Bootstrap helper ────────────────────────────────────────── */
static proc_entry_t *make_proc(uint32_t pid, int pri,
                                int nice, uint32_t size)
{
    proc_entry_t *p = &proc_table[pid % NPROC];
    memset(p, 0, sizeof(*p));
    p->pe_pid               = pid;
    p->pe_state             = SCHED_READY;
    p->pe_in_memory         = 1;
    p->pe_size              = size;
    p->pe_sched.sp_priority = pri;
    p->pe_sched.sp_nice     = nice;
    p->pe_sched.sp_time_slice = TIME_QUANTUM;
    p->pe_sched.sp_residence  = 0;
    return p;
}

int main(void)
{
    /* ── Initialize subsystems ──────────────────────────────── */
    sched_init();
    clock_init();
    map_init(&swap_map, "swap", 1, SWAP_DEVICE_BLOCKS - 1);
    phys_mem.pm_free_pages = PHYS_PAGES / 2;

    /* ── Create some test processes ─────────────────────────── */
    proc_entry_t *p1 = make_proc(1, 60, 0, 4);
    proc_entry_t *p2 = make_proc(2, 40, 5, 8);
    proc_entry_t *p3 = make_proc(3, 70, 0, 4);

    run_queue_add(p1);
    run_queue_add(p2);
    run_queue_add(p3);

    /* ── Simulate a few clock ticks ─────────────────────────── */
    for (int t = 0; t < 5; t++) {
        printf("\n=== TICK %d ===\n", t);
        clock_interrupt(0xC0001234, 0x400500);
        if (need_resched) { need_resched = 0; schedule_process(); }
    }

    /* ── Simulate a validity fault ──────────────────────────── */
    printf("\n=== VFAULT TEST ===\n");

    /* Build a minimal region and a not-present PTE */
    pte_t pte_arr[1];
    memset(pte_arr, 0, sizeof(pte_arr));
    pte_arr[0].pte_state = PAGE_DEMAND_ZERO;
    pte_arr[0].pte_flags = PTE_DEMAND_ZERO;

    fault_region_t reg = {
        .fr_vaddr_start = 0x1000,
        .fr_size        = PAGE_SIZE,
        .fr_pgtbl       = pte_arr,
        .fr_npages      = 1
    };

    /* Prime the free page list */
    free_pages.fpl_count = 4;
    for (int i = 0; i < 4; i++)
        free_pages.fpl_pages[i] = (uint32_t)(10 + i);
    free_pages.fpl_head = 0;
    free_pages.fpl_tail = 4;

    current_proc = p1;
    vfault(0x1000, &reg);

    /* ── Simulate a protection (COW) fault ───────────────────── */
    printf("\n=== PFAULT TEST ===\n");
    pte_arr[0].pte_flags |= PTE_COW | PTE_VALID;
    pfdata_table[pte_arr[0].pte_pfn % PHYS_PAGES].pfd_refcnt = 2;
    pfault(0x1000, &reg);

    /* ── Simulate swapper ────────────────────────────────────── */
    printf("\n=== SWAPPER TEST ===\n");
    p3->pe_state     = SCHED_READY_SWAP;
    p3->pe_in_memory = 0;
    p3->pe_swap_time = 0;
    swapper();

    return 0;
}
