UIOX NFS-lite covers:
  • NFS v3 client (UDP/TCP transport over uiox_fw_eth)
  • Plan 9 / 9P2000 client (simpler, used by QEMU virtfs)
  • VirtIO-FS client (FUSE-over-virtio, QEMU native)
  • Unified VFS mount layer (plugs into 32_FileSystem namei)
  • Remote file operations: open/read/write/close/stat/readdir
  • Attribute caching (30-second TTL)
  • Write-back page cache
  • Syscall shims: mount(2) / umount(2) / statfs(2)
========================================================
50_UIX/11_netfs/
├── include/
│   ├── uiox_nfs_types.h      # Types: FH, attr, dirent, error codes
│   ├── uiox_nfs_rpc.h        # RPC/XDR encode-decode (no libc)
│   ├── uiox_nfs_proto.h      # NFS v3 procedure call wrappers
│   ├── uiox_nfs_9p.h         # Plan 9 / 9P2000 client
│   ├── uiox_nfs_virtfs.h     # VirtIO-FS (FUSE over virtio)
│   ├── uiox_nfs_vfs.h        # VFS mount + inode cache bridge
│   ├── uiox_nfs_cache.h      # Attribute + page cache
│   └── uiox_netfs.h          # Master umbrella include
└── src/
    ├── uiox_nfs_rpc.c
    ├── uiox_nfs_proto.c
    ├── uiox_nfs_9p.c
    ├── uiox_nfs_virtfs.c
    ├── uiox_nfs_vfs.c
    ├── uiox_nfs_cache.c
    └── uiox_netfs_demo.c
=========================================
File	Layer	UIOX integration
uiox_nfs_types.h	Types — FH, attr, dirent, statfs, error codes	Shared by all layers
uiox_nfs_rpc.h/.c	Transport — ONC RPC/XDR encode/decode, UDP/TCP framing	uiox_fw_eth / uiox_fw_wifi for send/recv
uiox_nfs_proto.h/.c	NFS v3 — GETATTR/LOOKUP/READ/WRITE/READDIR/MKDIR/REMOVE/COMMIT	32_FileSystem inode operations
uiox_nfs_9p.h/.c	9P2000 — version/attach/walk/open/read/write/clunk/stat	QEMU -virtfs plan9 transport
uiox_nfs_virtfs.h/.c	VirtIO-FS — FUSE over VirtIO MMIO, nodeid-based ops	QEMU -device vhost-user-fs-pci
uiox_nfs_cache.h/.c	Cache — 30-second attribute TTL, write-back page cache	31_BufferCache design pattern
uiox_nfs_vfs.h/.c	VFS bridge — mount/umount/open/read/write/stat/readdir/syscalls	32_FileSystem namei hook, 40_SystemCallInterface SYS_MOUNT
uiox_netfs_demo.c	Demo — 15 scenarios: NFS3 + VirtFS mount, read/write/mkdir/readdir/rename/unlink	Full API coverage
=====================================================



five netfs files. The headline is blunt: 03_netfs has zero coupling to any of the four FS layers — including the one it claims to plug into.

The dependency scan
Layer	What netfs touches	Where
00_buffcache	nothing — no bread, bwrite, brelse, getblk, BufHdr, bcache	—
01_fsa	namei mentioned in a comment only	demo.c ×2
10_scfs	sys_mount, sys_umount, sys_statfs — declared, not defined	demo.c ×1 each
10_unfs	nothing — no unfs_, no unfs_fs_t, no UNFS_BLOCK_SIZE	—
Zero bcache calls. Zero UNFS references. The network filesystem never reaches the block layer through your stack.

What it does instead — three parallel client stacks


uiox_netfs.h  (umbrella)
   │
   ├── uiox_nfs_types.h      base types, fattr3, dirent, statfs
   ├── uiox_nfs_rpc.h/.c     ONC RPC + XDR  (64 KB buffers)
   ├── uiox_nfs_proto.h/.c   NFS v3 procedures (RFC 1813)
   ├── uiox_nfs_9p.h/.c      Plan 9 9P2000
   ├── uiox_nfs_virtfs.h/.c  FUSE over VirtIO MMIO
   ├── uiox_nfs_cache.h/.c   attribute cache + page cache
   └── uiox_nfs_vfs.h/.c     mount table + file ops
Three client protocols behind one VFS-ish façade, plus its own cache layer. All of it talks to the network through callbacks it does not own:

c


uiox_nfs_err_t (*send)(void *ctx, const uint8_t *buf, uint32_t len);
uiox_nfs_err_t (*recv)(void *ctx, uint8_t *buf, uint32_t max_len,
                       uint32_t *rx_len, uint32_t timeout_ms);
void           *net_ctx;
And the demo supplies stubs, not a network:

c


static uiox_nfs_err_t stub_send(void *ctx, const uint8_t *buf, uint32_t len)
{ UIOX_NFS_UNUSED(ctx); ... return UIOX_NFS_OK; }      /* sends nothing */
With a hard-coded 60-byte canned reply. So uiox_netfs_demo.c exercises the parsers against fixed bytes — a real test of the XDR decode path, but not of any network.

The four-layer perspective you asked for
00_buffcache — no relationship at all. Netfs has its own page cache (uiox_nfs_cache_t, 32 × 4096-byte entries with dirty flags and a writeback callback). That is a second buffer cache sitting beside yours, with different policies: LRU-with-TTL for attributes, hash-modulo eviction for pages, and no BufHdr, no hash queues, no Bach conditions. Two caches, no shared code, no coordination.

01_fsa — declared intent, no mechanism. uiox_nfs_vfs.h says "plugs into 32_FileSystem namei" and "called by 32_FileSystem namei hooks", but nothing in 01_fsa calls uiox_nfs_vfs_*. The link is a comment.

10_scfs — the same situation, one layer up. Netfs declares its own syscall numbers:

c


#define SYS_MOUNT      21u
#define SYS_UMOUNT     52u
#define SYS_STATFS     99u
long sys_mount (long src, long tgt, long fstype, long flags);
Your 10_scfs has uiox_kix_scfs_mount.c, _umount.c, _statfs.c — 31 syscall bodies, and netfs's sys_mount is a different function with a different signature. Both exist; neither knows about the other.

10_unfs — silent. Not one reference. Netfs doesn't know UNFS exists.

The two concrete conflicts worth naming
1. Duplicate syscall symbols. If 03_netfs were added to the build, sys_mount/sys_umount/sys_statfs would collide with 10_scfs's versions — assuming those export the same names, which the demo.c declarations suggest. That's the same failure mode as unfs_vfs.c's thirteen duplicates.

2. Two page caches. uiox_nfs_cache.c's UIOX_NFS_PAGE_SIZE 4096 matches UNFS_BLOCK_SIZE by coincidence, not design, and uiox_nfs_pcache_get compares e->page_offset == offset against an un-aligned offset while alloc stores an aligned one — so a get with a non-aligned offset always misses. A cache bug, independent of the layering.

One thing the author declared that's worth noting
Both uiox_nfs_cache.h and uiox_nfs_rpc.h say the network callbacks are "wired to uiox_fw_eth". uiox_fw_eth doesn't exist in your tree — and uiox_fw_printf (the symbol all three journal .c files need) is presumably meant to come from the same missing firmware layer.

So 03_netfs and 02_jrnal depend on the same absent thing: a uiox_fw_* platform layer. Not the FS stack at all.

Verdict
03_netfs is a self-contained network-filesystem client that does not connect to your filesystem. It's not a layer under 01_fsa; it's a parallel filesystem with its own cache, its own syscall numbers and its own mount table, sitting on network callbacks that don't exist.

That matches the file's own uiox_nfs_vfs.h: uiox_netfs_type_t covers NFS3, 9P2000 and VirtIO-FS — three remote filesystems. It's a netfs client, not a UNFS layer.

Recommendation
Leave 03_netfs unbuilt, same as 02_jrnal and unfs_vfs.c. Your Makefile v2.2.0 already lists it in the "not built here" block, and its own comment says "03_netfs — 9P client — not part of the on-disk path". That judgement was correct.

If you ever do want it, the order is:

Supply uiox_fw_eth — a network send/recv behind those two callbacks. Nothing works without it.
Decide who owns sys_mount — netfs's or 10_scfs's. One must yield.
Decide which page cache wins — netfs's own, or 00_buffcache. Two caches on one machine is a coherence problem waiting to happen.
None of the three is a small change, and none is needed for the on-disk filesystem you've built.


===============

What netfs actually is
Per 03_netfs.md, NFS-lite is a remote filesystem client — three protocols behind one VFS bridge:

Layer	Protocol	Transport
uiox_nfs_rpc	ONC RPC + XDR, 64 KB buffers	uiox_fw_eth / uiox_fw_wifi
uiox_nfs_proto	NFS v3 (RFC 1813) — GETATTR/LOOKUP/READ/WRITE/READDIR/MKDIR/REMOVE/COMMIT	UDP/TCP over the RPC layer
uiox_nfs_9p	Plan 9 9P2000 — version/attach/walk/open/read/write/clunk/stat	QEMU -virtfs plan9 socket
uiox_nfs_virtfs	VirtIO-FS, nodeid-based FUSE ops	QEMU -device vhost-user-fs-pci
uiox_nfs_cache	30-second attribute TTL, write-back page cache	—
uiox_nfs_vfs	mount/umount/open/read/write/stat/readdir + SYS_MOUNT shims	plugs into 32_FileSystem namei
The integration column names uiox_fw_eth, uiox_fw_wifi, 31_BufferCache, 32_FileSystem inode operations, 32_FileSystem namei hook, 40_SystemCallInterface SYS_MOUNT. Every one is a pull, not a push.

====
Meanwhile, the kernel-side gap list, on the assumption the move was deliberate
If netfs is meant to be kernel-resident inside 32_FS, then the gaps are these — and they're different from what I listed before:

1. It has no way to reach the block layer, and doesn't need one — but it must still present InCoreInode. A kernel filesystem that namei can walk must hand back 01_fsa's inode type. Netfs's uiox_nfs_vfs_open(path, flags, mode) -> int fd is a syscall shape, not an inode-returning shape. The bridge between struct uiox_nfs_mount/uiox_nfs_attr_t and InCoreInode is the single biggest missing piece.

2. No registration through vfs_register_fs. 10_unfs/10_scfs register an ops table per device. Netfs has mount(params, &idx) with its own params struct — a second mount mechanism parallel to the first. For kernel residency, it needs to fit vfs.h's uioxfileopst/uioxinodeopst shape, or namei can't dispatch into it.

3. Transport is uiox_fw_eth/uiox_fw_wifi — synchronous or not is unknown. Kernel-side, the RPC layer cannot block spin-waiting for a packet. The HAL declares uiox_fw_eth_send(dev, f, l) and uiox_fw_eth_set_rx_cb(dev, cb, p) — callback-based receive. So there's an async receive path and the RPC layer must be built around it. Whether uiox_nfs_rpc.c is written that way is the key question.

4. sys_mount/sys_umount/sys_statfs declared in demo.c with no definitions — the SYS_MOUNT shims the doc claims. Kernel-side these route through 40_SCIX, which is above 32_FS — the same upward-dependency shape as uix_sys.h.

5. uiox_netfs_demo.c must not be in the kernel archive. 15 scenarios, a main-shaped entry.

The one question that decides everything
Is netfs a filesystem (implements uioxinodeopst, dispatches through vfs.c, hands back InCoreInode) or a filter/translator (a layer that sits between the VFS and the network, with its own object model)?

Your 10_unfs is unambiguously the first. Netfs's headers (uiox_nfs_attr_t, uiox_nfs_mount_t, mount_idx) look like the second. If it's the second, then "moved into the kernel" means it needs an adapter — and that adapter is the work, not the Makefile.

Send the newer doc if it exists, and I'll pin the gap list to what the tree actually decided rather than to a draft its own author has moved past.

==========================
Step	State
six sources compile under -Wall -Wextra -Werror	✅
uiox_nfs_transport.c compiles and archives	✅ all four
libfs.a contains netfs	✅
kernels link, no undefined symbols	✅ all four
uiox_nfs_vfs_init() wired	✅ wired · ⬜ unverified
uiox_dss_init → eth_probe → netif_register	✅ called · ⬜ unverified
TX/RX entry points	✅
VA→PA	✅ identity-mapped
transport pair	✅
monotonic clock	✅ landed — field, call site, adapter, assignment all agree
one uiox_nfs_vfs_mount — the handshake	⬜ next
sys_mount to userspace	⬜ separate pass

Step	State	Notes
sys_mount to userspace	⬜ next	the wrapper/handler mismatch: 3 args vs 4, fstype is the gap
50_UIX/01_shell in the kernel build	⬜	skipped (no sources yet) on all four — umbrella glob
uiox_kernel_main.c:485 + stub at :211	⬜	delete, don't uncomment, under the EL0 design
kernel_main.c:401-402 dead comment pair	⬜	cleanup, from the clock work
kernel_main.c:119 time_service.h include	⬜	now unused — makes -I…01_schedular/include optional
uiox_dss_init → eth_probe → netif_register runtime proof	⬜ deferred	rides along with the boot
uiox_nfs_vfs_mount — the handshake	⬜ deferred	needs a run; nothing static left to check


Row	State
four-arch kernel build	✅ 2d21ad8 · 2c1c288 · 2b18a69 · 2c6c2a5, no undefined symbols
clean-subsys recipe	✅ fixed — ; do on the list's last line, verified end to end
34_CAS typo	✅ removed
30_KIX/Makefile + rebuildkernel.sh cleanup	✅ done
netfs SYS_* redefinitions	✅ commented out; build unchanged
sys_mount handler bodies + wrapper arity	⬜ next
boot	⬜ deferred


=======

Table 1 — the netfs build/runtime chain
Step	State	Everything behind it
six sources compile under -Wall -Wextra -Werror	✅	mount caller compiles clean
uiox_nfs_transport.c compiles and archives	✅ all four	in libfs.a on every arch
libfs.a contains netfs	✅	1,859,632 arm64; sized on the other three
kernels link, no undefined symbols	✅ all four	2d21ad8 · 2c1c288 · 2b18a69 · 2c6c2a5
uiox_nfs_vfs_init() wired	✅ wired · ⬜ unverified	uiox_kernel_main.c:445; nothing has run
uiox_dss_init → eth_probe → netif_register	✅ called · ⬜ unverified	called ≠ reached
TX/RX entry points	✅	ops->tx_submit, ops->rx_poll
VA→PA	✅ identity-mapped	holds only while the kernel stays identity-mapped
transport pair	✅	uiox_nfs_net_send / _recv declared + assigned
monotonic clock	✅ landed	field uiox_nfs_vfs.h:82, call site :102, adapter :398, assignment :498; code grew on three arches
one uiox_nfs_vfs_mount — the handshake	⬜ next (needs a run)	statically exhausted — no sleep, bounded retries, params complete
sys_mount to userspace	⬜ separate pass	now a session, not an edit — see below
Table 2 — the cleanup/follow-on list
Step	State	Notes
sys_mount to userspace	⬜ next	three pieces: 3 dispatch slots at (fn_t)0, no handler bodies, wrapper 3-args vs handler 4-args — fstype is the gap
netfs SYS_MOUNT/SYS_UMOUNT/SYS_STATFS redefinitions	✅ commented out	uiox_nfs_vfs.h:130-132; 22/63 authoritative
50_UIX/01_shell in the kernel build	✅ deleted	block removed from rebuildkernel.sh; optional layers line gone from all four runs
uiox_kernel_main.c:485 + stub at :211	✅ doesn't exist	uioxshellstart is in no revision of the file; :211 today is a fastboot stub
kernel_main.c:401-402 dead comment pair	✅ done	
kernel_main.c:119 time_service.h include	✅ done	-I…01_schedular/include now optional
30_KIX/Makefile header cleanup	✅ done	:17 shell entry + :19-26 move-list corrected
clean-subsys recipe	✅ fixed and verified	; do on the list's last line; make -C 30_KIX clean ran without the && error
34_CAS typo	✅ removed	real directory is 34_CSS
uiox_dss_init → eth_probe → netif_register runtime proof	⬜ deferred	rides along with the boot
uiox_nfs_vfs_mount — the handshake	⬜ deferred	the only row a build can't answer
What changed since you wrote these
Three rows moved, and one was wrong:

The shell row is done, not pending. The optional layers block came out of rebuildkernel.sh and your last four-arch run shows no such line.
The :485/:211 row was never real. Those line numbers were from an earlier revision; that code isn't in the file. I carried them longer than I should have.
clean-subsys is new to the list — a genuine bug found along the way, now fixed and confirmed by an actual make clean.
What's genuinely left
Two rows, and only one is code:

The handshake — one boot. Four arches link identically across four consecutive runs; there is nothing left to learn statically. The single line [netfs] mount: <err> separates EOK from an error from no eth0 — mounted skipped.

sys_mount to userspace — a session. Your own 03_netfs.md:151 already documents the route: "Kernel-side these route through 40_SCIX, which is above 32_FS." So the design decision is written down; what's missing is three handler bodies and a fourth argument in the userspace wrapper.

Two cosmetic leftovers, neither blocking: the three commented //#define lines in uiox_nfs_vfs.h:130-132, and 03_netfs.md:93-95, which still quotes 52u/99u against the tree's 22/63.
======
The dispatch file has no netfs include, and the 32_FS/Makefile has the directory variable ready but doesn't show it on the 10_scfs path yet. Two things to add.

1. The include
30_KIX/32_FS/10_scfs/src/uiox_kix_scfs_dispatch.c, with the other includes:

c


#include "uiox_nfs_vfs.h"   /* sys_mount / sys_umount / sys_statfs */
That's the 10_scfs → 03_netfs edge — lateral within 32_FS, not upward into 40_SCIX as 03_netfs.md:151 claimed.

2. The include path
The Makefile defines NETFS_DIR := $(MFDIR)03_netfs at :76, but the 10_scfs compile line needs its include on the path. Confirm with:

sh


grep -n 'NETFS_DIR' 30_KIX/32_FS/Makefile
If $(NETFS_DIR)/include isn't in the include list for 10_scfs sources, add it — the same one-line-per-recipe pattern that took four rounds for kernel_main.c. Given the file already carries 03_netfs/include for the netfs sources themselves, it may already be global.

3. The three table lines
uiox_kix_scfs_dispatch.c:163-166, replacing the (uiox_kix_scfs_fn_t)0 entries:

c


[SYS_STATFS]      = { (uiox_kix_scfs_fn_t)(void *)sys_statfs,  "statfs"  },
[SYS_MOUNT]       = { (uiox_kix_scfs_fn_t)(void *)sys_mount,   "mount"   },
[SYS_UNMOUNT]     = { (uiox_kix_scfs_fn_t)(void *)sys_umount,  "unmount" },
Same double cast the working entries at :108-115 use.

4. The userspace wrapper
40_SCIX/uix_sys.h:581, the arity defect — sys_mount must pass four words or fstype lands in the register the handler reads as flags:

c


static inline int sys_mount(const char *special, const char *dir,
                            const char *fstype, int flags)
{
    return my_syscall4(SYS_MOUNT, (long)(special), (long)(dir),
                       (long)(fstype), (long)(flags));
}
:586's sys_umount(special) is safe as-is only because the handler UIOX_NFS_UNUSEDs its three trailing args — luck rather than design, and worth a comment saying so.

Why the handler bodies are the good news
uiox_nfs_vfs.c:542 isn't a stub — it maps fstype (nfs/9p/else) to a protocol, takes read_only from flags & 1, parses W.X.Y.Z:/path into server_ip and export_path, and calls uiox_nfs_vfs_mount — the same entry kernel_main.c uses. :571 and :577 forward to uiox_nfs_vfs_umount and uiox_nfs_vfs_statfs. So this row is wiring, not writing.

One consequence worth noting: with these wired, fstype becomes the thing that selects the protocol from userspace — and "9p" picks the branch you already armed in the kernel. The mount in kernel_main.c and a userspace mount("/", "/net", "9p", 0) reach the same code path, which is a reasonable thing for the first boot to confirm.

Table
Row	State
handler bodies	✅ exist, full implementations
dispatch slots	⬜ 3 lines + #include
10_scfs → 03_netfs include path	⬜ confirm $(NETFS_DIR)/include
userspace wrapper arity	⬜ my_syscall3 → my_syscall4
boot	⬜ deferred
Four edits, and then this row is closed. Send the NETFS_DIR grep if you want the path question answered before the build.