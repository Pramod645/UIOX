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
=================================================================

11_netfs → 30_KIX/32_FS/03_netfs/
Why 32_FS: The layer map in the docs confirms:

uiox_nfs_rpc.h/.c — ONC RPC/XDR encode/decode with no libc — freestanding kernel code, not a userspace network library
Integrates directly with the VFS layer in 32_FS — mounts as a filesystem type alongside 01_fsa/ and 10_scfs/
No userspace process model needed — the RPC transport runs inside the kernel, same as Linux's in-kernel NFS client (fs/nfs/)
Uses the kernel's own network stack (from 34_CAS) not a userspace socket API
