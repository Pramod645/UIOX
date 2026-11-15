# SMP:Symmetric Multiprocessing.
# UIOX — Multiprocessor Design

What has to change to run UIOX on more than one core, and in what order. Every
claim cites a path in the tree; nothing is inferred from intent.

Companion to `DESIGN.md`. Read that first — this document assumes its seven
axes and the decisions they record.

Last reviewed: 2026-11-14.

---

## 0. The order, and why it cannot be reordered

    A. boot the kernel on ONE core              ← nothing has run yet
    B. task table + run queue for one core
    C. preemption (a timer tick, not just yield)
    D. per-CPU split        ← this is the actual SMP work
    E. locking discipline, written before any lock is added
    F. secondary core start-up
    G. IPIs
    H. load balancing

Steps D–H are all design that assumes B and C exist. You cannot reason about a
run queue on four cores before you have one run queue on one core, and you
cannot decide what to lock before you know what is shared.

**Do not design SMP yet.** The kernel has not booted. Everything below is
recorded so the decision lands against a written position, not because it is the
next thing to build.

---

## 1. What the tree already has, and why it matters

**The thread pointer is already in the switch frame.** This is the single most
important prerequisite for SMP, and it is present in all four arches:

| Arch | Register | In the frame at |
|---|---|---|
| arm64 | `tpidr_el0` | offset 120 of `arch_context_t` |
| riscv64 | `tp` | `14*8` |
| x86_64 | `fs_base` | offset 72 (read/written via MSR `0xC0000100`) |
| arm32 | — | *absent* |

`arch_context_switch` saves and restores it per context. A per-CPU base register
is how a core finds its own `struct cpu`, so the mechanism a multicore kernel
needs already exists on three of the four architectures.

**arm32 has no thread pointer in the frame.** Its layout is
`r4-r11, lr, sp, cpsr, pc` — 48 bytes, no `tp`. On ARMv7 there are two options:
`TPIDRURW` / `TPIDRURO` (CP15 coprocessor registers, available from ARMv6K), or
the older `R9` convention. Whichever it uses, the field has to be **added to the
struct and to the save/restore pair** before arm32 can support per-CPU state.

**`uiox_kix_psa_region.c` already has a lock bit.**
`UIOX_KIX_PSA_REG_LOCKED` is defined in `uiox_kix_psa_region.h` with the comment
*"the region is being modified; a concurrent fault must wait."* The tree has
started thinking about concurrency at the region level, which is the right level
to start at.

---

## 2. The per-CPU split

Every global in the tree, classified. **Per-CPU** means each core needs its own
copy. **Shared-locked** means one copy, with mutual exclusion. Getting this split
wrong is the classic SMP bug: a per-CPU copy of something that must be shared
gives you two versions of the truth.

### Per-CPU — one copy per core

| Global | Where | Why per-CPU |
|---|---|---|
| current task / thread pointer | `arch_context_t` offsets above | the core that is running a task must know which |
| idle task | not yet written | one per core is what a core runs when it has nothing else |
| run queue | not yet written | the alternative is a global queue behind a global lock, which is the thing SMP exists to avoid |
| u area (`uiox_kix_psa_u`) | `40_psa`, reached directly by `attachreg` at `region.c:270` | `u_pregs[]` is the **per-process** pregion table; one u area serving all cores means every core writes the same process's slots |
| kernel stack | `uiox_kix_psa_sys_context_t.sc_kstack[]` | a trap on core 2 cannot use core 1's stack |
| `u_error` | `uiox_kix_psa_u.u_error` (`region.c:317,321`) | it is per-syscall, so it is per-task, so it is per-CPU when one task runs per core |

The u-area row is the one that will bite first. `uiox_kix_psa_attachreg` reaches
`uiox_kix_psa_u.u_pregs[i]` **directly** — there is no context argument:

    uiox_kix_psa_pregion_t *prp = &uiox_kix_psa_u.u_pregs[i];       // region.c:270

That works on one core because there is one u area and one process. On two cores
it means two `fork`s racing for the same slot, and whichever writes last wins.
The fix is that the u area is per-*process*, not per-*CPU*, and must be reached
through the current task rather than through a global name.

### Shared-locked — one copy, with mutual exclusion

| Global | Where | Why shared |
|---|---|---|
| region table | `uiox_kix_psa_region_table[UIOX_KIX_PSA_NREGION]` (`region.c:49`) | a region is shared by reference count across processes; `dupreg` deliberately makes parent and child point at one |
| page-table pool | `s_pgtbl_pool[UIOX_KIX_PSA_NREGION][UIOX_KIX_PSA_MAX_PAGES]`, `s_pgtbl_used[]` (`region.c:57-58`) | ~768 KB of static BSS; duplicating it per core is 768 KB × cores |
| pool init flag | `s_pgtbl_ready` (`region.c:59`) | idempotent init — a second core racing it is harmless only because it re-zeroes; worth an explicit guard |
| physical frame allocator | `uiox_phys_alloc.c` — `phys_alloc_page()` | two cores allocating must not receive the same frame. This is the one that corrupts memory silently rather than crashing |
| bump allocator | `uiox_kix_psa_pmalloc()` (`region.c:147`) — *"the bump pointer is the whole state"* | a bare bump pointer with no lock hands the same block to two cores |
| buffer cache | `32_FS/00_bcache/` | shared by design; the whole point of a cache is that everyone reads the same blocks |
| inode cache | `32_FS/01_fsa/` — `iget` / `iput` | reference counts and hash queues, both read-modify-write |
| mount table | `32_FS/10_scfs/`, `uiox_fs_init` | read-mostly; needs a read lock at most |
| fd / file tables | `ufdfile[NOFILE]` (`uiox_kix_scfs.h`) | per-process, so reached through the task like the u area — shared *structure*, per-process *contents* |
| console | `uiox_console_write` | a console write from two cores interleaves mid-line. A short lock is enough; this is a correctness-of-output issue, not a memory one |

### Needs no protection

| Global | Why |
|---|---|
| `early_puts` before secondaries start | single-threaded by construction |
| `uiox_kix_scpcs_numbers.h` and the other `#define` tables | constants |
| the dispatch tables (`uiox_kix_scpcs_dispatch.c`, `scfs_owns`) | read-only after init |
| `_vector_table`, `VBAR_EL1` | installed once per core, then read-only |

---

## 3. The locking discipline

Decided before any lock is written, because an order that exists only in
people's heads is an order that will be violated.

Three approaches, and the honest answer for a first SMP UIOX:

| Approach | Cost | Where it is the right answer |
|---|---|---|
| **One big lock** | no scalability, trivially correct | the first working version — it makes SMP a correctness exercise rather than a concurrency one |
| **Per-subsystem locks** | ordering matters, real throughput | once the big lock is proven and you know which subsystems contend |
| **Lock-free** | fastest, hardest to prove | specific structures only, never as a policy |

Recommendation: **one big lock first.** It gets four cores running with the
existing code unchanged, then each subsystem that shows up in a profile gets its
own lock. This is what Linux did and it is the order that produces a working
system rather than a heroic one.

**Written before the lock:** the order locks are taken in, and the rule that no
path takes them in two orders. One page. Cheap now, impossible later.

**Also decided here:** whether interrupts are disabled while a lock is held.
On arm64 that is `DAIF`'s `I` bit; the arch files already manipulate `daif`
(`arch_irq_save` / `arch_irq_restore` exist in `arch_runtime.c`). A lock taken
without masking interrupts can deadlock against a handler on the same core.

---

## 4. Secondary core start-up

Arch-specific, and small if the boot path is clean. Each secondary needs: its own
stack, its own `struct cpu`, its own idle task, and then it joins the scheduler.

| Arch | Mechanism |
|---|---|
| arm64 | PSCI `CPU_ON`; secondaries arrive in a spin loop reading a mailbox entry |
| riscv64 | SBI `HART_START` |
| arm32 | the SMP bit in the auxiliary control register, then `sev` |
| x86_64 | INIT-SIPI-SIPI; the AP trampoline must be 4K-aligned and below 1 MB |

Two things this depends on:

**A per-CPU base register must be set before any C code runs on the secondary.**
Otherwise the first per-CPU access reads core 0's state. On arm64 that is
`msr tpidr_el1`, and it is the first instruction in the secondary's entry.

**`VBAR_EL1` is per-core.** Each secondary installs its own vector table base —
the table itself is shared read-only, the register is not.

---

## 5. IPIs — what makes it a system rather than N independent cores

Without inter-processor interrupts, cores cannot reschedule each other, shoot
down TLB entries, or deliver a signal to a task on another core.

| Arch | Mechanism |
|---|---|
| arm64 | `SGI` via the GIC |
| riscv64 | no IPI in the base ISA — the software-interrupt register in the CLINT |
| arm32 | `SGI` via the GIC |
| x86_64 | the local APIC's ICR |

**TLB shootdown is the one that bites first, and it is a security hole rather
than a performance bug.** If core A unmaps a page and core B's TLB still has it,
B can execute unmapped memory. `uiox_kix_page_fault.c` and the region detach
paths (`uiox_kix_psa_detachreg`, `uiox_kix_psa_freereg` — *"frees the page table"*)
are both places where a mapping is removed, so both need a shootdown.

---

## 6. What the userspace half needs — which is little

This follows from `DESIGN.md`'s axis 1: mechanism in the kernel, policy in
userspace. Concurrency is mechanism, so almost nothing in `50_UIX` changes.

| File | Change |
|---|---|
| `uios_apps.c` | **one new column** — CPU affinity. The header's own rule applies: *"Keep this file free of logic"* — a column, not a branch |
| `uix_uios.c` | `apply_policy()` already calls `sys_sched_setscheduler(pid, policy, priority)`; affinity is one more argument |
| the shell | none |
| `21_apps` | none |

`sys_fork` does not change shape. The child may land on another core, which is
the point.

---

## 7. What would break first, in practice

Ordered by how soon each would show up on a four-core boot:

1. **The u area.** `attachreg` writes `uiox_kix_psa_u.u_pregs[i]` with no
   context argument. Two forks racing give one process another's regions.
   Correct on one core, wrong the moment there are two.
2. **The physical frame allocator.** `phys_alloc_page()` handing the same frame
   to two cores is silent memory corruption — no fault, no message, just two
   tasks writing one page.
3. **The console.** Two cores writing interleave mid-line, so the boot log
   becomes unreadable exactly when you need it.
4. **The bump allocator.** `pmalloc`'s *"bump pointer is the whole state"* hands
   the same block twice.
5. **TLB staleness.** Manifests as a fault in code that was just unmapped, or as
   nothing at all — the worst kind.

Item 1 is first because it is the one that is already structurally wrong rather
than merely unprotected: it has no context parameter to pass a task through.

---

## 8. The decision to make now, while it is cheap

**Should `attachreg` take a context?** It currently reaches a global u area by
name. Making it take the current task as a parameter is a signature change in
`40_psa` — small today, with one caller in `uiox_kix_psa_region.c` and a handful
in the loader — and much larger once there are fifty callers.

That single question is the difference between "the tree is SMP-ready in shape"
and "SMP means re-plumbing the region layer."
====================================

Symmetric Multiprocessing.

Two words matter in that expansion:

"Symmetric" — every core is equal. Any core can run any task, take any interrupt, and enter the kernel. The opposite is asymmetric multiprocessing (AMP), where one core is the boss and the others are workers doing what they're told — a design you see in embedded systems with a control core and a DSP, and in the older "master/slave" kernels.

"Multiprocessing" — more than one processor executing at once, as opposed to uniprocessing, which is what UIOX does today.

Where the term sits among its neighbours
Term	Meaning
SMP	Symmetric Multiprocessing — cores are peers, one kernel image, shared memory
AMP	Asymmetric — cores have different roles, often different OS images
SMT	Simultaneous Multithreading — one core presenting two hardware threads (Intel Hyper-Threading). Not more cores, more contexts per core
NUMA	Non-Uniform Memory Access — SMP where cores reach memory at different speeds. Still symmetric in the software sense; the hardware is not uniform
BSP / AP	Bootstrap Processor / Application Processor — in x86 terms, the core that boots first and the ones it starts. Roles in boot, not in steady state
MP	The older abbreviation, from the era when "multiprocessor" meant two physical CPU sockets on one motherboard
Why it's the right word for what SMP.md describes
The document's whole argument depends on symmetry. arch_context_switch saving tpidr_el0 per context, the run queue becoming per-CPU, the big lock being taken by whichever core gets there first — all of that assumes any core can run any task. If core 0 were special (only it could handle the console, say), the design would be AMP, and SMP.md's per-CPU table would look different: it would name which cores the u area belongs to rather than making it per-task.

That's also why the document's §7 puts the u area first. Under SMP, attachreg reaching a global by name is wrong because any core might be the one forking. Under AMP it would be wrong for a different reason. The fix — pass the current task as a context — is the SMP fix.

So: Symmetric Multiprocessing, proposed for UIOX in SMP.md, which records what has to change before more than one core can be added.

=============================
None of the six is a "better" option — they're mostly answers to different questions, and only one of them is a choice you actually get to make.

The ones you don't choose
SMP, NUMA, SMT, BSP/AP are descriptions of hardware and boot roles. You don't pick them; you write code that copes with whichever your target has.

SMP vs NUMA — a memory topology fact. Your four arches on the hardware you're targeting are almost certainly flat SMP. NUMA becomes real on multi-socket servers, and the software answer is "same code, plus locality hints."
SMT — a core property. Two hardware threads on one core means two contexts sharing one L1/L2, so a spinlock held by one thread stalls its sibling. Nothing to design for on a target that has it off.
BSP/AP — boot roles only. Which core is first matters for a few hundred instructions; after the secondaries are up, they're peers. This is a section of your start-up code, not an architecture.
The one you do choose
SMP vs AMP is the real decision, and it's yours.

SMP	AMP
Cores	peers, any core runs any task	different roles, often different images
Kernel	one image, shared memory	often two images, or one with a pinned core
Your SMP.md per-CPU table	the whole thing applies	different — it would name which core owns the u area
Cost	locks everywhere	partitioning instead of locking
Where it's used	every general-purpose OS	DSPs, real-time co-processors, phone basebands, cars
For UIOX, SMP is the right answer, and it's the one SMP.md already assumes. Three reasons specific to your tree:

Your syscall path is symmetric by construction. syscall_dispatch takes a number and a register file and has no notion of which core it came from. There's nothing to partition.

uiox_apps.c is a service table, not a core assignment. Services are named and started, not pinned. AMP would force a core column into that table, which is the kind of logic its own header forbids.

The per-CPU split is already the right shape. arch_context_switch saves the thread pointer per context on three of four arches — that's an SMP primitive. An AMP design wouldn't need it, because each core has its own fixed base.

Where AMP would actually be defensible for you
One case, and it's worth naming because it's tempting: a real-time core. If you ever want a hard-deadline task that cannot be delayed by the general scheduler, AMP is the standard answer — pin it to a core, keep the other core's kernel off it, no locks between them. That's how most audio, motor-control and automotive systems handle it.

That's not a replacement for SMP. It's one pinned core alongside an SMP kernel, and the right time to consider it is after the SMP version works, not before.

So
There's one decision here and it's already made in the document: SMP. The other four labels describe hardware you'll meet and boot roles you'll implement, and none of them competes with it.




