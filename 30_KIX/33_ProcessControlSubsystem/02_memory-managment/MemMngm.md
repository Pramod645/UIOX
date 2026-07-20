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


