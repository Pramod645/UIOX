
uiox_kix_psa_process.h   ->  process.c, region.c, sleep_wakeup.c, context.c   (all include it)
uiox_kix_psa_region.h    ->  region.c, process.c
uiox_kix_psa_context.h   ->  context.c, process.c

void uiox_kix_psa_proc_wakeup(uintptr_t wchan);
int  uiox_kix_psa_proc_sleep(uintptr_t wchan, int priority, int interruptible);
Any caller includes proc_algo.h. A separate sleep_wakeup.h would declare the same two prototypes a second time — two places to keep in step, and a redefinition the moment they diverge.


Applied to the two files you named
uiox_kix_psa_proc_algo.h has no .c because its entire content is declarations: the u area type, the sleep hash, the abort-flag struct, and prototypes for functions defined elsewhere. There is nothing to define. In C, a header with a .c twin usually means module with a companion implementation; here the header is a layer-wide interface, so it sits above the sources rather than beside one.

uiox_kix_psa_sleep_wakeup.c has no .h because its two functions are already declared in that same layer-wide header, and no other file declares anything of its own that callers need.

===================================
Two options
Correct the banner now — one-line edit in uiox_kix_psa_context.h, from "belongs to 50_scps, which owns the dispatch mechanism" to something like "has no dispatcher in this tree yet; one must be written."

Or leave it until you build the dispatcher, then write the banner to match whatever you build.

I can also write the dispatcher — uiox_kix_scpcs_dispatch.c with the number table — if you want it. But I'd only do that once the numbering is settled, since a number table built on the wrong SIGCHLD is worse than none.

==========================================
is this uioxkixscpcsdispatch.c having call from system as system call from user space an dif yes then why we are using under 50scpcs files having funtion defination to get call from system call?


Two questions there, and the answer to the second is "we shouldn't, and reading it back, that's a real structural mistake in what I wrote."

Is uiox_kix_scpcs_dispatch.c called from user space?
Yes — this is exactly the entry point a trap lands on. The path is:



user program:  write syscall number in a register, trap
      ↓
CPU:           vector to the trap handler      (10_Arch, per-architecture)
      ↓
arch:          save registers, fill a0..a5 from the argument registers
      ↓
this file:     uiox_kix_scpcs_dispatch(nr, a0..a5, regs)
      ↓
the wrapper:   uiox_kix_scpcs_getpid() etc.
      ↓
back:          rc_r0 / rc_r1 / rc_carry written into the saved frame
So it's the correct place in the call chain — the arch layer hands it a number and arguments, and it hands back a return value and an error flag. That part is right.

But your second question is the sharp one
why we are using under 50_scpcs files having function definition to get call from system call?

I read that as: why are the function definitions sitting under 50_scpcs at all, if a syscall from user space arrives here? And the honest answer is that I've been describing the layering loosely, and it doesn't survive the question.

Here's the problem. The file you pasted — uiox_kix_psa_context.h — says:

System calls belong to 50_scps, which owns the dispatch mechanism and the syscall numbers.

That sentence treats 50_scpcs as the syscall layer: dispatch, numbers, and the calls themselves, all in one directory. But Bach's structure, which everything else here follows, separates those into different boxes:



System call interface          ← number table, argument unpacking, dispatch
      ↓
Algorithms: sys_fork, exec, ...  ← the things that DO the work
      ↓
Process table, regions, sleep/wakeup
In Bach, the syscall interface is a thin layer above the algorithms. It does not own them. And in your tree, the algorithms already have a home — 40_psa, where uiox_kix_psa_process.c holds proc_alloc, proc_set_state, sched_enqueue, and uiox_kix_psa_sleep_wakeup.c holds proc_sleep/proc_wakeup.

So there are three defensible shapes, and I've been writing as though there were one:

Shape	50_scpcs holds	40_psa holds
A. What I built	number table, dispatcher, and all 24 wrappers	algorithms
B. Thin interface	number table, dispatcher, validating stubs only	algorithms and the call bodies
C. What your old tree had	number table, dispatcher, 17 weak stubs	algorithms, plus a rival dispatcher
Shape C was the bug we removed. Shape A is what I wrote. Shape B is arguably what Bach's diagram shows — the interface decides which algorithm runs, and the algorithm is where the body lives.

What's actually wrong with Shape A
Not that it doesn't work. It's that the wrappers under 50_scpcs are doing two jobs at once:

Interface work — validate arguments a kernel entry point mustn't trust, refuse user pointers, map a return into rc_carry. That genuinely belongs to the syscall boundary.
Algorithm work — set_uid's privilege rule, nice's inversion and clamp, getpgrp's missing-field decision. That's process logic, and by the same reasoning that put proc_alloc in 40_psa, it belongs there too.
I wrote them all into 50_scpcs because that's where the 17 stubs used to live, and I never questioned it. Your question exposes that.

What I'd suggest, and it's small
Keep the split by job, not by directory:

50_scpcs — uiox_kix_scpcs_nr.h, uiox_kix_scpcs_dispatch.c, uiox_kix_scpcs_check_user_ptr.c, and the wrappers as validating front-ends (they already do this — every one checks its arguments before acting).

40_psa — the bodies that carry process logic: set_uid, nice, and the getpgrp decision. These are the same three functions whose banners already say "this makes a real decision" or "this is blocked on a missing field" — the ones that are not pure boundary work.

That's three functions moving, not twenty-four, because the rest are genuinely boundary code: getpid reads a field and returns it; fork returns ENOSYS; clock_gettime calls the scheduler.

But before I touch anything: which shape do you want? If 50_scpcs is meant to be the syscall layer in the loose sense — numbers, dispatch, and calls — then nothing needs to move and my banner in uiox_kix_psa_context.h just needs the wording fixed. If it's meant to be the thin interface, then the three process-logic bodies move down and the banner becomes accurate as written.

I'd rather you decide that than have me move files on my own reading of a diagram I still haven't seen — chat-image:2 (ProcessSystemCallAndInterfaces.png) is the one that would settle it.
==================

1. 01_schedular   → unblocks pause(), nano_sleep(), and the sleep half of 40_psa
2. 02_MemMngnt    → fixes PTE_COW + PAGE_SIZE, once the process model is settled
3. 00_IPC         → self-contained, no dependents waiting

=========

go as per 1 step and 2 and then 3 --->Step 1 — the RunQueue fix. Small, and everything else builds on the queue being clean. One change to the struct in scheduler.h, one pass through scheduler.c's four walk loops.

Step 2 — port the three unported sources. timekeeping.c is a straight read of your paste. timer.c and profiler.c too. But I need to know one thing first: does profiler.c or timer.c touch a Process at all? From what I read, timer.c uses TimerNode and profiler.c uses its own Profiler/NmiWatchdog structs — neither names Process. If that's right, they need no porting and only timekeeping.c's xtime reference matters.

Step 3 — the two misrouted rows. Add sysgetitimer to syscalltime.c and point SYSCLOCKGETRES at sysclockgetres.


=====


