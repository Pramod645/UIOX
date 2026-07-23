What Actually Exists (and its quality)
✅ 01_schedular/src/scheduler.c — Good foundation
Multi-level priority queue (RunQueue with MAX_PRIORITY_QUEUES bands)
enqueue_process, dequeue_process, schedule_process
jiffies counter, XTime wall clock
Priority recalculation / feedback (time-slice aging)
Problem: uses <stdio.h>, <stdlib.h>, <string.h> — won't compile freestanding
✅ 01_schedular/src/timer.c — Good foundation
Timer wheel with tv1[] buckets
timer_add, timer_del, timer_tick (fires expired callbacks)
TimerNode with fn/data callback model
Problem: same libc dependencies + uses calloc/free
⚠️ 01_schedular/src/sched.c — Thin wrapper only
Wraps scheduler.c behind a sched_proc_t array
kernel_sched_setscheduler, kernel_sched_yield, kernel_sched_tick
Missing: no connection to arch_syscall_dispatch or the IRQ timer tick
⚠️ 02_memory-managment/src/mm.c — Skeleton only
g_brk = 0x10000 hardcoded — not connected to real physical memory
sys_brk, sys_mmap stubs return addresses but never touch page tables
No physical page allocator — phys_alloc_page() still not implemented
No buddy allocator, no slab/kmalloc
✅ 02_memory-managment/src/page_fault.c — Surprisingly complete
vfault() and pfault() handlers with COW (Copy-on-Write)
alloc_physical_page(), free_physical_page() using a circular free list
Demand-zero fill, swap-in path (simulated)
Problem: uses <string.h>, <stdlib.h>, calloc, memcpy, memset
pfdata_table[] is a static array — not wired to real DRAM