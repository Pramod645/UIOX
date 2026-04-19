| File | Algorithms / Structures Implemented |
| --- | --- |
| include/fork.h | fork_result_t, kern_resources_t, resource checking prototypes |
| include/signal.h | 19 System V signals, classes, sigaction_t, issig/psig/kill prototypes |
| include/exec.h | exec_hdr_t, segment descriptors, exec_args_t, xalloc prototype |
| include/exit_wait.h | acct_record_t, wait_status_t, exit/wait prototypes |
| include/brk.h | Break value limits, kernel_brk prototype |
| include/init.h | inittab_entry_t, run levels, process classes, kernel_start prototype |
| src/fork.c | Algorithm 1: kernel_fork — resource check, copy proc slot, dupreg/attachreg regions, dummy context |
| src/signal.c | Algorithms 2 & 3: issig, psig, kernel_kill, kernel_signal, send_signal, dump_core |
| src/exec.c | Algorithms 6 & 7: kernel_exec, xalloc, segment loading, setuid handling |
| src/exit_wait.c | Algorithms 4 & 5: kernel_exit, kernel_wait, reparent_children, accounting |
| src/brk.c | Algorithm 9: kernel_brk — lock region, growreg, zero new space |
| src/init.c | Algorithms 11 & 12: kernel_start (boot), kernel_init (process 1), swapper_loop (process 0) |



#based on PrcsCtrl.txt