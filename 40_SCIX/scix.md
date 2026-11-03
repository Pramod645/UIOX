system call dispatcher in 40_SCIX based on BSD numberig and write dispacher for both 32_FS[10_scfs] and 33_PCS[50_scpcs]


all three dispatchers. Let me confirm the exact function names each must route to.


Used Workspace
write them against the BSD numbering from uix_sys.h and the entry points I've confirmed.


Used Workspace
All three dispatchers are written.

The three files
40_SystemCallInterface/uix_archSysCall.c — 163 lines. The thin layer's own dispatcher. Receives a number and six argument registers from the arch trap handler, decides which subsystem owns the number, and hands off. Contains no process logic and no file logic.

33_PCS/50_scpcs/src/uiox_kix_scpcs_dispatch.c — 235 lines. Routes process numbers to the 24 uiox_kix_scpcs_* entry points.

32_FileSystem/10_scfs/uiox_kix_scfs_dispatch.c — 267 lines. Routes file numbers to the fs_* entry points.

How the split works


uix_archSysCall.c        knows "122 belongs to process control"
    ├── scpcs_dispatch   knows "122 is kill"
    └── scfs_dispatch    knows "3 is read"
Each layer's table stays small, and neither has to learn the other's. A new process call is one row in scpcs_dispatch plus one case in SCiX's scpcs_owns().

The numbering, as you asked
BSD's, taken as given — SYS_EXIT 1, SYS_FORK 2, SYS_READ 3, SYS_GETPID 20, SYS_CLOCK_GETTIME 87, SYS_KILL 122, SYS_EXECV 250. Rows are in numeric order, not grouped, because BSD spreads each family across the space and a reader looking up 128 should find rename where 128 lives.

My earlier uiox_kix_scpcs_nr.h is superseded — it renumbered 1–24 in Bach's grouping, which conflicts with this. Delete it; numbers now come from uix_sys.h.

Three distinctions I preserved
Code	Meaning
EINVAL	not this subsystem's number — a routing bug
ENOSYS	a real BSD syscall with no implementation yet
EFAULT	the call ran and rejected a user pointer
SYS_KQUEUE is a genuine syscall this kernel lacks, so it returns ENOSYS. Telling a caller its number is invalid would send it looking for a bug in its own table.

What I could not verify — three things
The fs_* function names are guesses. I've never seen 10_scfs/include/fs.h. I invented uiox_kix_scfs_read, _write, _open and 30 more, following the naming convention. Every one of those rows will fail to compile if your header spells them differently — which is actually the useful failure mode, since the error names the function rather than the number. But it will not link until those names are corrected against the real header.

I have not compiled any of this. No compiler here.

The uix_sys.h constants I reference may not all exist. I used SYS_BREAK 17, SYS_EXECV 250, SYS_SETPRIORITY 96, SYS_TIMES, SYS_GETPPID, SYS_ALARM, SYS_GETPGID, SYS_SIGSUSPEND — some of these I did not see in the excerpt you pasted (which stopped at SYS_MQ_UNLINK 334). If a constant is absent, that row fails to compile by name.

The regs write-back
Both subsystem dispatchers write the same convention: positive error number in rc_r0 with rc_carry set. I also gave scfs_dispatch a local mirror struct of the register layout rather than including 40_psa's header — so the file system does not depend on the process layer's types. Worth flagging because it means the layout is now written down in two places, and a change to uiox_kix_psa_reg_context_t must be mirrored there.


uix_archSysCall.c

Code · C

uiox_kix_scpcs_dispatch.c

Code · C

uiox_kix_scfs_dispatch.c


==========

is this two files uixarchSysCall.h and uixsys.h also needed i thin layer along with new three file generated?


Yes — both are needed, and neither is optional. Here's why each exists.

uix_sys.h — the number source
My uix_archSysCall.c does #include "uix_sys.h" because that's where the BSD constants live. Every case SYS_READ: and every [SYS_KILL] = ... depends on it. Delete it and nothing compiles: SYS_KILL would be an undeclared identifier.

So it's not a peer of the three files I wrote — it's an input to them. Same for scpcs_dispatch.c and scfs_dispatch.c, both of which include it.

uix_archSysCall.h — the contract
This one is subtler, and I owe you a correction. My uix_archSysCall.c includes it and declares:

c


int64_t uix_arch_syscall(uiox_uint64_t nr, ...  void *regs);
I invented that signature. I never read uix_archSysCall.h — the three URLs I tried all failed. So the header exists in your repo, and my .c was written without it. Two possibilities:

The header already declares uix_arch_syscall with that exact signature — then it's needed, and everything lines up.
It declares something else — a different name, a different arity, or a typedef the trap handler uses — and my .c defines a function nobody calls, while the real entry point stays unimplemented.
The second is more likely than I'd like. A file named uix_archSysCall.h in the interface layer almost certainly holds the arch-facing declaration, and I guessed at it rather than reading it. That's the same mistake shape as the fs_* names — writing against an invented contract and finding out at compile time.

So the honest answer to your question
File	Needed?	Why
uix_sys.h	Yes, certainly	source of the BSD numbers — all three new files include it
uix_archSysCall.h	Yes	the declaration my .c must match — and mine may not
Plus two others already in that folder that I've never read: README.md and SystemInterface.rst. The .rst is the design document for this layer, and it would settle the entry point's signature, the SYS_CALL_ENBLE_DISABLE behaviour, and whether SCiX is kernel-side or userspace-side — three things I've been assuming.

What would settle it
Paste uix_archSysCall.h and SystemInterface.rst. With those two I can:

Fix my uix_arch_syscall to the real signature, or confirm it
Correct the scfs_* names against fs.h if you send that too
Replace the invented row list with one that actually links
Until then, treat uix_archSysCall.c as a shape rather than a working file — the routing logic is sound, but the symbol it defines has not been checked against the header it includes.

One thing I can offer meanwhile: I have uix_sys.h in full from your earlier paste, so I could verify that every SYS_* constant my three tables reference actually appears in it — catching the "constant doesn't exist" failures before you compile. Say the word and I'll run that check.

=====================================

uix_archSysCall.h  this attached file needed but earlier all the code was diabled?


Yes, it's needed — and the #if 0 is the reason, not a reason to discard it.

What the file is
uix_archSysCall.h is the userspace-side half of the syscall mechanism. It defines my_syscall0 … my_syscall6: inline-asm macros that put the syscall number in x8, the arguments in x0–x5, and execute svc #0. That's how a program in 50_UIX makes a syscall.

So it pairs with my uix_archSysCall.c, which is the kernel-side half — what runs after the trap. Different directions:



50_UIX program
   my_syscall2(SYS_KILL, pid, sig)     ← this header
        svc #0  →  CPU traps
   uix_arch_syscall(...)               ← my .c
Why #if 0 doesn't mean "unused"
The guard covers the whole body:

c


#if 0 // ARM64,ARM32 and x84_64
... my_syscall0 ... my_syscall6 ...
#endif
That's a temporary switch, not a deletion. Three things say so:

The comment names the arches it's for — ARM64, ARM32 and x86_64. Whoever wrote it disabled the block while it wasn't yet correct or wanted, and left the intent behind.

The file is a header with an include guard (__ARCH_SYS_CALL__H). A header nobody includes wouldn't need one. It's written to be included and is awaiting the flip.

A #if 0 is the standard way to park code that must not vanish. Deleting it loses the register conventions — x8 for the number, x0–x5 for arguments, return in x0 — which are the exact values the kernel-side dispatcher has to agree with.

And it settles something I'd flagged as unverified
Two messages ago I said I'd invented the signature uix_arch_syscall(nr, a0..a5, regs) and couldn't check it. This file confirms the calling convention the arch layer produces:

ARM64
syscall number	x8
arguments	x0, x1, x2, x3, x4, x5
return value	x0
trap instruction	svc #0
That's exactly the a0..a5 shape my three dispatchers take, and it confirms six arguments is the ceiling — which is why the table is capped at six and not more. So the shape is right.

What it does not confirm is the C-level symbol name or the regs parameter, because those macros never call a C function — they go straight to svc. The uix_arch_syscall declaration your trap handler uses is still something I haven't seen.

One thing worth noting about the return
The ARM64 comments say "syscall return comes in x0" and "the arguments are cast to long". My dispatchers write rc_r0 and rc_r1 — a 32-bit pair — because uiox_kix_psa_reg_context_t is defined that way. On ARM64 a long is 64-bit and the return arrives in a single x0.

So there's a mismatch to resolve when the block is enabled:

64-bit return in one register (what ARM64 wants) versus
split across rc_r0/rc_r1 (what my reg_context_t carries)
The split works — it's how a 32-bit ABI carries a 64-bit value — but on a 64-bit target you'd store the whole value in rc_r0 and leave rc_r1 clear, or widen the context type. Worth deciding before this block is turned on, because it's the kind of thing that produces a correct-looking 32-bit result that silently truncates.

So: keep it
Along with uix_sys.h (the numbers) and the entry-point declaration your trap handler uses. The three .c files I wrote are the kernel side; this header is the userspace side of the same boundary, parked until you enable it.

If you want, I can check the ARM32 and x86_64 conventions in the same file — the comment mentions all three arches, but the pasted excerpt only shows the ARM64 block. That would tell me whether x8/svc is the only shape the context type has to serve, or one of three.

===============================
That last one is a real hole, not a style question. A userspace gettimeofday() would hand the kernel an arbitrary address and the kernel would write to it. The gate exists, but this path skips it.

Two ways to close it, and I'd want your pick:

A. The adapter calls uiox_kix_scpcs_check_user_ptr(a0, sizeof(TimeVal), 8) itself, returning EFAULT on failure. Then every timing row is self-guarding and the order of calls stops mattering.

B. SCiX does the check before dispatching to any subsystem, so no wrapper needs to. That's cleaner layering but a bigger change to the thin layer.

My own preference is A — it keeps the guarantee local to the call that writes the pointer, and it matches how the rest of 50_scpcs already behaves.
-------------------


Every architecture traps, with its own instruction:

Arch	Trap	Count
arm64	svc	12
arm32	svc	11
riscv64	ecall	12
x86_64	syscall	11
arm64's earlier count was 12, and it still is. The other three are 11–12 — the small spread is the uios_syscall_selftest you added, which only arm64 and riscv64 compiled in identically, plus inlining differences per backend. All four are non-zero, which is what matters: each per-arch register set (x8, r7, a7, rax) assembles to a real trap instruction.

The userspace syscall layer is done
Item	State
uix_sys.h	80 of 83 wrappers trap; numbers from your own table
uix_archSysCall.h	four architectures, #else/#error guard
Four-arch image	builds and stages, distinct sizes per arch
Router	moved to 30_KIX/33_PCS/50_scpcs/src/, in the Makefile glob
EL0→EL1 path	verified at the instruction level on all four
What's left is the kernel half
Three things, in order:

1. The arch forwards. Each arch_runtime.c needs to call uix_arch_syscall from its trap entry, so a trapped number reaches the router instead of dead-ending. The four edit files from earlier are in this workspace as outputs/scix_clean/arch_runtime_*_EDIT.txt.

2. Kernel relink. The router now compiles into libpcs.a via the wildcard glob, so the link should pick it up — but nothing references uix_arch_syscall yet, which means the linker may drop it.

3. Boot test. Timer ticks, scheduler dispatches, a userspace binary execs and issues a syscall that reaches a dispatcher. That's the only thing that shows any of this runs.

Two loose ends I'd close first
The bare #elif at uix_sys.h lines 716 and 728, in the two shm stubs. They compile only because SYS_CALL_ENBLE_DISABLE is 0; a -D…=1 build would hit a syntax error.

sys_shm_open and sys_shm_unlink still have no syscall number on either side, and both are in your apps' caller list. That's a kernel-side decision — assign numbers and dispatcher entries, then convert the two wrappers.

And one cleanup you can now do safely
Delete the editor backups once you're satisfied: 40_SCIX/uix_sys.h.bak, .bak2, and uix_archSysCall.h.bak. Keep them until the boot test passes, in case you need to compare.

The trap mechanism works on all four architectures. The remaining question is no longer "does userspace reach svc" — it's "does the kernel do anything useful when it arrives."
-----------------------
now use to kernal system call establisged.

ok now out of 4 two 2 flow 1. from userspace to till 40_scix and 2. from scix to kernel establishd right? can you explanin how ssystem call from from user space to till kernel call ?


You've got the split right, with one correction: it's two halves, but four hops, and only three of them are wired.

The path, hop by hop
Hop 1 — userspace wrapper → trap. A program in 50_UIX calls sys_getpid(). In 40_SCIX/uix_sys.h that wrapper is now one line:

c


static inline uix_pid_t sys_getpid(void)
{
    return (uix_pid_t)my_syscall0(SYS_GETPID);
}
my_syscall0 is a macro in 40_SCIX/uix_archSysCall.h that pins the number into x8 and the args into x0-x5, then executes svc #0. EL0 ends here. Verified in the disassembly:



400074:  mov  x8, #0x14      ← 20 = SYS_GETPID
40007c:  svc  #0x0           ← the trap
Hop 2 — hardware → EL1 trap entry. svc raises an exception. The CPU vectors to arch_irq.S, which builds an hw_context_t from the interrupted registers, switches to the EL1 stack, and calls arch_syscall_dispatch(nr, a0..a5) — the C forward in 10_BSP/10_Arch/arm64/src/arch_runtime.c.

Hop 3 — the arch forward → the router. arch_syscall_dispatch is four lines and its whole job is to hand off:

c


long arch_syscall_dispatch(unsigned long nr, unsigned long a0, ...)
{
    return syscall_dispatch(nr, a0, a1, a2, a3, a4, a5);
}
This was the break. syscall_dispatch used to resolve to the weak stub in 10_BSP/src/uiox_bsp_stubs.c returning -1L, so every syscall died there. Now 30_KIX/common/uiox_kix_router.c defines it strongly and the weak one is overridden — which is what nm confirmed:



T syscall_dispatch     ← strong, from the router
Hop 4 — the router → a subsystem dispatcher. syscall_dispatch in the router reads the number and decides which subsystem owns it:

c


if (scfs_owns(nr))                                       /* filesystem numbers */
    return uiox_kix_scfs_dispatch(nr, a0, ..., regs);

if (scpcs_owns(nr) || scix_time_owns(nr) || scix_ipc_owns(nr))
    return uiox_kix_scpcs_dispatch(nr, a0, ..., regs);    /* process numbers */

return SCIX_ENOSYS;    /* a real BSD number this kernel has no implementation for */
For SYS_GETPID (20), scpcs_owns matches, so it reaches uiox_kix_scpcs_dispatch, whose table entry is:

c


[SYS_GETPID] = { scpcs_call_get_pid, "getpid" },
That adapter calls uiox_kix_scpcs_get_pid(), which returns p->p_pid from the current process. The value comes back up the same path — return register x0, restored from the saved context, and svc returns into userspace with the pid.

Which of your four are established
Hop	Layer	State
1	50_UIX → 40_SCIX (wrappers → svc)	✅ 80 wrappers, 12 svc on arm64
2	svc → EL1 trap entry	✅ 12 svc counted; vector code in 10_BSP/10_Arch
3	arch forward → router	✅ T syscall_dispatch — the weak stub overridden
4	router → subsystem dispatcher	✅ T uiox_kix_scfs_dispatch, T uiox_kix_scpcs_dispatch
So all four link. What separates them is verification: hops 1 and 2 are proven at the instruction level — the svc is in the binary and the wrapper arities are right. Hops 3 and 4 are proven only as symbols present in one image. A T symbol means it links and could be called; it does not mean the trap vectors to it, or that the number reaches the right table slot.

That's the honest gap. There are three places it could still fail at runtime:

The trap may not vector where the router expects. arch_irq.S needs to call arch_syscall_dispatch with the register frame filled in. If the vector entry isn't installed or reads the wrong frame slots, the svc goes somewhere else entirely.

The number space may disagree. Userspace uses SYS_GETPID 20 from uix_sys.h; the kernel's uiox_kix_scpcs_numbers.h also says 20 — those two I checked. But uix_sys.h defines some names twice with different values (the kernel's own header notes SYS_LSTAT 40 vs 107, SYS_FSTAT 53 vs 108, and ~35 more), and the later #define wins silently. A wrapper compiled against one value could trap a number the router routes elsewhere.

regs is NULL on this path. The forward passes (void *)0, documented as skipping the dispatchers' write-back — the return is expected to land in x0 via the saved frame instead. If the trap entry doesn't do that, the value never reaches userspace.

The one thing that settles all three
A boot test. Everything above is what the code says; none of it has executed a syscall. The narrowest version is a userspace sys_getpid() whose return value you can see — if it comes back as a plausible pid rather than -1, all four hops ran. If it's -1, the ENOSYS path or the stub was taken and the break is at hop 3 or 4.

The selftest you added earlier (uios_syscall_selftest) is the cheapest probe: it executes my_syscall0(SYS_GETPID) and stores the result to memory. If you can read that location after boot, you've answered it.

===========================
 now kernel to userspace.

 ok good thanks. can now establsish the path from kernel to userspace as leftover half. in two steps 1. kernel call from kernel to 40_scix and 4. scis to os or how how kernel want back communivation from kernel to usespace , can we go in that in two step fist close kenel boundery and then user space boundry?


Yes — that's exactly the right split, and the return path is a genuinely separate half of the work. Let me lay out what each half needs before we touch anything.

The return path, hop by hop
Return hop 3 — dispatcher back to the router. Each subsystem dispatcher returns an int64_t. The router passes it straight up:

c


if (scpcs_owns(nr) || ...)
    return uiox_kix_scpcs_dispatch(nr, a0, a1, a2, a3, a4, a5, regs);
Nothing to do here — it's already a plain return. But there's a convention to settle: the dispatchers share a "positive error number in rc_r0 with rc_carry set" scheme for the uiox_kix_psa_reg_context_t frame, which is not the frame the SVC vector builds. The router's own comment says it does not reinterpret one subsystem's error as another's — so the value passes through raw.

Return hop 2 — router back to the arch forward. arch_syscall_dispatch returns long. Also already a plain return. The question is what it means: a negative int64_t from the router, or a BSD style -errno? Userspace wrappers expect -1 with errno set in the real ABI; this tree returns the raw value and lets the wrapper interpret it.

Return hop 1 — EL1 back to EL0. This is the one that is not wired, and it's the real work:

c


return uix_arch_syscall((uiox_uint64_t)nr, ..., (void *)0);
                                                 ↑ regs is NULL
The arch forward passes NULL for regs, and the dispatchers document that a NULL regs skips their write-back. So the return value exists only in x0 in C — it has to be placed into the saved hw_context_t so that when arch_irq.S executes RESTORE_REGS and eret, EL0 sees it.

Your earlier note on arch_irq.S says it already does this:



str x0, [sp, #OFF_X0]     ← into the saved hw_context_t, before RESTORE_REGS
If that store is there for the SVC vector path, return hop 1 works and the value reaches userspace. If the store is in the IRQ path but not the SVC path, the value is computed and discarded — userspace gets whatever was in x0 before the trap.

The two steps you proposed
Step 1 — close the kernel boundary. That's hops 3 and 2 above: confirm the return convention end to end within the kernel. Concretely, the things to check are:

sh


cd /Users/pramodkumar/Hack/WS/UIOX && grep -nE 'return|rc_r0|rc_carry' 30_KIX/common/uiox_kix_router.c | sed -n '1,20p'
and what the dispatchers do with regs when it's non-NULL versus NULL:

sh


cd /Users/pramodkumar/Hack/WS/UIOX && grep -nE 'regs|rc_r0|rc_carry' 30_KIX/33_PCS/50_scpcs/src/uiox_kix_scpcs_dispatch.c | head -20
Step 2 — close the userspace boundary. That's hop 1: read arch_irq.S's SVC vector and confirm the return lands in the saved frame. That's the file that decides it:

sh


cd /Users/pramodkumar/Hack/WS/UIOX && grep -nE 'OFF_X0|arch_syscall_dispatch|RESTORE_REGS|eret|svc' 10_BSP/10_Arch/arm64/src/arch_irq.S | head -30
The design question underneath both
There are three different frame layouts in this tree, and the return path has to be clear about which one it's writing:

Frame	Where	Used by
hw_context_t	built by arch_irq.S on the SVC vector	the trap entry
uiox_kix_psa_reg_context_t	40_psa, carries rc_r0/rc_carry	the dispatchers' write-back
scps_xtime_t-style local structs	per-wrapper	argument marshalling only
The router passes regs as void * precisely because it must not depend on 40_psa's layout. So the honest design is: the arch layer places the return in its own hw_context_t, and never passes a regs pointer — which is what the current (void *)0 does, provided arch_irq.S does the store.

Which to start with
Start with hop 1 — the arch_irq.S grep above — because it's the only hop whose status is unknown. Hops 2 and 3 are plain C returns that the compiler already handles; if the value is correct at the router, it's correct at the forward.

If arch_irq.S stores x0 into the saved frame after calling arch_syscall_dispatch, then the return path is complete too and all that's left is the boot test. If it doesn't, that's a small assembly edit — one str — and it's the last piece.

Paste that arch_irq.S grep and I'll tell you which of the two it is.
=======================================================================

The return path is complete. arch_irq.S already does the one thing that mattered:



108: .global arm64_svc_entry
110: arm64_svc_entry:
121:     bl      arch_syscall_dispatch
122:     str     x0,  [sp, #OFF_X0]   /* write return value back to frame  */
124:     RESTORE_REGS
125:     eret
137:     .align 7; b arm64_svc_entry     /* Synchronous (SVC) */
142:     .align 7; b arm64_svc_entry     /* Synchronous (SVC from EL0) */
So the sequence is right, and it answers the question you were asking about how the kernel gets back to userspace:

bl arch_syscall_dispatch — C computes a return value in x0
str x0, [sp, #OFF_X0] — it's stored into the saved hw_context_t, which is why the router can pass regs = NULL without losing the result
RESTORE_REGS — reloads every register from that frame, x0 included
eret — returns to EL0
And lines 137/142 show the same entry wired for both the AArch64 synchronous vector and the EL0 SVC vector, so a trap from a user process lands there.

Both halves, now verified
Hop	Direction	State	Evidence
1	wrapper → svc	✅	mov x8, #0x14 / svc #0 in the disassembly
2	svc → EL1 entry	✅	arm64_svc_entry, vector slots 137/142
3	entry → router	✅	T syscall_dispatch, weak stub overridden
4	router → dispatcher	✅	T uiox_kix_scfs_dispatch, T uiox_kix_scpcs_dispatch
4r	dispatcher → router	✅	plain return, no conversion
3r	router → forward	✅	plain return
1r	forward → EL0	✅	str x0, [sp, #OFF_X0] before RESTORE_REGS
The outbound and return halves match at every hop — one symbol pair, one register convention (x8 in, x0 out), and the regs = NULL choice is now explained rather than merely tolerated: the arch layer places the return itself, so no dispatcher write-back is needed.

The one thing still unknown
str x0, [sp, #OFF_X0] writes to the frame at OFF_X0 = 0, which arch_irq.S defines. What matters is whether that offset matches where RESTORE_REGS reads x0 from in the struct it reloads. If they agree, the value lands in userspace's x0. A mismatch would silently put the return in the wrong register — plausible-looking, wrong result.

sh


cd /Users/pramodkumar/Hack/WS/UIOX && sed -n '1,60p' 10_BSP/10_Arch/arm64/src/arch_irq.S
That shows OFF_X0 and the RESTORE_REGS macro's reload list. If both use the same offset, the chain is closed end to end and the boot test is the only thing left.

What's actually established now
Every hop is statically verified — the instructions exist, the symbols link, the register convention is consistent. What remains is runtime: whether a sys_getpid() executed at EL0 comes back with a plausible pid. That's the boot test, and this is the first point where it's the only open question rather than one of several.

===========================================================================
What remains
The boot test, and nothing else in the syscall path. Every instruction, symbol and offset is present and self-consistent on all four architectures; no syscall has executed anywhere. A sys_getpid() at EL0 returning a plausible pid — rather than -1, or the pre-trap value of the return register — is what turns this from "the code is right" into "the value arrives."

The uios_syscall_selftest you added is the cheapest probe: it runs my_syscall0(SYS_GETPID) and stores the result. Three outcomes tell you where you stand:

a small positive pid → the whole chain ran
-1 → the ENOSYS path or the weak stub; the break is at the router or a dispatcher table entry
the constant 1 → sys_getpid's stub branch returned it, meaning the wrapper edit didn't take
Two tidy-ups while the tree is green: the editor backups (40_SCIX/uix_sys.h.bak, .bak2, 40_SCIX/uix_archSysCall.h.bak), and the doubled region.h/context.h pair in brk.c's include block.


-----
-============
========================================================
how image lod by kernel????


ok good, this we can stop here. now another question but it repeatative foyou as above already explain but i wantr in step wise step. can you explain how kernel load the userspave image to excute it? and another question it good iead to keep one image for user space to multiple images from user space. please explanin in details and step wise step?


Two separate questions. I'll take them in order, and I'll be explicit about which parts I've seen in your tree and which are the general shape a boot path has.

Part 1 — How the kernel loads and runs the userspace image
Step 1 — The image is a file, staged on disk. rebuild_uix.sh produces, per arch:



50_UIX/20_uios/build/arm64/image/bin/uios
50_UIX/21_apps/build/arm64/image/21_apps_/app_*
These are plain files in the build tree, not on a device yet. Getting them onto real storage is the UNFS format — a separate step.

Step 2 — The kernel mounts a filesystem that holds them. 32_FS provides this. 10_scfs is the syscall-facing layer; 10_unfs and 01_fsa underneath. kinit_main mounts the root, so the paths uios's g_apps[] names (/../21_apps_/app_shell) resolve.

Step 3 — kinit_main execs uios. From 30_KIX/common/uiox_kernel_main.c. It calls the same path a userspace execve would: uiox_kix_scpcs_execve(path, argv, envp).

Step 4 — execve reads the ELF and validates it. The kernel opens the file, reads the ELF header, checks magic and machine type, and reads the program headers. Rejecting a bad header here is what stops a corrupt image from reaching userspace.

Step 5 — It builds the address space. For each PT_LOAD segment it allocates a region and maps it:

uiox_kix_psa_allocreg() — takes a free region
uiox_kix_psa_attachreg() — attaches it to the process at a virtual address
uiox_kix_psa_loadreg() — loads the segment contents
arch_mmu_map() — installs the page-table entries
One thing worth knowing: uiox_kix_psa_loadreg()'s own header says it marks the region LOADING and stops — it does not mark pages VALID and does not claim to have read anything, because no readi()-equivalent is wired from 01_fsa yet. So the loader is honest about being incomplete there.

Step 6 — It sets up the initial context. The entry point from the ELF header goes into the saved elr_el1 slot; the stack pointer goes into sp_el0; the target exception level is set to EL0 in spsr_el1. That frame is exactly the hw_context_t arch_irq.S builds — which is why the return path works the way it does.

Step 7 — It hands the process to the scheduler. uiox_kix_psa_sched_enqueue() puts it on the ready queue; uiox_kix_psa_proc_set_state() moves it to READY.

Step 8 — The first context switch drops to EL0. The scheduler's switch restores that frame — RESTORE_REGS plus eret on arm64 — and the CPU is now executing uios at EL0. No svc has happened yet; this is just the exception return into user mode.

Step 9 — uios runs and reaches its first syscall. main → uios_init → uios_launch_apps → sys_fork(). That's the trap path we spent this session on: svc #0 → arm64_svc_entry → arch_syscall_dispatch → syscall_dispatch (the router) → uiox_kix_scpcs_dispatch → the handler.

Step 10 — The child execs an app, and the cycle repeats. Conditional on pid == 0, the child calls sys_execve(g_apps[i].ua_path, argv, 0) — steps 4–8 run again for app_shell, app_logger, and the rest.

So the load is invoked from two places: once by kinit_main for uios, then ten more times by uios itself via sys_execve.

What I've actually verified versus what's the general shape: steps 3–10 are read from your tree — execve, allocreg/attachreg/loadreg, arch_mmu_map, the scheduler calls, and the syscall path all appear in files you've shown me. Step 2 (the mount) I'm describing from the layer map, not from reading kinit_main's body. Step 1 is verified output from rebuild_uix.sh.

Part 2 — One userspace image, or several?
Several, and they're mostly already several. The distinction that matters isn't how many files you keep — it's how many independent ELF images and what each contains.

Today you have eleven. uios plus ten app_* binaries, each a separate PT_LOAD executable with its own main. That's already the multi-image design. The evidence is in your own g_apps[]:

c


{ "shell",   "/../21_apps_/app_shell", 0, 0, -1, 0 },
{ "logger",  "/../21_apps_/app_logger", 0, 0, -1, 0 },
...
Ten paths, ten execs, ten processes.

Why separate binaries are usually the right call:

Isolation. A fault in app_netserver kills that process. If all ten were one binary, one bad pointer takes the set down.
Heap. brk and region growth are per-process. One image means all ten share one data segment.
Fault-in. execve loads a binary when it's needed. A monolithic image loads every app's data even if only the shell runs.
Signals and exit codes. wait_pid reaps per-process. One image means one pid for everything.
The case for a single image is real but narrow: shared code stays in memory once (no per-app copy of the libc-ish helpers), and you skip ten execve calls at boot. On a 32-bit model with the sizes you have — uios at 75 KB, apps far smaller — that saving is small, and it isn't worth losing the isolation.

Where the middle ground actually belongs is shared libraries, not merged binaries. If the ten apps share a meaningful amount of code, that code goes in one shared object mapped into each process's address space — one copy in physical memory, ten mappings. But that needs dynamic linking, and your 20_uios/21_apps makefiles link -nostdlib -nostartfiles with fixed entry points. Static multi-binary is the right design at this stage.

Two things worth doing regardless:

Consistent staging. The image tree should be one directory per arch containing everything — bin/, 21_apps_/, and later /etc, /dev. rebuild_uix.sh already checks the per-arch path, which is what catches the "every arch wrote into arm64" bug you hit before.

The root filesystem is the real unit. kinit_main mounts a filesystem; the eleven binaries live inside it. Whether you keep one image or several is a question about what's on that filesystem, not about how many files the build produces.

One caution, and it's from this session: execve is the syscall that makes multi-image work, and the loader's loadreg still marks regions LOADING without claiming to have read anything. So the multi-image design is right; the loader that would make it fully work is one of the pieces still marked incomplete. That's worth fixing before you add a twelfth binary.
