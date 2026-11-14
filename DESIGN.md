# UIOX — Design Position

What UIOX is, which decisions it has made, which are only implicit, and which
are still open. Every claim cites a path in the tree; nothing here is inferred
from intent.

Last reviewed: 2026-11-14.

---

## 1. The six axes

An OS's architecture is decided by a small number of questions. UIOX's answers
are below, with their status: **decided** (stated in a file), **implicit** (the
tree acts as though it is decided but nothing says so), or **open** (nothing
settles it).

| Axis | UIOX's position | Status |
|---|---|---|
| Where the kernel ends | Monolithic kernel; policy lives in userspace | decided |
| How drivers run | In-kernel, in `30_DSS`, probed at boot | decided, but only by default |
| Process model | `fork` + `exec`, one address space per process | decided |
| Address space | Per-process regions via PSA; COW on fork | decided |
| Start-up | Service table (`g_apps[]`) + supervisor (`uix_uios.c`) | decided |
| IPC | Five mechanisms; none designated primary | **implicit** |
| copyin / copyout | Ad-hoc, per caller | **open** |

Note that this table has seven rows against six named axes: copyin/copyout is
not one of the original six, and it is the one that matters most as callers
multiply. It is listed last so it is not lost.

### Axis 1 — where the kernel ends

**Decided, and stated.** `30_KIX/common/uiox_kix_router.c` says it in its own
header:

> It receives a NUMBER and up to six arguments from the arch trap handler,
> decides WHICH subsystem owns that number, calls that subsystem's dispatcher,
> and returns the result.  It holds no process logic and no file logic — a
> place where it did would be a sign the boundary had drifted.

That is the Unix principle: mechanism in the kernel, policy in userspace. It is
the test every future addition should pass.

### Axis 2 — how drivers run

**Decided only by default.** `30_KIX/30_DSS/src/uiox_dss_init.c:122` defines
`int uiox_dss_init(void)`, called from `30_KIX/common/uiox_kernel_main.c`.

The seam for a different answer already exists:

- `30_KIX/30_DSS/10_DI/` — driver interfaces, archived as `libdi.a`
- `30_KIX/30_DSS/20_DD/` — driver implementations, archived as `libdev.a`

Two archives, one for the interface and one for the body, is exactly the split a
userspace driver server needs. `uiox_kix_psa_region.h` states the governing rule
for that kind of split: *"the test is direction of calls, not depth in the tree."*

What is missing is the intent, written down. Until it is, nothing stops a future
driver from reaching upward into the kernel.

**The decision to make:** do `20_DD` servers ever run out-of-kernel? If no, say
so — it makes `10_DI`'s contract simpler and permanent. If yes, `10_DI` must be
message-passing from today, because retrofitting it later is the migration macOS
is still carrying out three decades on.

### Axis 3 — process model

**Decided.** `fork` then `exec`, one address space per process.

Evidence, from `30_KIX/33_PCS/50_scpcs/src/uiox_kix_scpcs_dispatch.c`:

    [SYS_FORK]  = { scpcs_call_fork,     "fork"  }
    [SYS_WAIT4] = { scpcs_call_wait_pid, "wait4" }
    [SYS_EXECV] = { scpcs_call_execve,   "execv" }

with `SCPCS_ADAPTER(scpcs_call_fork, uiox_kix_scpcs_fork, 0)` and
`SCPCS_ADAPTER(scpcs_call_execve, uiox_kix_scpcs_execve, 3)` beside it. The
userspace supervisor uses the same shape: `50_UIX/20_uios/uix_uios.c` calls
`sys_fork()` in `start_app()` and `sys_execv()` in the child.

This is Unix's model, not Windows'. `CreateProcess` allocates and loads in one
call and has no fork; UIOX has chosen the harder model and built it.

### Axis 4 — address space

**Decided.** Per-process regions, shared by reference count, COW after fork.

- `30_KIX/33_PCS/40_psa/include/uiox_kix_psa_region.h:52-54` — region types:
  `REG_TEXT` (shared), `REG_DATA`, `REG_STACK` (private after first write)
- PSA algorithm 9, `dupreg` — `40_psa/src/uiox_kix_psa_region.c` — copies the
  descriptor and the page table and marks writable pages COW
- `uiox_kix_psa_context.h` — `uiox_kix_psa_user_addr_space_t` is the per-process
  map: `uas_text_base`, `uas_data_base`, `uas_stack_base`, `uas_shmem_base`

One caveat worth recording: `uiox_kix_psa_sys_context_t`, `pc_regs` and
`pc_user` are three views over one region of memory, declared as a union. The
loader must write `sc_switch` and `sc_layers[0]` and must never write
`pc_user`/`pc_regs` directly — doing so overwrites `sc_proc_entry` and the
interrupt path's own frame.

### Axis 5 — start-up

**Decided.** A declarative service table plus a supervisor:

- `50_UIX/20_uios/uios_apps.c` — ten rows: name, path, policy, priority,
  restart, dependency, then four runtime fields
- `50_UIX/20_uios/uix_uios.c` — `uios_init`, `uios_launch_apps`, `uios_run`
  (the reaper), `uios_shutdown`

This is systemd's answer, or launchd's, rather than BSD's `init` plus rc
scripts. The table's own header states the intent:

> Separaed from uix_uios.c on purpose: init exists so that "what boots" is a
> decision you can change without recompiling the supervisor.

Three things the general-OS version has that UIOX does not yet:

1. **Readiness as a signal.** `UIOS_READY_TICKS` is a liveness check, and the
   header says so: *"that is a liveness check, not a proof of readiness, and the
   difference matters: the alternative (ask the service) needs a channel that
   does not exist yet."* A channel is axis 6.
2. **Configuration outside the binary.** The table is compiled in.
3. **A distinction between services and tests.** Several rows are test programs,
   not services — see §3.

### Axis 6 — IPC

**Implicit — and this is the gap.** Five mechanisms exist, all routed:

    SYS_MSG*   33_PCS/00_IPC/src/uiox_kix_msg.c
    SYS_SEM*   33_PCS/00_IPC/src/uiox_kix_sem.c
    SYS_SHM*   33_PCS/00_IPC/src/uiox_kix_shm.c
    (socket)   33_PCS/00_IPC/src/uiox_kix_socket.c
    (ptrace)   33_PCS/00_IPC/src/uiox_kix_ptrace.c

Linux has the same five and works because sockets won informally. Windows chose
objects; Mach chose ports. UIOX has all five with none designated, which means
the second server to be written will pick one by accident.

**The decision to make:** name the mechanism a program should reach for, in a
header, and mark the rest as layers over it. Axis 5's readiness signal depends
on this — you cannot ask a service whether it is ready without agreeing how to
ask.

### Axis 7 — copyin / copyout

**Open, and the one with the shortest fuse.** There is no `copy_from_user` in
the tree. `30_KIX/33_PCS/50_scpcs/src/uiox_kix_scpcs_execve.c` says so in its own
header:

> execve receives a USER pointer and namei() wants a KERNEL string.  There is no
> copy_from_user in this tree — 50_scpcs says so in three places — so
> user_copyin_str() below does it, bounded, probing each byte.

That is correct for one caller. It is not a boundary. Every syscall taking a
pointer needs the same validation, and twenty slightly different validations is
where real OSes get their security bugs — which is why Unix has `copyin` and
Windows has the object manager.

The file already names the promotion: *"Promote it into the syscall layer when a
second caller appears."* That promotion is the next real boundary work.

---

## 2. What the tree refuses to fake

The strongest property of this tree is that it will not claim something it has
not done. That habit is why this document could be assembled from the sources at
all, and it is worth protecting as the open axes get decided.

| File | What it refuses to claim |
|---|---|
| `uiox_kix_psa_region.h` | to mark pages VALID that were never read |
| `uios_apps.c` | that `/bin/app_*` exists before the volume carries it |
| `uiox_kix_router.c` | to name a subsystem it does not implement |
| `uix_uios.h` / `uios_apps.c` | readiness it cannot prove (ticks, not signals) |
| `uiox_kix_scpcs.h` | an upward include for one constant — the numbers are copied down instead |
| `uiox_pkg_types.h` | nothing — but its `_pad[24]` comment claims 512 while the packed size is 416, and the packer measured rather than believed it |

The last row is the counter-example that proves the rule: a comment claiming a
size the layout does not produce, caught by a tool that measured. Keep measuring.

---

## 3. Services versus tests

`uios_apps.c` is a **start-up list**, so every row should be something the
machine needs running. Four of the ten are test programs:

| Row | Kind |
|---|---|
| `logger` | service |
| `filesvr`, `netsvr` | service |
| `shell` | service (the interactive one) |
| `fstest`, `ipctest`, `memtest`, `sigtest`, `clocktest` | **test** |
| `schedtest` | **test** |

In a general-purpose OS these run from a test harness, not from init. Linux's
`cyclictest`, FreeBSD's `stress2`, illumos's `ztest` are all programs a person or
CI invokes deliberately; no boot path spends a pid proving timeslicing works.

The general-OS shape, once it is available:

- the program lives in a test location, not in the boot list
- it is invoked explicitly, with arguments selecting the case
- it exits non-zero on failure so a harness can read the result

Two reasons this is not urgent: `g_apps[]` is currently the only mechanism for
starting a program, and none of these tests blocks the boot. It becomes a real
question the moment the shell can fork something — at which point `schedtest`
can be a shell command rather than a boot service.

---

## 4. The boot chain, for reference

Established by build, not by run. As of this revision, all four architectures
produce a kernel ELF with no undefined symbols and a userspace image with its
trap instructions counted.

    kernel ELF
      → kinit_main: vfs_mount_root(0)
      → uiox_fs_populate_root  (writes /bin, /pkg)
      → uiox_pkg_store_mount
      → uiox_dss_init          (driver probes)
      → uiox_kix_scpcs_execve("/bin/uios", argv, envp)
           → bare ELF: base = 0, regions mapped, eret to EL0
      → uios: uios_syscall_selftest  (a real trap, before anything else)
      → uios_launch_apps → start_app → sys_fork + sys_execv
      → SYS_EXECV (250) → scpcs_call_execve
           → /pkg/app_shell.upkg: pkg_payload sets base = 388
           → regions mapped, shell entered
      → the shell's first write(1, ...) traps
           → uiox_kix_router.c: fd 0/1/2 branch
           → uiox_console_write

The first three lines of console output on a boot distinguish the three possible
outcomes:

| Output | Meaning |
|---|---|
| `[kernel] uiox_dss_init()...` then `[scix] svc reached the kernel` then the shell prompt | the whole chain works; EL0 confirmed |
| the shell prompt with no `[scix]` line | exec worked, the shell is at EL1 — the arch user-mode values need another look |
| `[uios] execv(...) failed` | the loader did not find the ELF inside the `.upkg` — check `base` and the path |

---

## 5. Open decisions, in the order they bind

1. **Name the primary IPC mechanism** (axis 6). Axis 5's readiness signal is
   blocked on it.
2. **Decide whether drivers ever leave the kernel** (axis 2). The `10_DI` /
   `20_DD` seam is the place, and the time to decide is before a second driver
   is written.
3. **Promote `user_copyin_str` into the syscall layer** (axis 7). The trigger the
   file names is a second caller; the second caller is whichever syscall next
   takes a pointer.
4. **Separate services from tests** (axis 5, §3). Not urgent until the shell can
   fork.
