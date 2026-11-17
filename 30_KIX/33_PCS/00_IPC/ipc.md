| File | Concept from source |
| --- | --- |
| ipc_types.h | Shared IPC constants, IpcPerm, SimProcess, sleep/wakeup stubs |
| ptrace.h/c | Process tracing — PTRACE_TRACEME, PEEKDATA/POKEDATA (4-switch transfer), CONT, KILL, GETREGS/SETREGS |
| msg.h/c | Algorithms 1–4 — msgget, msgctl, msgsnd, msgrcv (typed message selection, sleep/wakeup) |
| shm.h/c | Algorithms 5–8 — shmget, shmat (lazy page allocation on first attach), shmdt (data persists), shmctl |
| sem.h/c | Algorithms 9–11 — semget, semctl, semop (atomic P/V, reverse-on-block, UNDO flag, wait_incr/wait_zero) |
| socket.h/c | Three-layer model (socket/protocol/device) — socket, bind, listen, connect, accept, send/recv, sendto/recvfrom, OOB, peek, shutdown, close, getsockname, getsockopt/setsockopt |


=================
So I'd rather do them in two batches and tell you the plan than start and stop:

Batch 1 — msg.c and sem.c. These are the two where the sleep change is behavioural (the busy loop, the dead wakeup), so they're the ones worth getting right.

Batch 2 — ptrace.c, shm.c, socket.c. These are closer to mechanical: shm.c and socket.c only carry last_pid/owner_pid fields, which become lookups, and ptrace.c's SimProcess *debugger becomes the real entry.

========================================================


# UIOX Inter-Process Communication

**Design decision, primitive comparison, and ring-buffer layout**

Version 1.0 · draft for review · 15 November 2026

---

## 1. Summary and decision

UIOX carries all five POSIX IPC primitives in the tree and none designated as primary. That is
the open question this document answers. The decision is a layered pair rather than a single
winner:

> **Shared memory is the data plane and a semaphore is the signal that makes it safe; message
> queues carry control traffic; sockets are reserved for the network.**

The reasoning rests on three properties of the UIOX architecture already fixed in the tree:

- **Local costs are real.** A monolithic kernel with a microkernel-styled userspace, so every
  local IPC primitive bottoms out in the same kernel and none of them is cheaper for being in a
  different address space.
- **Control and data differ.** The service table start-up system needs small, typed, ordered
  traffic such as registration and readiness signalling, not bulk transfer.
- **The network plane is not ready.** netfs runs on raw Ethernet frames because the IP stack
  above netif is not yet generated, so a socket is not a cheaper default today — it is a
  heavier one.

---

## 2. The four primitives compared

The table below compares message queues, semaphores, shared memory and sockets on the dimensions
that decide the choice. Rows that carry a warning are the ones that rule a primitive out for a
given job.

| Dimension | Message queue | Semaphore | Shared memory | Socket |
|---|---|---|---|---|
| What it moves | discrete typed messages | a counter; no data | raw bytes, direct | bytes over a stream or datagram |
| Copy cost | 2 copies (send, receive) | none — it is a count | 1 copy to map, 0 per transfer | 2 copies plus framing |
| Kernel involvement | per operation | per operation | mapping only | per operation plus routing |
| Bandwidth | low | n/a | **highest** | medium |
| Latency | low (one syscall each way) | lowest | lowest once mapped | highest (framing, routing) |
| Synchronisation built in | yes — queue semantics | yes — the counter is the sync | **no — needs a semaphore** | partial (stream buffering) |
| Structured or raw | structured | none | raw | raw |
| Cross-machine | no | no | no | **yes — the only one** |
| Namespacing | key or pathname | key or pathname | key or pathname | address and port |
| Composes with any protocol | no — the framing is the message | n/a | yes — the region is just bytes | no — the socket is the framing |
| Failure mode | queue fills, sender blocks | waiter starvation | **no sync means races** | peer vanishes |
| Reachability in UIOX today | in tree | in tree | in tree | needs the IP stack, which is not generated |

### 2.1 What each primitive is good at

**Message queue — the control plane.**
Its value is structure and synchronisation for free: a message has a type, a length and an order,
and the queue handles blocking on both ends. That is exactly the shape of a service registration,
a readiness signal, or a small typed command. It is the wrong shape for bulk, because a queue
deep enough to hold a 1 MB 9P read is a queue that copies that megabyte twice.

**Semaphore — the signal.**
The cheapest synchronisation primitive available: a counter and a sleep/wake pair, one syscall to
change it. On its own it moves no data at all, which is why it is never a transport. Its role in
this design is to make shared memory usable — it is the piece that turns a shared region into a
working queue.

**Shared memory — the data plane.**
The only primitive whose cost does not scale with message size. Map once, then every transfer is
pointer arithmetic on both sides. Its weakness is that it provides no synchronisation whatsoever:
two writers to the same region race, and the kernel will not notice. That is a reason to pair it
with a semaphore, not a reason to avoid it.

**Socket — the network plane, and only that.**
The one primitive that crosses a machine boundary, and therefore irreplaceable for a remote peer.
Locally it is the most expensive of the four: framing, routing, and two copies to do what a
mapping does with one. In UIOX specifically, the IP layer above netif is not yet generated —
netfs runs on raw Ethernet frames precisely to avoid depending on it — so sockets are not even
reachable as a local shortcut today.

---

## 3. Ranking for UIOX

| Rank | Primitive | Role | Why this rank |
|---|---|---|---|
| 1 | **Shared memory** | the data plane — bulk transfers between services | one mapping, zero copy per transfer; composes with any protocol on top |
| 2 | **Semaphore** | the signal — wakeups, mutual exclusion, ring head and tail | cheapest synchronisation; it is the piece shared memory is missing |
| 3 | **Message queue** | the control plane — service registry, small typed commands | structure and blocking built in; wrong for bulk |
| 4 | **Socket** | the network plane only — cross-machine peers, netfs's remote end | the only cross-machine primitive; expensive locally, and not reachable yet |

---

## 4. Which primitive for which UIOX workload

| Workload | Primitive | Reason |
|---|---|---|
| Service table start-up (registration, readiness) | message queue | small typed messages needing order; matches queue semantics exactly |
| Inter-service bulk (9P frames, file data, buffers) | shared memory + semaphore | both ends map once; the semaphore signals "N bytes at offset X" |
| SMP mutual exclusion, kernel data | semaphore | `33_PCS/01_schedular` already has the wait-queue structure to build on |
| netfs to a remote peer | socket, or the existing raw-frame transport | the 9P path uses injected `net_send`/`net_recv`, so it does not care which |
| Console and fd 0/1/2 | none of these | already bound at boot by `uiox_console_bind()` and intercepted by the router |

---

## 5. Shared-memory ring: layout and protocol

This is the concrete structure that the recommendation implies for inter-service bulk transfer.
It is deliberately minimal: one writer, one reader, one semaphore, and a descriptor ring whose
size is fixed at creation.

### 5.1 Region layout

A single shared region, created by the producer and mapped by both sides read-only or
read-write depending on direction. No kernel data structure is visible inside it.

```
+---------------------------------------------------------------+
| uiox_ipc_ring_t                          (1 cache line, 64 B) |
|   uint32  magic           0x55494F58  'UIOX'                  |
|   uint16  abi_version                                         |
|   uint16  head            producer: next slot to write        |
|   uint16  tail            consumer: next slot to read         |
|   uint16  slots           power of two, fixed at creation     |
|   uint16  slot_size       bytes per slot, fixed at creation   |
|   uint32  drops           produced but overwritten            |
|   uint32  flags                                               |
+---------------------------------------------------------------+
| ring slots: slots x slot_size                                 |
|   each slot = uiox_ipc_slot_t:                                |
|       uint32  len           0 = empty, else payload bytes     |
|       uint32  seq           monotonic sequence number         |
|       uint8   payload[slot_size - 8]                          |
+---------------------------------------------------------------+
| (optional) second ring for the reverse direction              |
+---------------------------------------------------------------+
```

### 5.2 The protocol, side by side

| Step | Producer | Consumer | Synchronisation |
|---|---|---|---|
| 1 | read `head` | — | none needed; `head` is producer-owned |
| 2 | write payload into `slot[head & (slots-1)]` | — | none — the slot is not published yet |
| 3 | store `slot.len`, `slot.seq`, then `head++` | — | release barrier before `head++` |
| 4 | `sem_post()` | — | the only syscall on the fast path |
| 5 | — | `sem_wait()` | blocks until a slot is published |
| 6 | — | read slot at `tail & (slots-1)` | acquire barrier after `sem_wait` |
| 7 | — | consume payload, then `tail++` | `tail` is consumer-owned |
| 8 | if `head - tail == slots`, overwrite oldest and `drops++` | — | back-pressure policy, see 5.4 |

### 5.3 Why it is safe with one writer and one reader

- **Single ownership.** `head` and `tail` are each written by exactly one side, so neither needs
  a lock — only a memory barrier so the payload store is visible before `head` advances.
- **Publish last.** A slot is not visible to the consumer until `len` and `seq` are written and
  `head` advances, so a partially written slot can never be read.
- **The semaphore is a doorbell, not a queue.** It carries no data — it only says "a slot is
  ready". Under spurious wakeup the consumer re-checks `head` against `tail` and loops.

### 5.4 Back-pressure and failure policy

| Situation | Policy | Why |
|---|---|---|
| ring full | producer overwrites oldest, increments `drops` | a blocked producer would stall the kernel path; drops are countable and visible |
| consumer slower than producer | `drops` counter rises; consumer sees a `seq` gap | silent loss is worse than a visible gap |
| producer exits without cleanup | ring stays mapped until the mapper closes it | no kernel object to leak; the owner cleans up |
| mapping lost mid-transfer | consumer sees no new `head`; `sem_wait` times out | the same read-and-compare pattern the netfs mount uses |

> **Note on timeouts.** `uiox_kix_scpcs_nano_sleep` is `ENOSYS` today — the wakeup half is
> missing, so a timed sleep never ends. Any IPC wait must therefore be written as
> read-and-compare against the clock, never as sleep-until-deadline. This is the same constraint
> the netfs mount already respects.

---

## 6. Where each piece lands in the tree

The placement follows the include rule already stated in the `32_FS` Makefile: the test is
direction of calls, not depth in the tree.

| Component | Proposed location | Layer | Notes |
|---|---|---|---|
| ring type and header | `30_KIX/33_PCS/50_scpcs/include/uiox_kix_ipc_ring.h` | kernel, PCS | types only; no implementation, no upward include |
| shared-region mapping | `30_KIX/33_PCS/40_psa/` | kernel, PCS | `s_pages` and `uiox_kix_psa_u_area_pool` are the current pools |
| semaphore | `30_KIX/33_PCS/01_schedular/` — beside the wait queue | kernel, PCS | `sched_types.h` already declares `jiffies` and the tick path |
| message queue | `30_KIX/33_PCS/` (new) or `34_CSS` | kernel, PCS/CSS | control plane; small, ordered, typed |
| socket layer | `30_KIX/34_CSS/01_Com/eth/` — beside `uiox_proto.h` | kernel, CSS | `uiox_proto.h` already declares ip4/ip6/tcp/udp input hooks |
| userspace wrappers | `00uixlibs/sys/` — beside `shm`, `sem`, `msg` | userspace lib | raw trap wrappers; `40_SCIX` owns the numbers |
| SCFS entries | `30_KIX/32_FS/10_scfs/src/uiox_kix_scfs_dispatch.c` | kernel, FS | table slots exist; see 6.1 |

### 6.1 The dispatch-table pattern

The primitives will need syscall numbers and dispatch entries. The existing table gives the
shape: three of its slots already sit at a null function pointer and return `ENOSYS` cleanly by
design, so the pattern for adding a primitive is well established.

```c
/* uiox_kix_scfs_dispatch.c — table entry form */
[SYS_SHMAT]  = { (uiox_kix_scfs_fn_t)(void *)uiox_kix_scfs_shmat,  "shmat"  },

/* a real BSD number with no implementation yet: */
[SYS_SEMOP]  = { (uiox_kix_scfs_fn_t)0, "semop" },
```

Adding a primitive is therefore three lines and a body, not a new mechanism. The same
three-part shape that applied to `sys_mount` — number, dispatch slot, handler — applies here.

### 6.2 Similarities to `sys_mount`'s path

The syscall side of IPC will look like the syscall work already done for the filesystem, and the
parallels are worth stating because they predict the failure modes:

- **Two places must agree.** A number in `40_SCIX/uix_sys.h`, mirrored in `10_scfs`'s numbers
  header. When the two disagree, the userspace wrapper and the kernel handler can disagree about
  which call they mean — the same class of conflict netfs's own `SYS_*` defines created.
- **A null slot is safe.** A table entry whose function pointer starts at null returns `ENOSYS`
  cleanly. Reaching an unwired primitive is an error code, not a crash.
- **Arity must match.** A handler with the four-word shape the table declares, and a userspace
  wrapper whose `my_syscallN` count matches it. A three-argument wrapper against a four-argument
  handler puts the wrong value in the register the handler reads.
- **Implicit declarations stop the build.** A build-visible failure under `-Werror` rather than a
  silent one, provided the declaration is reachable. The include path is the usual cause when it
  is not.

---

## 7. Open questions and consequences

1. **Which primitive is primary.** This document names shared memory plus semaphore as the local
   substrate, with message queues for control and sockets for the network. Everything else can be
   a library wrapper on top.
2. **Whether the ring lives in `33_PCS` or `34_CSS`.** Both have the structures — `40_psa` has a
   user-area pool, `34_CSS` has the protocol layer. The deciding question is whether a service
   registry is a process-management concern or a communication one.
3. **Semaphore semantics.** POSIX counting semaphores or the kernel's own wait queue. The tree has
   `sched_types.h` and a timer wheel, so the kernel-internal form is the shorter path; the POSIX
   form is what userspace expects.
4. **Whether the message queue should exist at all.** Or whether a second shared ring plus the
   same semaphore covers registration traffic. The argument for the queue is structure and
   ordering for free; the argument against is a second mechanism to maintain.
5. **How a ring is named and found.** Shared memory needs a key or a pathname, and UIOX has
   neither a namespace service nor the mount table populated yet. The unix-socket filesystem
   namespace is the conventional answer, and it depends on `sys_mount`, which is the pair of
   edits still outstanding.

---

## 8. What not to do

- **Shared memory alone.** Two writers to one region race, and the kernel cannot see it. This is
  the single most common mistake and it fails intermittently, which makes it expensive to find.
- **Sockets as the local default.** Two copies, framing and routing to do what a mapping does
  with one — and in UIOX the IP stack they need is not generated yet.
- **Message queues for bulk data.** The queue copies every byte twice regardless of size, and its
  ordering guarantee is worthless for a 1 MB buffer.
- **A semaphore as a transport.** It moves a count, not data. It is the signal for shared memory,
  never a channel in its own right.
- **Sleeping on a deadline.** `nano_sleep` is `ENOSYS`; a timed sleep never wakes. Read the clock
  and compare.
===================
0.Dimension	1.Message-queue	2.Semaphore	3.Shared-memory	4.Socket
0.What it moves	1.discrete typed messages	2.a counter, no data	3.raw bytes, direct	4.bytes over a stream/dgram
0.Copy cost	1.2 copies (send + receive)	2.none — it's a count	3.1 copy to map, 0 per transfer	4.2 copies, plus protocol framing
0.Kernel involvement	1.per operation	2.per operation	3.mapping only	4.per operation, plus routing
0.Bandwidth	1.low	n/a	highest	medium
0.Latency	1.low (one syscall each way)	2.lowest	3.lowest after mapping	4.highest (framing, routing)
0.Producer/consumer sync built in	1.✅ queue semantics	2.✅ the counter IS the sync	3.❌ needs a semaphore beside it	4.⚠️ partial (stream buffering)
0.Structured vs raw	1.structured	2.none	3.raw	4.raw
0.Cross-machine	1.no	2.no	3.no	4.yes — the only one
0.Namespacing needed	1.key or pathname	2.key or pathname	3.key or pathname	4.address + port
0.Fits UIOX's in-kernel drivers	1.✅	2.✅	3.✅	4.⚠️ needs netif + proto stack
0.Works with the service table	1.✅	2.✅	3.✅	4.✅ but heavier
0.Failure mode	1.queue fills, sender blocks	2.waiter starvation	3.no sync = races	4.peer vanishes