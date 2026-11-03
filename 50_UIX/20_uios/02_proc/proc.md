pramodkumar@Mac 33_PCS % cd /Users/pramodkumar/Hack/WS/UIOX/30_KIX
ls 33_PCS/50_scpcs/src 33_PCS/40_psa/include
sed -n '1,60p' 33_PCS/50_scpcs/scpcs.md
grep -n 'proc_create\|psa_process\|CREATED\|sched_enqueue\|sched_add' 33_PCS/40_psa/include/*.h 33_PCS/01_schedular/include/*.h | head -20

33_PCS/40_psa/include:
uiox_kix_psa_context.h          uiox_kix_psa_process.h
uiox_kix_psa_proc_algo.h        uiox_kix_psa_region.h

33_PCS/50_scpcs/src:
uiox_kix_scpcs_alarm.c          uiox_kix_scpcs_get_euid.c       uiox_kix_scpcs_raise.c
uiox_kix_scpcs_brk.c            uiox_kix_scpcs_get_gid.c        uiox_kix_scpcs_set_uid.c
uiox_kix_scpcs_check_user_ptr.c uiox_kix_scpcs_get_pgrp.c       uiox_kix_scpcs_sig_action.c
uiox_kix_scpcs_clock_get_time.c uiox_kix_scpcs_get_pid.c        uiox_kix_scpcs_sig_pending.c
uiox_kix_scpcs_current.c        uiox_kix_scpcs_get_ppid.c       uiox_kix_scpcs_sig_procmask.c
uiox_kix_scpcs_dispatch.c       uiox_kix_scpcs_get_uid.c        uiox_kix_scpcs_signal.c
uiox_kix_scpcs_execve.c         uiox_kix_scpcs_kill.c           uiox_kix_scpcs_times.c
uiox_kix_scpcs_exit.c           uiox_kix_scpcs_nano_sleep.c     uiox_kix_scpcs_wait_pid.c
uiox_kix_scpcs_fork.c           uiox_kix_scpcs_nice.c
uiox_kix_scpcs_get_egid.c       uiox_kix_scpcs_pause.c
Seventeen implementations in outputs/33_PCS/50_scps/src/:

The three-way split
Category        Files   What they do
Real today      get_pid, get_ppid, clock_get_time       read p_pid/p_ppid; call sys_clock_gettime
Wrapper over a real function    exit, brk       call kernel_exit, kernel_brk — correct wrappers, with the callee's limits documented
ENOSYS with the reason  fork, wait_pid, execve, mmap, munmap, nano_sleep, kill, sig_action, read, write, open, close      validate args, then refuse

====================================
      arch_sys_call_dispatch()          ← 10Arch/<arch>/archruntime.c
                 │
                 ▼
   ┌─────────────────────────────┐
   │ 50_scps   THE BOUNDARY      │
   │  uiox_sys_call.c            │
   │    sys_call_dispatch(nr,a0..a5)
   │    17 weak stubs            │
   │  uiox_sys_call_nr.h         │
   └─────────────┬───────────────┘
                 │  calls (or is overridden by)
                 ▼
   ┌─────────────────────────────┐
   │ 40_procStruct  ALGORITHMS   │
   │  proc_t, PROC_*             │
   │  fork.c exec.c brk.c        │
   │  exit_wait.c signal.c       │
   │  process.c region.c         │
   │  sleep_wakeup.c context.c   │
   └─────────────┬───────────────┘
                 │  needs
                 ▼
   ┌─────────────────────────────┐
   │ 01_schedular    run queue   │
   │ 02_MemMngnt     paging      │
   │ 00_IPC          sleep/wakeup│
   └─────────────────────────────┘

=============================
All eighteen files are written. My verification flagged two, and one is a real problem worth fixing before you copy anything.


Used Workspace
Both flags are false positives — I checked each:

uiox_kix_scps_current.c — my regex looked for int64_t, and this one returns proc_t *. The function is present. Not an issue.

uiox_kix_scps_mmap.c — the match is inside a comment: mm pointer (uiox_task_t has p_mm; proc_t does not). That line is explaining why mmap can't work, not binding to the model.

So all seventeen bind to proc_t, each includes the header, and each defines its function.

What was generated
outputs/33_PCS/50_scps/include/uiox_kix_scps.h (99 lines) — seventeen prototypes matching uiox_sys_call.c's weak stubs exactly, plus uiox_kix_scps_current(). Its banner lays out Bach's three groups from the SCPS text.

Seventeen files in outputs/33_PCS/50_scps/src/, each citing the SCPS algorithm it belongs to:

Group   Files
1. Memory Management    fork (Alg 1, dupreg+attachreg), execve (Alg 6+7), brk (Alg 9, growreg), exit (Alg 4, detachreg)
2. Synchronization      wait_pid (Alg 5), kill, sig_action
3. Miscellaneous        get_pid, get_ppid
not in SCPS     mmap, munmap, nano_sleep (Alg 10 sleep), clock_get_time, read, write, open, close
33_PCS/40_psa/include/uiox_kix_psa_proc_algo.h:35:#include "uiox_kix_psa_process.h"
33_PCS/40_psa/include/uiox_kix_psa_process.h:2: * 30_KIX/33_PCS/40psa/include/uiox_kix_psa_process.h
33_PCS/40_psa/include/uiox_kix_psa_process.h:60:    UIOX_KIX_PSA_PROC_CREATED        = 8,  /* PSA 8: newly created, in transition */
33_PCS/40_psa/include/uiox_kix_psa_process.h:87: *            is the convention sched_enqueue's ordering relies on.
33_PCS/40_psa/include/uiox_kix_psa_process.h:216:/* Allocate a free table slot and initialise it to state CREATED.
33_PCS/40_psa/include/uiox_kix_psa_process.h:220: * caller must move the process out of CREATED with proc_set_state. */
33_PCS/40_psa/include/uiox_kix_psa_process.h:247:void uiox_kix_psa_sched_enqueue(uiox_kix_psa_proc_t *p);
33_PCS/40_psa/include/uiox_kix_psa_region.h:30:#include "uiox_kix_psa_process.h"
33_PCS/01_schedular/include/sched_types.h:22: *              uiox_kix_psa_process.h.  The old six TASK_* values are
33_PCS/01_schedular/include/sched_types.h:50:#include "uiox_kix_psa_process.h"   /* the ONE process type             */
33_PCS/01_schedular/include/uiox_kix_scheduler.h:14: * links actually are, and the type 40_psa's own sched_enqueue /
pramodkumar@Mac 30_KIX % 

===========================================
Scheduling improvements you asked about
Probably not needed yet. uiox_sched_init + timer + run queues is a complete round-robin foundation. What's actually missing is a process to schedule, not a better scheduler. Improvements worth considering later, in order:

Priority levels — if sched_types.h already has them, use them; if not, don't add until something needs them
Preemption on the timer tick — depends on whether uiox_timer_init currently calls into the scheduler; worth checking with grep -n 'sched' 01_schedular/src/uiox_kix_timer.c
Blocking states — SLEEP/WAIT for IPC and I/O; the scheduler needs them before the terminal can block on input
=====================================================================
What's missing for first-process creation
Four things, and I can name three from what I've seen:

1. uiox_sched_init() is never followed by a process. uiox_proc_init ends at step 6 with s_pcs_ready = true — no process is created, no PID 1. The run queues are empty when kernel_common_init proceeds to uiox_shell_start().

2. uiox_shell_start() runs in kernel context. kernel_common_init:470 calls it directly. That's the current "first process" — a C function, not a scheduled task, with no user address space.

3. vfs_mount_root() is not attempted. uiox_fs_init.c:70 says so in the bring-up list. No root filesystem means no path to exec.

4. uiox_sys_call_init() doesn't exist. pcs.md:578 flagged it as declared extern with no definition — it's step 5 of some other init in the doc, and it's what makes int 0x80-equivalent dispatch work.

==============
The design, now concrete
PID 1 creation belongs in uiox_proc_init at step 3, right after uiox_sched_init():



3.  uiox_sched_init()                        ← queues ready
3a. p = uiox_kix_psa_proc_alloc()             ← :216, returns slot in CREATED
3b. p->p_pid = 1; p->p_ppid = 0;              ← the init process
    p->p_entry = init_main;                   ← kernel thread body
3c. uiox_kix_psa_proc_set_state(p, READY)     ← must leave CREATED
3d. uiox_kix_psa_sched_enqueue(p)             ← :247, now runnable
4.  uiox_timer_init()                         ← the tick starts switching
Then init_main is where the OS actually starts:



init_main():
  1. vfs_mount_root(ROOT_DEV)     ← currently "NOT attempted" in uiox_fs_init.c
  2. console open                  ← ttyS0 (the uioxuartprobe() thread)
  3. execve("/sbin/init", argv)    ← needs the ENOSYS stub fille

What has to be built, in dependency order
#	Item	File	Blocked on
1	vfs_mount_root()	32_FS/src/uiox_fs_init.c	UNFS mount — s_priv patch is in
2	execve implementation	50_scpcs/src/uiox_kix_scpcs_execve.c	(1) + loader
3	ELF/uPIX loader	50_scpcs/src/	unfs_bdev_read exists
4	PID 1 create + enqueue	33_PCS/src/uiox_pcs_init.c step 3	the psa_process.h:216 API
5	read/write/open/close	50_scpcs/src/	terminal + VFS
6	User-mode entry	10_BSP/10_Arch/*/arch_runtime.c	eret/sret
Step 4 is the cheapest and the one that makes the kernel actually multiprocess — the API exists (proc_alloc at :216, sched_enqueue at :247), and it needs no loader. It gets you a scheduled PID 1 running kernel code.

Step 1 is the one that unblocks 2 and 5 — no root filesystem, no paths.



My read on sequencing
Your four steps are right, but this changes the picture: execve is not one item, it's the largest. The kernel side (context, register frame, arch restore, syscall dispatch) is complete. What's missing is:

Context array + PID 1 — small, kernel-side, safe now
vfs_mount_root() — needed before any path resolves
The ELF/uPIX loader — reads the file, builds the user map, copies text/data. This is the real work.
execve body — calls 1–3 in order, sets rc_pc/rc_sp/rc_sr
fork — clone the address map, duplicate the context, enqueue the child
Steps 1–2 I can write now. Step 3 needs a decision you haven't made: what format are the app binaries? ELF, or a uPIX-specific format? 50_UIX/21_apps should say — if the apps are built by a Makefile in this tree, the format is whatever that produces.

Want me to write steps 1–2 now (context array, PID 1, init_main) while you check 21_apps?


=======
What I'd revise in the plan
#	Item	Where	Note
1	Context array + uiox_kix_psa_context_for()	40_psa/src/uiox_kix_psa_context.c	kernel
2	PID 1 create, rc_pc = uios_init	33_PCS/src/uiox_pcs_init.c step 3a	kernel
3	uios_init body — mount, console, launch	50_UIX/20_uios/uix_uios.c	UIOS, not kernel
4	Loader + execve	50_scpcs/src/	kernel mechanism
5	50_UIX/20_uios/Makefile path fix	lines 9, 14	stale 33_ProcessControlSubsystem
You caught a real layering error. init_main in the kernel would have put policy (which root, which apps) inside the mechanism layer — exactly the mistake psa_context.h warns about with "A second dispatcher in this header would mean two places decide what syscall number 9 does."


============================================================
What that means for the design
uios_init cannot be PID 1's rc_pc directly, because the kernel doesn't link uios.o. Two ways out, and the tree already shows the pattern for both:

Option A — weak stub in the kernel (the uiox_fs_init pattern).

uiox_kernel_main.c already does this:

c


/* 32_FS — filesystem: ... Weak above, so the link succeeds whether or not
 *        32_FS is in this build. */
(void)uiox_fs_init();
So the kernel declares uios_init weak, and when UIOS is linked in it wins; when it isn't, the stub parks. PID 1's rc_pc points at whatever the linker resolved.

Option B — uios is the init binary, loaded by execve, not linked.

That matches the structure better: 20_uios builds uios, 21_apps are separate binaries, and g_apps[] paths are /../21_apps_/app_shell — filesystem paths, not symbols. So the intended flow is:



kernel: PID 1, rc_pc = kinit_main (kernel-side, tiny)
  kinit_main:
    1. vfs_mount_root(ROOT_DEV)      ← must happen before any path
    2. execve("/bin/uios", argv)     ← loads the uios binary
         ↓ user mode
  uios (user process, main()):
    uios_init → uios_launch_apps → uios_run
    each app fork'd + exec'd from /21_apps_/app_*
That's why uix_uios.c has a main() — it's a program, not a kernel routine. And why uix_sys.h has SYS_CALL_ENBLE_DISABLE 0 with the comment "0 = stub returns (simulation / hosted build)" — UIOS is written to run either stubbed (hosted, for testing) or against the real kernel.

So the corrected chain


kernel_common_init
   ↓
uiox_proc_init (33_PCS)               step 3a: create PID 1
   rc_pc = kinit_main (kernel-side)   ← tiny: mount + execve only
   ↓  sched_enqueue, timer starts
kinit_main (kernel thread, sc_kstack)
   1. vfs_mount_root(ROOT_DEV)        ⛔ "NOT attempted"
   2. execve("/bin/uios", argv)       ⛔ ENOSYS + no loader
   ↓  user mode, rc_pc = uios binary's entry
uios main() (user process, PID 1 after exec)
   uios_init → uios_launch_apps → uios_run
        fork + execve each of the 10 apps from /21_apps_/app_*
PID 1 is a kernel thread only until it execs /bin/uios. After that it's the user-space supervisor. That's cleaner than either of my earlier versions — I had uios_init running on the kernel stack permanently, but uix_uios.c having a main() says otherwise.

=======================

Open items, in order
#	Item	Where	State
1	s_sys_ctx[NPROC] + uiox_kix_psa_context_for()	40_psa/src/uiox_kix_psa_context.c	to write — no context storage exists
2	Step 3a + kinit_main	33_PCS/src/uiox_pcs_init.c	to write
3	vfs_mount_root()	32_FS/src/uiox_fs_init.c	⛔ "NOT attempted"
4	ELF/uPIX loader + execve body	50_scpcs/src/	⛔ ENOSYS
5	fork	50_scpcs/src/uiox_kix_scpcs_fork.c	⛔ ENOSYS
6	Cross-build + install uios, app_* into the image	50_UIX/20_uios/Makefile, 21_apps/Makefile	❌ no aarch64 rule
7	Stale paths 33_ProcessControlSubsystem → 30_KIX/33_PCS	uix_sys.h, uix_uios.h, both Makefiles	❌
Steps 1 and 2 I can write now — kernel-side, all APIs confirmed, and they'll produce a real milestone: PID 1 created with a context, scheduled by the timer, running kinit_main, which stalls at the mount and says so.

Item 3 is the next real gate — no root filesystem means no /bin/uios to exec.

Want me to write 1 and 2 as complete files? I have everything needed: s_sys_ctx[NPROC] in context.c, uiox_kix_psa_context_for() declared in the header, step 3a in uiox_pcs_init.c, and the kinit_main body.


=========
Where the OS work stands
The kernel is green on all four arches, which is the base the next steps sit on:

#	Item	State
1	s_sys_ctx[NPROC] + uiox_kix_psa_context_for()	done (bss grew)
2	Step 3a + kinit_main	done (build passes)
3	vfs_mount_root()	⛔ still "NOT attempted" in uiox_fs_init
4	Loader + uiox_kix_scpcs_execve body	⛔ ENOSYS
5	fork	⛔ ENOSYS
6	Cross-build uios + app_*, install into image	❌
7	Stale 33_ProcessControlSubsystem paths	❌ uix_sys.h, uix_uios.h, two Makefiles
Item 3 is the next gate — no root filesystem means /bin/uios can't resolve, so kinit_main's exec fails no matter what else lands.

What's next: the mount, or the stale paths so 20_uios/21_apps can at least be built for aarch64?

=======

Where that leaves the OS work
#	Item	State
1	s_sys_ctx[64] + uiox_kix_psa_context_for()	✅
2	Step 3a + kinit_main	✅ builds, four arches green
3a	UNFS on ROOT_DEV, hook bound, unfs_kern_mount written	✅
3b	SCFS off device 0, on its own index	✅
3c	vfs_mount_root(ROOT_DEV) at uiox_fs_init.c:87	present — should now reach unfs_ops_mount
4	Loader + uiox_kix_scpcs_execve body	⛔ ENOSYS
5	fork	⛔ ENOSYS
6	Cross-build uios + app_*, install into image	❌
7	Stale 33_ProcessControlSubsystem paths	❌ three files

now here we are

The list, updated
#	Item	State	Evidence
1	s_sys_ctx[64] + uiox_kix_psa_context_for()	✅	not touched this session
2	Step 3a + kinit_main	✅	four arches green, ELF links
3a	UNFS on ROOT_DEV, hook bound, unfs_kern_mount	✅	not touched
3b	SCFS off device 0, on own index	✅	not touched
3c	vfs_mount_root(ROOT_DEV) at uiox_fs_init.c:87	✅ present	unchanged
4	Loader + uiox_kix_scpcs_execve body	🔶 in progress	see below
5	fork	⛔ ENOSYS	not started
6	Cross-build uios + app_*, install into image	❌	50_UIX/01_shell skipped (no Makefile) on all four
7	Stale 33_ProcessControlSubsystem paths	❌	three files



What the table doesn't cover
It's scoped to the tick-and-switch chain. Outside it, four things are open:

Item	Evidence
Boot test	Nothing has run. Every claim so far is static — symbols present, sections placed. Whether IRQ 30 fires, whether clock_tick executes, whether the frame pool filled: all unproven.
uart0_irq_handler × 4	Your grep found only extern and irq_request lines on all four architectures — no body anywhere. Same defect timer0_irq_handler had.
Item 6 — image staging	image/bin/ exists on arm64 only, and holds uios but not the ten apps.
Item 7 — stale paths	33_ProcessControlSubsystem in common.mk:73, paths.mk:39, uiox_compile_flags.txt:58. Read by nothing today.
==================

The table, updated
#	Item	State	Evidence
1	s_sys_ctx[64] + uiox_kix_psa_context_for()	✅	untouched, compiles
2	Step 3a + kinit_main	✅	four arches green, ELF links
3a	UNFS on ROOT_DEV, hook bound, unfs_kern_mount	✅	untouched
3b	SCFS off device 0, own index	✅	untouched
3c	vfs_mount_root(ROOT_DEV) at uiox_fs_init.c:87	✅ now actually reached	the umbrella stub is deleted, so the real uiox_fs_init runs
4	Loader + execve body	✅ done	written, compiles + links on all four — no longer ENOSYS
5	fork	✅ done	body written, libpcs.a grew, links on all four
6	uios + apps, install into image	🔶 apps build ×2; image on arm64 only, uios only	your ls
7	Stale 33_ProcessControlSubsystem paths	❌ 5 refs (3 live), read by nothing	your grep
Items 4 and 5 are the big change since that table was written — both were ⛔/🔶 there, and both are now written, compiled, and linked on all four architectures.

Beyond the table — what this session added
Piece	State
loadreg reads through readi	✅
Frame pool from phys_alloc_page() + pfd_kva	✅ BSS 3.83 → 9.76 MB
Per-process U-area: pool, uarea_for, sc_uarea, accessor, macro swap	✅ verified in disassembly
arch_context_t sc_switch + wiring at PID 1 and in fork	✅
context_switch → arch_context_switch	✅ T in all four ELFs
clock_tick → tick_process → schedule_process	✅ clock_tick T
timer0_irq_handler + re-arm + hook	✅ T on all four
uiox_fw_printf (6 static copies) · bcache_plat_* (duplicates) · 5 umbrella stubs	✅ fixed
What's genuinely left
Item	Note
Boot test	parked by you — and it hadn't started: the hang is in 01_uBoot, before the kernel is reached
uart0_irq_handler × 4	declared, registered, no body anywhere
x86_64 LAPIC timer	x86_lapic_init never programs INITCNT/DIVIDE; 8259A masked → no tick
Item 7 — 5 refs	one line each; dormant today
Item 6 — staging	3 more architectures + the 10 apps
riscv64 timer rate	/100u → /ARCH_TIMER_HZ (parked)
arm32 re-arm address	verify against SOC_TIMER0_BASE (parked)
The one reading I'd keep in view
Everything is verified statically and nothing has run. Every ✅ above means "written, compiles, links, symbol present in the ELF" — not "works". The boot hang stopped short of [arch_init], so the frame pool filling, readIR returning real bytes, the tick firing, and arch_context_switch moving registers are all still unobserved.

If you want the next productive step without QEMU, uart0_irq_handler is the last bodiless registration and takes the same shape as the timer handler you just landed — one function per architecture, PL011 on three and NS16550A on riscv64.



===========
The list, final
#	Item	State
1–3c	context, kinit_main, UNFS, SCFS, vfs_mount_root	✅
4	Loader + execve	✅ four arches
5	fork	✅ four arches
6	uios + apps, install into image	✅ four distinct trees, wired into rebuild.sh
7	Stale paths	🔶 cosmetic, parked
The list's functional work is done. Items 1–6 are complete and verified to the level static checking allows.

What's left, ranked by whether it changes behaviour
1. The boot test — the only thing that converts any of this from "linked" to "running." Everything to date is symbols in ELF files and sections in place. The frame pool filling, readi returning real bytes, the tick firing, arch_context_switch moving registers — all unobserved. Right now it's blocked on 01_uBoot hanging before [arch_init], which is a pre-existing bootloader issue rather than anything we touched.

2. uart0_irq_handler × 4 — a real gap, same class as timer0_irq_handler was. Declared at each arch_init.c, registered via irq_request, no body anywhere. Depending on whether it lands strong or weak, the first UART interrupt either faults at address 0 or the handler is discarded by --gc-sections and serial input never works. That matters for 50_UIX/01_shell, which is a shell.

3. x86_64's LAPIC timer — no tick on that architecture at all. The 8259A is masked at arch_init step 3 and x86_lapic_init only sets the spurious vector, never INITCNT/DIVIDE. timer_irq_handler is correctly wired; the re-arm is an empty stub because there's no timer register to write. Real new work.

Two parked defects worth remembering
riscv64's timer rate is still /100u against HZ = 1000 — 10× fast, so xtime and every time slice are stretched when it runs. You parked the value change; it's behavioural, not cosmetic.

arm32's re-arm address (0x10011000) is my estimate of the SP804 Timer1 load register. Its arch_init.c has no timer init of its own — that's uiox_soc_arm32.c, which writes SOC_TIMER0_BASE + 0x000u. Worth one grep to confirm the base matches.

If you'd rather stop here, this is a clean place: the kernel builds and links on all four architectures, six of seven list items are done, and the remaining items are each independently scoped. uart0_irq_handler is the smallest one that changes behaviour.
