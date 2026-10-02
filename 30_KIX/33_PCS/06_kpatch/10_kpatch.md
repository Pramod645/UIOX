50_UIX/10_kpatch/
├── include/
│   ├── uiox_kp_types.h        # Types, error codes, patch descriptor
│   ├── uiox_kp_arch.h         # Arch-specific: trampoline opcodes
│   ├── uiox_kp_mem.h          # Executable memory allocator
│   ├── uiox_kp_patch.h        # Core patch engine API
│   └── uiox_kpatch.h          # Master umbrella include
└── src/
    ├── arch/
    │   ├── uiox_kp_arch_arm64.c
    │   ├── uiox_kp_arch_arm32.c
    │   └── uiox_kp_arch_x86.c
    ├── uiox_kp_mem.c
    ├── uiox_kp_patch.c
    └── uiox_kp_demo.c
====================================
Integration into main.c (Stage 7)
/* In main.c Stage 7 (scheduler init) — add after sched_init(): */

#include "50_UIX/10_kpatch/include/uiox_kpatch.h"

/* Stage 7: Scheduler / Sync */
banner("Stage 7 — Scheduler / Sync + kpatch init");

sched_init();
wait_init();
timer_tick_start();

/* Initialise the live patching engine */
uiox_kp_err_t kp_rc = uiox_kp_engine_init();
kprintf("  [kpatch] engine: %s\n", uiox_kp_err_str(kp_rc));
==============================================================
Layer Map:
File	Layer	UIOX integration
uiox_kp_types.h	Types — patch descriptor, state machine, module	Shared by all layers
uiox_kp_arch.h	Arch — trampoline opcodes, jump sizes, cache flush	10_Arch — arch_defs.h register ops
uiox_kp_mem.h/.c	Executable memory — bump allocator for trampolines	33_ProcessControlSubsystem — mm.h
arch/uiox_kp_arch_arm64.c	ARM64 — B/far-JMP stubs, LDR/BR trampoline	10_Arch/arm64/include/arch_defs.h
arch/uiox_kp_arch_arm32.c	ARM32 — B/LDR-PC stubs	10_Arch/arm32/include/arch_defs.h
arch/uiox_kp_arch_x86.c	x86-64 — JMP rel32 / FF25 stubs, clflush	10_Arch/x86_64/include/arch_defs.h
uiox_kp_patch.c	Engine — register/enable/disable, module load, stop_machine	34_CAS atomics, 33_ProcessCtrl quiesce
uiox_kp_demo.c	Demo — two buggy functions patched live	Exercises full API
uiox_kpatch.h	Umbrella — single include + state/error helpers	40_SystemCallInterface SYS_KPATCH_*
==================================
10_kpatch → 30_KIX/33_PCS/06_kpatch/
Why 33_PCS: The docs are explicit — uiox_kp_engine_init() is called directly after sched_init(), wait_init(), and timer_tick_start() in the kernel init sequence. The layer map in the docs confirms:

uiox_kp_mem.h/.c — executable memory bump allocator for trampolines → integrates with 33_PCS/02_MemMngnt/
uiox_kp_arch_arm64.c / uiox_kp_arch_arm32.c — arch-specific trampoline opcodes, direct instruction patching of live kernel text — requires kernel write access to executable memory, impossible in userspace
Syscalls SYS_KPATCH_* in 40_SystemCallInterface — kernel syscall table entries
The trampoline allocator sits on top of mm.h from 33_PCS/02_MemMngnt/ directly



========

original function
  → patched: JMP
    → trampoline (5-byte JMP → patch wrapper)
      → patch wrapper: uiox_kp_new_func()
        → optional call back to saved original code



That's the classic detour shape: overwrite the function's first instruction with a jump to your replacement, and hand the replacement a pointer it can call to reach the original. The diagram is accurate for what the code intends.

What "kernel patching" actually means here
Three things are happening at once, and the code separates them well:

1. Instruction rewriting in live text. uiox_kp_enable() overwrites the first N bytes of a function that is already in memory and executable. UIOX_KP_JMP_SIZE_ARM64_NEAR 4 / _FAR 16 / X86_NEAR 5 — those are the byte counts of a jump instruction per architecture. The saved_bytes[16] field in the descriptor exists because you must remember what you destroyed.

2. A trampoline so the original stays callable. Overwriting the prologue means the original code is gone from its own address. So a trampoline is built in a separate executable pool: it runs the saved prologue bytes, then jumps to orig_func + saved_len — past the damage. That's what makes kp_new_sched_get_prio() able to call kp_orig_sched_get_prio() and get real behaviour rather than a jump into itself.

3. Cache coherency. icache_flush in the arch ops exists because on ARM the I-cache and D-cache aren't coherent. You write instructions with a data store, and the CPU may keep executing stale bytes from the instruction cache. Without the flush the patch silently doesn't take effect — which is the worst failure mode, since everything reports success.

Why a kernel would want this
Four reasons, and they're the real answer to your question:

Fixing a bug without rebooting. You have a kernel in the field with a wrong bounds check in a driver. Rebuilding and reflashing means downtime. Loading a patch that replaces the function's first instruction takes effect immediately — the next caller gets the new code.

Testing a change against live state. Rebooting loses whatever the machine was doing. Patching lets you alter one function while the system keeps its accumulated state, so you can observe the effect of that single change.

Instrumentation. Wrapping a function to count calls, log arguments, or measure latency. call_count in the descriptor is exactly that — the engine already tracks how often the replacement ran.

Emergency mitigation. A security fix, or a workaround for a hardware erratum, deployed before the next scheduled release.

And the reason it must be in the kernel rather than userspace: make_writable() on kernel text, instruction-cache flushing, and quiescing all CPUs are all privileged. The layer map's reasoning — that trampoline opcodes require write access to executable kernel memory — is correct.

Where the code contradicts itself
Four findings, in order of severity.

1. uiox_kp_demo.c calls two functions that don't exist in uiox_kp_patch.h. The header declares uiox_kp_engine_init and uiox_kp_engine_deinit, but the demo calls:

c


uiox_kp_err_t rc = uiox_kp_engine_init();   // declared as taking (void) — OK
That one's fine. But uiox_kp_err_str() is used in the demo and lives in uiox_kpatch.h, while the demo includes ../include/uiox_kpatch.h — so that resolves. The real problem is the demo's own use of s_fix_module.patches[0].trampoline before uiox_kp_module_load() is called: the trampoline can't exist until the patch is enabled, so kp_orig_sched_get_prio is wired to uiox_sched_get_prio and then re-wired after load. The code acknowledges this in a comment, but it's a race the API shape shouldn't require.

2. s_arch and s_arch_ops are two variables for one thing. In uiox_kp_patch.c:

c


static const uiox_kp_arch_ops_t *s_arch = NULL;
...
static const uiox_kp_arch_ops_t *s_arch_ops = NULL;
s_arch is declared, never assigned, never read. s_arch_ops is what uiox_kp_arch_register/_get use. Under -Wall that's an unused-variable warning — and the duplication is the same shape as every other defect this session has turned up.

3. uiox_kp_mem_free zeroes but does not reclaim. The banner says "free is a no-op but marks the region unused," and the body does mark used = false — but s_pool_top never decreases, so the space is never handed out again. It's a bump allocator with a leak by design. Fine for a fixed set of patches; a problem if you load and unload modules repeatedly, which the demo does.

4. The x86 far-jump constant and its comment disagree.

c


#define X86_JMP_FAR_OP0         0xFFu
#define X86_JMP_FAR_OP1         0x25u
#define X86_JMP_FAR_SIZE        14u
FF 25 00 00 00 00 is 6 bytes, plus the 8-byte target is 14. That's right. But X86_JMP_NEAR_SIZE 5u with E9 <rel32> — also right. So no error here; I'm flagging it because I checked and it holds.

What the layering gets right
The move from 50_UIX/10_kpatch/ to 30_KIX/33_PCS/06_kpatch/ is correct, and for the reason the docs give: the engine is called from main.c Stage 7 right after sched_init(), it patches kernel text, and it allocates trampolines from an executable pool. None of that is possible in userspace.

But 06_kpatch's placement inside 33_PCS is worth questioning. The module's own banner says it integrates with 34_CAS for atomics and 10_Arch for cache flush — neither of which is in 33_PCS. And it patches any kernel function, not process-control ones specifically. 33_PCS is the process-control subsystem; a general kernel patching engine there is a layering accident, the same kind that produced 02_MemMngnt doubling 01_schedular's clock.

What I'd want to read next
uiox_kp_patch.c in full — I have it only in preview past (451 lines), and specifically uiox_kp_enable's body. That's where make_writable, the jump write, and the cache flush have to be sequenced correctly, and it's the one place a mistake produces a patched kernel that appears to work until the next call.

Paste it and I'll check the enable/disable sequence against the diagram, plus the two variables and the trampoline wiring.




 #include "uiox_klibc.h