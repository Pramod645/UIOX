# UIOX — Server Design

What it takes to make a KIX machine serve: files, packages, and services to
other machines. Companion to `DESIGN.md` and `SMP.md`; read those first.

Every claim cites a path in the tree. Nothing is inferred from intent.

Last reviewed: 2026-11-14.

---

## 0. Three meanings of "server", and they are different projects

| Meaning | Needs | Where it lands |
|---|---|---|
| A machine that **exports its filesystem** | `03_netfs` as a server, not a client | `30_KIX/32_FS/03_netfs/` + a mount route |
| A machine that **runs services for others** | `uios` starting servers, a protocol, a name service | `50_UIX/20_uios` + `21_apps` |
| The kernel as a **bootstrap target** | netboot: the image arrives over the network | `01_uBoot` + the NIC path |

The tree already anticipates the first two. `uios_apps.c` carries
`app_fileserver` and `app_netserver`; `uiox_pkg_store.h` names
`UIOX_PKG_REPO_LOCAL` as the only repo kind implemented. So this is mostly making
existing declarations real rather than inventing a design.

---

## 1. What `03_netfs` actually is

Eight headers, seven sources:

    include/  uiox_netfs.h      uiox_nfs_9p.h     uiox_nfs_cache.h
              uiox_nfs_proto.h  uiox_nfs_rpc.h    uiox_nfs_types.h
              uiox_nfs_vfs.h    uiox_nfs_virtfs.h

    src/      uiox_netfs_demo.c uiox_nfs_9p.c     uiox_nfs_cache.c
              uiox_nfs_proto.c  uiox_nfs_rpc.c    uiox_nfs_vfs.c
              uiox_nfs_virtfs.c

**It carries three protocols, selected at RUNTIME — and that is a design, not
a defect.** `uiox_nfs_9p.c` is Plan 9's protocol; `uiox_nfs_rpc.c` is ONC RPC,
the NFS protocol; `uiox_nfs_virtfs.c` is a virtio-backed filesystem. The mount
layer dispatches on a type field:

    uiox_nfs_types.h:138   typedef enum {
    uiox_nfs_types.h:140       UIOX_NETFS_9P2000  = 1,   /* Plan 9 */
    uiox_nfs_types.h:142   } uiox_netfs_type_t;

    uiox_nfs_vfs.c:65      switch (p->type) {
                           case UIOX_NETFS_NFS3:    … uiox_nfs3_mount(&m->nfs3, …)
                           case UIOX_NETFS_9P2000:  … uiox_9p_connect(&m->p9, …)
                           case UIOX_NETFS_VIRTFS:  … uiox_virtfs_init(&m->virtfs, …)
                           default: return UIOX_NFS_ERR_UNSUP;

An earlier revision of this document called that "several mechanisms, none
designated primary" and recommended deleting one. That was wrong, and it was
written before `uiox_nfs_vfs_mount` had been read. Deleting either protocol
breaks a `case`.

**Both protocol implementations are at the same stage.** Line counts:

    268  uiox_nfs_rpc.c     ONC RPC
    261  uiox_nfs_9p.c      Plan 9

Within seven lines of each other. This is not one finished implementation and
one stub — it is two ports of comparable completeness behind one interface,
which is what a runtime-selectable layer should look like.

**And it is built, linked, and unreachable.** `-I.../03_netfs/include` is on the
kernel's include line and `libfs.a` is 1,556,954 bytes, so it compiles into the
archive. But:

    uiox_nfs_vfs.c:28      uiox_nfs_err_t uiox_nfs_vfs_init(void)      ← definition
    uiox_nfs_vfs.h:89      uiox_nfs_err_t uiox_nfs_vfs_init(void);     ← declaration
    uiox_netfs_demo.c:80   uiox_nfs_err_t rc = uiox_nfs_vfs_init();    ← its ONLY caller

and a grep over `30_KIX/common`, `32_FS/src` and `33_PCS/src` finds exactly one
mention outside `03_netfs` — a comment at `uiox_kernel_main.c:73`:

    32_FS/03_netfs    — in-kernel NFS/RPC client

So the init has no production caller and nothing names the subsystem. This is the
same shape `uiox_dss_init` was in before it was wired: implementations with no
entry point.

**Note what that comment says: "client".** Netfs is described in the kernel's own
header as a client. A server is the other half, and the tree does not claim to
have it.

---

## 2. Why serving needs a network filesystem at all

Three reasons specific to this tree, not general hygiene:

**`UIOX_PKG_REPO_LOCAL` is the only repo kind the store uses.**
`uiox_kernel_main.c` calls `uiox_pkg_store_init(&s_pkg, UIOX_PKG_REPO_LOCAL,
"/pkg")`. A local-only repo means every `.upkg` must ship inside the kernel image
— which is what `uiox_fs_populate.c` does. A remote repo is the enum value that
does not exist yet.

**Two servers are already in the boot table with no protocol between them.**

    { "filesvr", "/bin/app_fileserver", 2, 5, UIOS_RESTART_CR, UIOS_NO_DEP, … }
    { "netsvr",  "/bin/app_netserver",  2, 5, UIOS_RESTART_CR, UIOS_NO_DEP, … }

Both have the right policy for a server — restart on crash, priority 5. Neither
has a client protocol.

**Everything the machine runs currently comes from its own kernel image.**
`uiox_fs_populate.c` writes `/bin/uios` and `/pkg/<name>.upkg` from embedded
blobs. A network FS is the mechanism that lets a volume exist elsewhere.

---

## 3. What the kernel gains

Four items. Two are real work; two are decisions.

### 3.1 A working NIC path — **real work, and first**

`30_KIX/30_DSS/20_DD/01_Com/eth/` exists and `libdev.a` builds. But
`uiox_dss_init` was only recently wired, so **nothing has probed the NIC**. A
server kernel needs:

- the Ethernet driver actually initialised and its IRQ registered
- a receive path — `uiox_uart_if.c` shows the pattern for a character device:
  an IRQ handler filling a ring, a consumer draining it
- a transmit path with a queue, so a send does not block inside an interrupt

Nothing else in this document works until this does. A protocol over no link is
a suggestion.

### 3.2 The mount table accepting a non-local filesystem — **decision**

SCFS owns the mount table — the transcript states it plainly: *"SCFS owns the
syscall bodies and the mount table."* Today it holds:

    UNFS   on ROOT_DEV (0)   the on-disk format
    SCFS   on SCFS_DEV (1)   the file/syscall level, no on-disk format

A network mount is a third kind of entry, and the table's entry type must allow
it. The question to answer is whether a remote mount is:

- **a peer** — SCFS knows about a `REMOTE` filesystem type alongside UNFS, or
- **a layer under UNFS** — the network block device presents blocks and UNFS
  formats over it

The second is what NFS-over-a-block-device would be; the first is what 9P
client-in-place is. `uiox_kix_psa_region.h` gives the test to apply: *"the test
is direction of calls, not depth in the tree."* Whichever choice makes the call
direction downward is the right one.

### 3.3 An init call — **two lines, once 3.4 is decided**

The pattern already exists at `uiox_kernel_main.c:427`:

    early_puts("[kernel] uiox_fs_init()...\r\n");
    (void)uiox_fs_init();

Netfs gets the same treatment — non-fatal, announced, so a machine with no
network still reaches the console and the local package store:

    early_puts("[kernel] uiox_nfs_vfs_init()...\r\n");
    (void)uiox_nfs_vfs_init();

with `extern uiox_nfs_err_t uiox_nfs_vfs_init(void);` beside the other
declarations, and the include if `uiox_nfs_vfs.h` is not already reachable.

This is safe now: the mount layer selects its protocol at runtime from
`p->type`, so wiring the init does not bake a choice — the choice is made per
mount by the caller.

### 3.4 Protocol selection — **already decided, and it is runtime**

There is no decision to make here. The mount layer takes a type and dispatches:

| Case | Protocol | Source |
|---|---|---|
| `UIOX_NETFS_NFS3` | ONC RPC | `uiox_nfs_rpc.c` — 268 lines |
| `UIOX_NETFS_9P2000` | Plan 9 | `uiox_nfs_9p.c` — 261 lines |
| `UIOX_NETFS_VIRTFS` | virtio-backed | `uiox_nfs_virtfs.c` |

Each case wires the caller's transport into its own context and then calls that
protocol's mount or connect. Nothing is chosen at build time, so nothing is
chosen by accident — the earlier concern in this document.

**What the caller supplies, in every case:**

    uiox_nfs_mount_params_t
        type          which protocol
        mount_point   where it lands
        read_only
        net_send      fn pointer  ← the transport
        net_recv      fn pointer  ← the transport
        net_ctx       void *      ← opaque, any wire fits
        timeout_ms
        server_ip / export_path   (NFS3)
        p9_aname                  (9P)
        virtio_mmio_base          (VIRTFS)

**The transport is two function pointers, not a chosen IPC mechanism.** That is
the correction to `SMP`-era reasoning about this subsystem: `net_ctx` is
`void *` and `send`/`recv` are injected, so a 9P or RPC client needs any two
functions — not a designated primary transport. Netfs is therefore **not**
blocked on `DESIGN.md` axis 6, and this document's earlier claim that it was is
withdrawn.

What that leaves is one file: whatever link you choose, someone must write
`net_send` and `net_recv` over it.

**One gap the mount layer names itself:**

    uiox_nfs_vfs.c:100  /* Initialise cache (stub uptime — real kernel wires
                            timer here) */

The page cache in `uiox_nfs_cache.c` carries timestamps and expects a monotonic
clock to be supplied. Whatever wires the netfs init should also supply that.
---

## 4. What userspace gains

This is the larger half, and it follows from `DESIGN.md` axis 1: mechanism in
the kernel, policy in userspace.

### 4.1 A name service — **the gating item**

A client must find a server. `uios_apps.c` names services to **start** them; it
has no name-to-endpoint mapping. So a client cannot ask for "the fileserver"
without knowing where it is.

This is the same gap that blocks readiness signalling — `DESIGN.md` axis 5:

> that is a liveness check, not a proof of readiness … the alternative (ask the
> service) needs a channel that does not exist yet.

One program closes both. The smallest version:

- it is started first — `g_apps[0]`, `UIOS_NO_DEP`
- every other service registers `name → endpoint` with it on start
- a client asks it for a name and gets an endpoint back
- a service publishing "I am ready" is the same message

That is D-Bus's job reduced to one file. It cannot be written until the IPC
mechanism is chosen — see §5.

### 4.2 Server apps as bodies

`app_fileserver` and `app_netserver` are rows with the right policy and no
implementation. They need:

- a loop that accepts connections and serves them
- the protocol from §3.4 on one side, and the local filesystem on the other
- to never exit on a bad request — a server that dies on malformed input is a
  denial-of-service waiting to happen

### 4.3 A remote package repo

`UIOX_PKG_REPO_LOCAL` is what the store is initialised with. If
`UIOX_PKG_REPO_REMOTE` exists in `uiox_pkg_types.h`, the store already
anticipates fetching and only needs the transport. If it does not, that is a
one-line enum addition plus the fetch path.

---

## 5. The decision that gates everything

**Pick the primary IPC mechanism.** `DESIGN.md` axis 6 states the position it is
waiting for:

> name the mechanism a program should reach for, in a header, and mark the rest
> as layers over it.

Two things are blocked on it:

1. the name service (§4.1) — it needs a transport
2. the readiness signal (`DESIGN.md` axis 5) — same

**Netfs is not blocked on it**, and an earlier revision of this document was
wrong to say so. `uiox_nfs_mount_params_t` takes `net_send` / `net_recv` as
injected function pointers with a `void *net_ctx`, so a network filesystem
needs any two functions, not a designated primary mechanism. §3.4.

So the server project and the IPC axis overlap in one place only: the name
service. That is still the gating item, and it is still the right first
decision — a service nobody can find is a service nobody can use.

---

## 6. Order

    1. boot the kernel once                     ← still never done
    2. pick the primary IPC mechanism           ← DESIGN.md axis 6
    3. build the name service                   ← g_apps[0], closes readiness too
    4. verify the NIC path (§3.1)               ← real work, blocks all serving
    5. pick 9P or RPC, remove the other         ← §3.4
    6. wire uiox_nfs_vfs_init + a remote mount  ← two lines + one table entry
    7. server apps as bodies                    ← app_fileserver, app_netserver

Steps 1–3 are prerequisites for anything in this document. Step 4 is
independent of 2–3 and can proceed in parallel.

**A server without a name service is a process nobody can reach.** Step 3 is the
one that turns "a server exists" into "a server can be used."

---

## 7. What this document does not claim

- **That netfs works.** It compiles and links. Whether a single 9P or RPC
  exchange has ever executed is unknown from the sources.
- **That the NIC path works.** Nothing has probed the driver.
- **That either protocol is complete.** `uiox_nfs_proto.c` and
  `uiox_nfs_9p.c` exist; what fraction of the protocol they implement is not
  something a file listing shows.
- **That the kernel can serve.** The tree's own comment calls netfs a *client*
  (`uiox_kernel_main.c:73`). The server half is not claimed anywhere.

The verification for each is the same shape: a boot, a link, and a single
exchange that either completes or does not. Recommended before any of §6:
`grep -n 'send\|recv\|write' 30_KIX/32_FS/03_netfs/src/uiox_nfs_9p.c | head -20` —
that shows whether the protocol file has a transport call at all, or only
message construction.
