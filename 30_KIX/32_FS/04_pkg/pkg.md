Core layer

File	Role	Algorithm / mechanism
uiox_pkg_types.h	Base types	No algorithm. Declares the error enum, version packing macros (VER_MAJOR/MINOR/PATCH via shifts), uiox_pkg_hdr_t (packed, 512-byte aligned), uiox_pkg_dep_t, uiox_pkg_file_t, uiox_pkg_state_t, uiox_pkg_rec_t. Primitives come from uiox_klibc.h, not a local typedef block.

uiox_pkg_buf.h/.c	Record pool = the registry	Linear scan allocation. alloc walks 64 slots for in_use == 0, memsets it, sets the flag, decrements a counter. free asserts in_use > 0, clears the flag and next, increments. O(n) per alloc, no free list.


Store layer
File	Role	Algorithm / mechanism
uiox_pkg_store.h	Store contract	Types only: repo enum, uiox_pkg_index_hdr_t, uiox_pkg_index_entry_t, stats, context (index cache of 256 entries).

uiox_pkg_store.c	Index + archive I/O	Linear index search — index_find scans index[0..index_count) with a bounded strncmp. index_add appends at index_count++. index_remove does swap-with-last: overwrite the found slot with the final entry, shrink, zero the vacated tail — O(1) removal, so index order is not stable. load_pkg allocates from the pool and returns the pointer; caller frees. Two build modes: RAM archive table (default) or the UIOX_PKG_REAL_IO UNFS branch.


Resolver
File	Role	Algorithm / mechanism
uiox_pkg_resolve.h	Plan + graph types	uiox_pkg_plan_entry_t (name, version, op, is_auto), uiox_pkg_plan_t (128 entries + conflict msg), and the resolver context: adjacency matrix adj[64][16], nadj, in_degree, node_name/node_ver.

uiox_pkg_resolve.c	Dependency resolution	DFS graph build + Kahn's topological sort. build_graph recurses into required/optional deps, skipping already-installed ones and CONFLICT entries; add_node dedupes by linear name scan; add_edge records pkg → dep. topo_sort copies in_degree, seeds a FIFO with all zero-in-degree nodes, emits each one and decrements its successors' degrees. If processed ≠ node_count, a cycle exists → sets has_conflicts and returns UIOX_PKG_ERR_CONFLICT. Every loaded record is freed on both exit paths.


Executor
File	Role	Algorithm / mechanism
uiox_pkg_exec.h	Executor contract	Event enum (INSTALL_START…UPGRADE_FAIL), the callback typedef, executor state machine (OFF/INIT/READY/BUSY/ERROR), and the context holding store + resolver + stats.

uiox_pkg_exec.c	Runs the plan	Sequential plan walk. Each public operation resolves a plan, prints it, flips state to BUSY, then run_plan iterates plan->entries[] in order calling exec_plan_entry. That function is a switch on pe->op: INSTALL loads → extracts → marks installed → index_add + index_update; REMOVE loads → removes files → index_remove; UPGRADE removes files → marks UPGRADING → reloads → extracts → marks INSTALLED. A conflict short-circuits before any entry runs. Failure at entry i aborts the remaining entries — no rollback. Records are freed per entry, and events go out by callback on the same call.


The three-way split of work
Concern	File	Decides	Runs
What exists in the repo	store	—	index add/remove/update
What order to do it in	resolve	✓	—
Do it	exec	—	✓
That's the whole shape: types defines, buf stores, store reads and writes, resolve plans, exec executes.
=============================================================
userspace
   │  syscall  SYS_pkg_install(name, ver)
   ▼
40_SystemCallInterface          sys_pkg_install()   — copyin the name
   │
   ▼
32_FS/04_pkg  exec.c            resolve → run_plan → exec_plan_entry
   │                                  │
   │  (A) archive + index            │  (B) extracted files
   ▼                                  ▼
32_FS/04_pkg  store.c           unfs path ops: namei / creat / write / unlink
   │  unfs_bdev_read(dev, blk, buf)  │
   │  unfs_bdev_write(dev, blk, buf) │
   ▼                                  ▼
32_FS/10_unfs  unfs_io.h         performs the 4096 → 512 sector conversion
   │                                  │
   ▼                                  ▼
32_FS/00_bcache                  bread / bwrite  →  uiox_blk_ops_t
   │
   ▼
30_DSS/20_DD   emmc | nvme | sdmmc | sata | usbms   (s_*_blk_ops.read)
   │
   ▼
30_DSS/10_DI   uiox_<cls>_hw.h types + <cls>_fwops.c register writes
   │
   ▼
02_FwHal  →  10_BSP  →  silicon


Three things are missing to make that real, and they're exactly the gaps I flagged in the README:

1. The UNFS bridge in store.c. Five declared-but-empty wrappers:

Wrapper	Should call
pkg_unfs_open_dir(path)	the UNFS directory open (namei → iget)
pkg_unfs_read_sector(dir, lba, buf)	unfs_bdev_read(dev, blk, buf) — note block, not sector
pkg_unfs_write_sector(dir, lba, buf)	unfs_bdev_write(dev, blk, buf)
pkg_unfs_write_file(root, path, data, len)	creat / write through UNFS
pkg_unfs_unlink(path)	unlink through UNFS
That block is the single seam where the UNFS decision is made, which is deliberate — but it's currently a stub, so nothing flows.

2. The root handle field on uiox_pkg_store_t. I wrote store->root = dir in mount(), but the struct has no such field, so even the real-IO build wouldn't compile.

3. The addresses. UIOX_PKG_INDEX_LBA is a name I invented, and UIOX_PKG_ARCHIVE_LBA(name) likewise. The index location is a decision your store has to make — a fixed block in the /pkg area, or resolved through the directory entry.

==============

pkg integration — changes needed, and where
Part A — pkg's own files (all in 32_FS/04_pkg/)
#	File	Change	Why
A1	src/uiox_pkg_store.c	pkg_strncpy(/pkg_strncmp( → uiox_strncpy(/uiox_strncmp( — lines 106, 204, 213, 234	Those names don't exist; uiox_klibc.h:269/275 provides them
A2	include/uiox_pkg_store.h	add uiox_uint32_t dev; and uiox_uint32_t index_blk; to uiox_pkg_store_t	The real branch needs a device and a location; root was my invention
A3	src/uiox_pkg_store.c	define the five pkg_unfs_* wrappers (currently declared only)	They're the seam between pkg and UNFS
A4	src/uiox_pkg_store.c	drop pkg_path_join or wire it to a real helper	Called in the real-IO extract path, defined nowhere
A5	src/uiox_pkg_store.c	store_extract / store_remove_files stay simulated, with a comment naming 01_fsa creat/write	No file-creation API exists above bmap_alloc yet
A6	src/uiox_pkg_store.c	close_evt_child_id — nothing	✅ done: PKG_LOG is the if(0) form at :31
Status: A1 is done (the sed landed). A2–A5 are open. pkg compiles today — four objects in libfs.a, sim path working.

Part B — the gate, in other layers
#	File	Line	Change
B1	01_fsa/include/superblock.h	70	void *s_priv before } SuperBlock; — justified by vfs.h:14
B2	10_unfs/src/unfs_vfs.c	45–48	add #include "superblock.h"
B3	10_unfs/src/unfs_vfs.c	~67	unfs_priv() + three shims, above the table
B4	10_unfs/src/unfs_vfs.c	70–75	three NULL entries → unfs_ops_mount/unmount/sync
B5	10_unfs/src/unfs_vfs.c	92	unfs_register(uint8_t dev, unfs_fs_t *fs)
B6	32_FS/src/uiox_fs_init.c	~30	static unfs_fs_t s_unfs_fs; + #include "unfs_fs.h"
B7	32_FS/src/uiox_fs_init.c	70	mount, then register
All seven are written up in outputs/unfs_mount_a1.patch.md with code and line numbers.

Part C — userspace (outside the kernel)
#	File	Change
C1	uiox_pkg_device.h	uiox_pkg_subsys_t → uiox_pkg_exec_t; include uiox_pkg_exec.h
C2	uiox_pkg_device.c	dev->subsys → dev->exec; all uiox_pkg_subsys_* → uiox_pkg_exec_*
C3	uiox_pkg_device.c	uiox_pkg_query becomes a uiox_pkg_rec_t ** wrapper; caller frees
Part D — build wiring for pkg
#	File	Line	Change	Status
D1	30_KIX/32_FS/Makefile	75	PKG_DIR := $(MFDIR)04_pkg	✅ done
D2	30_KIX/32_FS/Makefile	143	-I$(PKG_DIR)/include	✅ done
D3	30_KIX/32_FS/Makefile	159	FS_SRCS_C += $(wildcard $(PKG_DIR)/src/*.c)	✅ done
D4	30_KIX/common/uiox_kernel_main.c	Stage 3	uiox_pkg_open() after vfs_mount_root()	open
Order of work


B1–B7  (s_priv patch)      ← the gate; nothing below can run without it
   ↓
A2–A5  (pkg real branch)   ← can be written now, exercised after B
   ↓
C1–C3  (userspace pair)
   ↓
D4     (Stage 3 call)      ← needs a mounted FS
Two things still needing your decision
Where does /pkg/index.upix live? UIOX_PKG_INDEX_LBA is a name I invented. Fixed block, or a directory entry found via namei? That decides A2 and A3.

Does uiox_fs_init.c allocate the unfs_fs_t, or does 10_unfs own it? unfs_mount.c:5 says "it does NOT register in a mount table" and "the caller owns the unfs_fs_t" — so the static goes in uiox_fs_init.c, following the s_root_journal pattern. Worth confirming.

Unrelated, still open elsewhere
Item	Where	Note
PCS_SRCS_C missing $(MFDIR)src	33_PCS/Makefile:148	one line; unblocks uiox_proc_init and ~20 more
uiox_arch_main.c outside wildcard	10_BSP/Makefile:257	one line
uiox_sata_if_flush	20_DD/.../sata/	declaration only
Both Makefile lines are from the sweep — neither is pkg, but they're what's failing the link.




