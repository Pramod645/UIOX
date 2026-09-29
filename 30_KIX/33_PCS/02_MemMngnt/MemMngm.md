| File | Algorithms Implemented |
| --- | --- |
| include/scheduler.h | proc_entry_t, sched_param_t, proc_tms_t, run_queue_t |
| include/clock.h | callout_t, sys_stats_t, kprof_t |
| include/swapper.h | map_entry_t, res_map_t, swap_device_t, phys_mem_t |
| include/page_fault.h | pte_t, disk_blk_desc_t, pfdata_t, fault_region_t, page states |
| src/scheduler.c | Algorithm 1 schedule_process, round-robin multilevel feedback, recalc_priority, fair-share |
| src/clock.c | Algorithm 2 clock_interrupt, callout table, profiling, per-second priority decay |
| src/swapper.c | Algorithm 1 map_malloc (first-fit), Algorithm 2 swapper, swap_in/out_process |
| src/page_fault.c | Algorithm 3 vfault (5 page states), Algorithm 4 pfault (COW handling) |

==============================================================================================
https://docs.kernel.org/arch/arm64/memory.html
https://gcc.gnu.org/projects/c-status.html

========================
What 02_MemMngnt contains vs what Bach specifies
Your pasted chapter gives four algorithms. Here's how the files map:

Bach algorithm	Implemented in	State
1. malloc — first-fit on a map	swapper.c → map_malloc	✅ complete, faithful
2. swapper — swap in/out	swapper.c → swapper	⚠️ logic right, infinite loop
3. vfault — validity fault	page_fault.c → vfault	⚠️ simplified — no sleep, no locking order
4. pfault — protection/COW fault	page_fault.c → pfault	❌ COW never fires
Plus two things Bach doesn't have: uiox_phys_alloc.c/mm.c (a second physical allocator) and clock.c (a second clock, duplicating 01_schedular's).

The four findings, worst first
1. There are THREE PHYS_PAGES-style page abstractors, all disagreeing.

Source	Page count	Page size	PTE flags
page_fault.h	PHYS_PAGES 2048	PAGE_SIZE 4096	7 flags, COW 0x020
swapper.h	PHYS_PAGES 2048	—	none
mm.c / uiox_phys_alloc.c	UIOX_MAX_PAGES 16384	UIOX_PAGE_SIZE 4096	none
40_psa's region header	UIOX_KIX_PSA_MAX_PAGES 1024	UIOX_KIX_PSA_PAGE_SIZE 4096	5 flags, COW 0x010


2. mm.c and uiox_phys_alloc.c are the same file twice. Byte-for-byte the same logic — same uiox_mm_init, same free list, same UIOX_MAX_PAGES 16384. The only differences: mm.c uses bare uintptr_t and the PTR_TO_UINTPTR memcpy macros (the arm32 -Werror fix); uiox_phys_alloc.c uses uix_uintptr_t from uix_types.h. One is redundant.


3. swapper() never terminates. Bach's Algorithm 2 loops with sleep(event must swap in); goto loop;. This version:

c


if (!swap_in_proc) {
    swapper_sleep = 1;
    return;              /* ← returns instead of sleeping */
}
Returning is better than spinning — but the second return (no suitable victim) is the bug: the caller must re-enter swapper() for progress, and nothing in clock.c does. wakeup_swapper() only clears the flag; it never calls swapper().


4. page_fault.c's vfault skips Bach's two hardest steps. Bach says "while (page contents not valid) sleep (event contents become valid)" — for a page another process faulted first. And "read virtual page from swap dev or exec file; sleep (event I/O done)". This version calls read_page_from_swap() which sets pfd_content_valid = 1 synchronously — no I/O, no sleep. So the locking is present but the concurrency it guards against cannot happen.


The process model in this layer
swapper.h defines proc_entry_t with pe_pid, pe_state, pe_sched, pe_tms, pe_size, pe_swap_time, pe_alarm, pe_group_id.

Compare 40_psa's uiox_kix_psa_proc_t: it has p_pid, p_state, p_sched, p_timers, p_size, p_alarm_expire… The same fields, renamed.

And swapper.c's scheduler.h defines a third state enum: SCHED_UNUSED/RUNNING/READY/SLEEP/READY_SWAP/SLEEP_SWAP/ZOMBIE/CREATED — eight states that map one-for-one onto PSA's ten.

So the answer to your earlier question is now clear: proc_entry_t is not a swap record — it IS a rival process entry. Merging means 02_MemMngnt binds to uiox_kix_psa_proc_t, and proc_entry_t goes.


What I'd do, and the order
This layer is bigger than 01_schedular and it holds three of the four findings. In dependency order:

Collapse the page constants first — one header with PAGE_SIZE, PAGE_SHIFT, and the shared PTE flag bits (PTE_COW 0x020, so 40_psa's 0x010 moves). That fixes finding 1 and the original item 1 in one edit.
Delete one of mm.c / uiox_phys_alloc.c — keeping whichever your build config expects. uiox_phys_alloc.c is the one archruntime.c externs, so I'd keep that and drop mm.c.
Merge proc_entry_t into uiox_kix_psa_proc_t — biggest edit; touches swapper.c, page_fault.c, clock.c here plus 02_MemMngnt's scheduler.h.
Fix swapper()'s re-entry — wire wakeup_swapper() to actually call it.
Delete 02_MemMngnt/src/clock.c — it's a second clock, duplicating 01_schedular's. Its callout_add collides with the one I just wrote.

==================
uiox_kix_paging.h replaces page_fault.h.


uiox_kix_paging.h replaces page_fault.h. It keeps only what this layer needs — uiox_kix_pte_t (four fields, the handler's view), uiox_kix_disk_blk_desc_t, uiox_kix_pfdata_t, uiox_kix_fault_region_t, the page cache and the free list — and takes page geometry and the PTE bits from uiox_kix_pagemap.h. No process type anywhere in it.

uiox_kix_swap.h replaces swapper.h. It keeps the resource map (Bach's Algorithm 1), the swap device, and the swapper's view of main memory. PHYS_PAGES and PTE_*: zero definitions, as intended — both now come from the shared header.
=========================

Item in 02_MemMngnt/clock.c	Where it belongs now
callout_add / callout_del / callout_tick	01_schedular — uiox_kix_clock.c owns these. This is the collision
clock_interrupt	01_schedular — uiox_kix_clock.c's clock_tick
profile_kernel_tick / profile_user_tick	01_schedular — uiox_kix_profiler.c
gather_system_stats, gather_per_process_stats, adjust_cpu_utilization	02_MemMngnt — genuinely this layer's
So the replacement keeps three functions and drops six. But there's a problem I should flag before writing it:


==================
Updated — 7 files written this session
include/ — 3

File	Lines	Replaces
uiox_kix_paging.h	209	page_fault.h
uiox_kix_swap.h	226	swapper.h
uiox_kix_accounting.h	140	(from clock.h)
src/ — 4

File	Lines	Replaces
uiox_phys_alloc.c	208	both itself and mm.c
uiox_kix_swapper.c	481	swapper.c
uiox_kix_page_fault.c	442	page_fault.c
uiox_kix_accounting.c	176	(three functions from clock.c)

=============
Why uiox_kix_pagemap.h one layer above
Because two layers walk page tables, and only one of them can own the definition.

The PTE_COW defect is the proof. 40_psa's dupreg() sets the bit; 02_MemMngnt's pfault() tests it. Two different subsystems, in two different directories under 33_PCS:



33_PCS/
├── 40_psa/        dupreg sets PTE_COW
├── 01_schedular/
└── 02_MemMngnt/   pfault tests PTE_COW

f uiox_kix_pagemap.h lives in 02_MemMngnt/include/, then 40_psa has to reach down into a sibling subsystem to get its flag bits — and 02_MemMngnt is not a parent of 40_psa, it's a peer. A peer reaching into a peer is how you get a circular dependency when the other direction is later needed: uiox_kix_swap.h already includes uiox_kix_psa_process.h from 40_psa, so 02_MemMngnt → 40_psa exists. Adding 40_psa → 02_MemMngnt closes a loop the compiler cannot resolve.

33_PCS/include/ is the place both peers can see without either depending on the other. That's what a shared header is for.

There's a second reason, and it's the one that would have prevented the bug. When the definition sits in one of the two layers that uses it, the other layer's author has no reason to look at it. page_fault.h defined PTE_COW 0x020; the region header defined its own 0x010; nothing forced the two to be compared. A header in 33_PCS/include/ is named for the shared concept, so it's the obvious place to check — and it's now the only place a bit is defined, which makes the comparison unnecessary rather than merely likely.


Are these seven files all of memory management?
No. Same answer as before, and I'll keep it short since I gave the full table last message.

The 7 files  is the seven files I wrote, not the whole layer. Beyond them you still need:

From outside: uiox_kix_pagemap.h (the shared one), uiox_kix_psa_process.h, uiox_kix_psa_region.h, uiox_kix_scheduler.h, uiox_klibc.h
Untouched in your tree: include/mm.h — declares kernel_mmap, kernel_munmap, kernel_mprotect with no bodies, and kernel_brk with one
Loose end: include/clock.h — everything in it moved to 01_schedular, nothing replaced it
To delete: clock.c, mm.c, page_fault.c, swapper.c, page_fault.h, swapper.h, scheduler.h

And one correction to the header you pasted
Its banner says PAGE_SHIFT and PAGE_SIZE are bare names "for the headers that used them before this one existed". That's true, but there's a sharper version: those #ifndef guards mean whichever header is included first wins. If a source includes page_fault.h before uiox_kix_pagemap.h, the old definitions take precedence — including a PTE_COW of 0x020, which happens to be correct, but by accident rather than by design.

The guards are a migration aid, not a fix. They stop existing when page_fault.h is deleted — which is on your deletion list.