| File | Algorithms / Structures |
| --- | --- |
| include/process.h | proc_t, proc_state_t, signals, scheduling params, timers |
| include/region.h | region_t, pregion_t, pte_t, region types/flags |
| include/context.h | reg_context_t, sys_context_t, interrupt vector table |
| include/proc_algo.h | u_area_t, syscall table, sleep hash, all prototypes |
| src/process.c | proc_alloc/free/find, proc_set_state, sched_enqueue/pick |
| src/region.c | allocreg, attachreg, growreg, loadreg, freereg, detachreg, dupreg |
| src/context.c | inthand, context_save/restore/switch, intr_register |
| src/syscall.c | syscall (algorithm 2), syscall_register, global u area |
| src/sleep_wakeup.c | proc_sleep (algorithm 10), proc_wakeup (algorithm 11) |


#based on PrcsStruct.txt