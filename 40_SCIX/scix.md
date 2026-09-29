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
