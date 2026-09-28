

The journaling module slots into 32_FileSystem/ as a new 13_journal/ subdirectory.
==========
32_FileSystem/13_journal/ — Complete File Set (9 files)
========
32_FileSystem/13_journal/
├── include/
│   ├── uiox_jrnl_types.h     # Error codes, magic numbers, on-disk structs,
│   │                         #   txn state machine, buf/handle/txn types
│   ├── uiox_jrnl_io.h        # Block I/O abstraction + platform hooks
│   ├── uiox_jrnl_txn.h       # Transaction/handle/buffer lifecycle API
│   ├── uiox_jrnl_recover.h   # Crash recovery: scan → revoke → replay
│   └── uiox_jrnl.h           # Master include, lifecycle API, syscalls
└── src/
    ├── uiox_jrnl_io.c        # CRC-32, SHA-256, block read/write stubs
    ├── uiox_jrnl_txn.c       # Begin/commit/checkpoint/abort +
========
32_FileSystem/13_jrnl/
├── include/
│   ├── uiox_jrnl_types.h      # Error codes, magic numbers, on-disk structs
│   │                          #   (superblock, descriptor, commit, revoke,
│   │                          #    block tag, logged block entry)
│   ├── uiox_jrnl_tx.h         # Handle + transaction types and API
│   ├── uiox_jrnl_recovery.h   # Replay map, recovery stats, recovery API
│   └── uiox_jrnl.h            # Master include — lifecycle, VFS hooks,
│                              #   syscalls, diagnostics
└── src/
    ├── uiox_jrnl_tx.c         # Handle pool, get_write_access, dirty_metadata,
    │                          #   revoke, stop, is_aborted
    ├── uiox_jrnl_recovery.c   # Scan (circular log walk) + replay engine
    └── uiox_jrnl.c            # Init, mount, unmount, abort, tick,
                               #   checkpoint, commit pipeline, VFS hooks,
                               #   syscall handlers, diagnostics
================================
VFS write path (32_FileSystem)
    │
    ├─ uiox_jr_vfs_get_write_access()   ← before modifying inode/dir block
    ├─ uiox_jr_vfs_dirty_metadata()     ← after modifying
    └─ uiox_jr_vfs_revoke()             ← on truncate / unlink

Scheduler tick (33_ProcessControlSubsystem)
    └─ uiox_jr_tick()                   ← commits if interval elapsed (5 s)

Mount / unmount path
    ├─ uiox_jr_mount()  → uiox_jr_recover()   ← scan + replay on dirty mount
    └─ uiox_jr_unmount() → force_commit + checkpoint + clean superblock

Syscall table (40_SystemCallInterface)
    ├─ SYS_SYNC      (162) → uiox_jr_force_commit + checkpoint
    ├─ SYS_FSYNC      (74) → uiox_jr_force_commit
    ├─ SYS_FDATASYNC  (75) → uiox_jr_force_commit (data only)
    └─ SYS_SYNCFS    (306) → same as SYS_SYNC
=========================
Transaction state machine
INACTIVE
   │  uiox_jr_start()
   ▼
RUNNING  ← get_write_access / dirty_metadata / revoke
   │  all handles stopped / log full / tick
   ▼
LOCKED
   │  descriptor block written
   ▼
FLUSH    ← data blocks written to log
   │
   ▼
COMMIT   ← commit block written → fsync guarantee met
   │
   ▼
CHECKPOINT ← blocks written back to home location
   │
   ▼
INACTIVE
=======================================
13_journal — Filesystem Journal
Must be kernel — belongs in 32_FS, not 50_UIX.

Implements filesystem journaling: mount/unmount, transaction state machine (INACTIVE → RUNNING → COMMIT → CHECKPOINT)
Responds to syscalls SYS_SYNC (162), SYS_FSYNC (74), SYS_FDATASYNC (75), SYS_SYNCFS (306) — syscall handlers run in kernel space only
uiox_jr_tick() is called from the scheduler tick — kernel context
Has on-disk structures (superblock, descriptors) written directly — no libc, no POSIX layer
Where it should move: out of 50_UIX and into 30_KIX/32_FS/ as a sub-module. It's a filesystem subsystem, not a UIX-layer concern. The 50_UIX numbering is misleading — this is purely 32_FS internals.
=====================================================================
analysis — and the key finding is that the journal is completely decoupled from your filesystem, which changes how it should be linked.

What you gave me
File	Lines	Purpose
uiox_jrnl_types.h	249	Base types, error codes, on-disk structs, magics
uiox_jrnl_tx.h	325	Handle + transaction + context structs, tx API
uiox_jrnl_recovery.h	—	Replay map, stats, uiox_jr_recover/scan/replay
uiox_jrnl.h	169	Master header, lifecycle, syscalls, VFS hooks
uiox_jrnl.c	505	Lifecycle, commit, checkpoint, VFS hooks
uiox_jrnl_tx.c	181	Handle pool, start/get_write_access/dirty/stop
uiox_jrnl_recovery.c	201	Scan + replay
uiox_jrnl_io.h	—	The weak I/O hooks
The finding: zero coupling to UNFS
Every journal file depends on exactly one external symbol:



uiox_jrnl.c           uiox_fw_printf × 33
uiox_jrnl_tx.h        uiox_fw_printf × 21
uiox_jrnl_types.h     uiox_fw_printf × 2
uiox_jrnl_tx.c        (none)
uiox_jrnl_recovery.c  (none)
No bread, no bwrite, no unfs_, no BufHdr. The engine talks to storage only through two weak hooks:

c


__attribute__((weak)) uiox_jr_plat_log_read (log_dev_base, log_blocknr, buf);
__attribute__((weak)) uiox_jr_plat_log_write(log_dev_base, log_blocknr, buf);
__attribute__((weak)) uiox_jr_plat_fs_write (fs_blocknr, buf);
And by default they're memory-mapped stubs:

c


uint8_t *src = (uint8_t *)(uintptr_t)(log_dev_base + log_blocknr * UIOX_JR_BLOCK_SIZE);
rc_memcpy(buf, src, UIOX_JR_BLOCK_SIZE);
So the journal addresses memory directly, the same way bcache_plat_* does. That's why it has no dependency — and why it can be linked or not linked without breaking anything.

Where it goes
There's a 02_jrnal/ directory already, and your Makefile v2.2.0 lists it in the "not built here" block. It also has its own src/ files you saw earlier in the grep:



02_jrnal/src/uiox_jrnl.c
02_jrnal/src/uiox_jrnl_recover.c
02_jrnal/src/uiox_jrnl_txn.c
Note the names differ from what you're sending: _recover.c vs _recovery.c, _txn.c vs _tx.c. Check which set is actually on disk — the grep earlier found the first spellings.

The three-way split, and why it matters


┌─────────────────────────────────────────────────────────────┐
│ 02_jrnal/          THE JOURNAL ENGINE                        │
│   types.h, tx.h, recovery.h, io.h                            │
│   uiox_jrnl.c, uiox_jrnl_tx.c, uiox_jrnl_recovery.c         │
│                                                              │
│   Depends on: uiox_fw_printf, and 3 weak platform hooks      │
│   Depends on UNFS:  NOTHING                                  │
└─────────────────────────────────────────────────────────────┘
                          │
        ┌─────────────────┼─────────────────┐
        ▼                 ▼                 ▼
   01_uBoot       31_BufferCache /      (a future BSP)
                   00_buffcache         writes its own
                    provides the        hooks
                    weak hooks
The five things that must be supplied before it links
Symbol	Where it should come from	Status
uiox_fw_printf	the SoC stdio / firmware layer	❌ not defined anywhere — extern, used 56 times across three files
uiox_jr_plat_log_read	a platform, or the built-in mmap stub	✅ weak stub exists
uiox_jr_plat_log_write	same	✅ weak stub exists
uiox_jr_plat_fs_write	same	⚠️ weak stub that does nothing — returns UIOX_JR_OK without writing
uiox_jr_checkpoint	uiox_jrnl.c	need to confirm it's there
uiox_fw_printf is the blocker. It's declared extern in all three .c files and defined in none. Nothing in your tree provides it — this is exactly the early_puts situation again, where a symbol is used but never declared.

The bigger issue is uiox_jr_plat_fs_write. Its stub:

c


uiox_jr_err_t uiox_jr_plat_fs_write(uint64_t fs_blocknr, const void *buf)
{
    (void)fs_blocknr; (void)buf;
    return UIOX_JR_OK;        /* writes NOTHING, reports success */
}
Recovery calls this to replay committed blocks back to the filesystem, and it silently discards them. A journal that appears to recover cleanly while writing nothing is worse than no journal — this is the same class of bug as bcache_plat_write_block dropping past-end writes, but here the entire replay path is a no-op.

How to make it link — and the honest recommendation
Option A — don't build it yet. Leave 02_jrnal out of FS_SRCS_C. The filesystem works without it: unfs_format.c writes a volume, unfs_mount.c mounts it, 10_scfs serves the syscalls. The journal is a durability feature, not a requirement.

Option B — build it as a standalone engine. Add it to the Makefile, supply uiox_fw_printf (trivial — map it to uiox_printf from the SoC stdio header), and let the mmap stubs stand. It would compile and the log/unmount path would work against memory. But recovery would silently do nothing, so it would be a demonstration rather than a working journal.

Option C — wire it to bcache properly. Write real hooks:

c


uiox_jr_err_t uiox_jr_plat_log_read(uint64_t base, uint32_t blk, void *buf)
{
    /* one UNFS block = 8 bcache sectors */
    ...
}
uiox_jr_err_t uiox_jr_plat_fs_write(uint64_t fs_blocknr, const void *buf)
{
    /* write one 4096-byte block back to the filesystem */
}
That's the real work, and it has the same 4096↔512 conversion that unfs_io.c already handles.

My recommendation
Option A. Don't link it until you decide the journal question, and there's a specific reason:

The journal's on-disk layout assumes a journal area that your format no longer has. uiox_jr_init(ctx, log_dev_base, log_blocks, mode, …) takes an explicit log base and size. But:

unfs_format.h reserves blocks 1–257 with no constants for them — the header says "No constant is defined for the range: a reserved span nobody touches needs no name."
UNFS_JR_MIN_LOG_BLOCKS is 1024 — four times the 256 blocks reserved.
uiox_jr_init wants a log_dev_base pointing at a separate device or area, while UNFS's reserved range is inside the volume.
unfs_mkfs_params_t.journal_blocks = 256 — the userspace file I analysed earlier — is 768 blocks short of the journal's own minimum.
So the journal engine and the UNFS format were written against different geometries. Linking them would produce something that compiles and then fails at uiox_jr_init with UIOX_JR_ERR_INVAL.

And separately: you parked COW on the scheduler, and the journal's commit model is explicitly time-driven — commit_interval_ms default 5000, uiox_jr_tick() "called from scheduler or timer ISR". Same dependency, same reason to wait.

The three concrete questions to settle first
Where does the journal log live? A separate device, or blocks 1–257 of the volume? UNFS_JR_MIN_LOG_BLOCKS = 1024 says separate; the reserved range says in-volume.
Who defines uiox_fw_printf? It's the one hard symbol blocking a compile.
Does uiox_jr_plat_fs_write get a real body? Without it, recovery is a no-op that reports success.
Answer those and I'll write the glue. Answer them as "later" and the right move is Option A — leave 02_jrnal unbuilt, exactly as your Makefile already does, and the filesystem stays whole.

=========================================================
The three journal questions, recorded for later
Worth keeping with the files, since they're what a future pass starts from:

Where does the log live? UNFS_JR_MIN_LOG_BLOCKS is 1024, but unfs_format.h reserves blocks 1–257 with no constants, and unfs_mkfs_params_t.journal_blocks defaults to 256. Three numbers, no agreement.
Who defines uiox_fw_printf? Used 56 times across three .c files, defined nowhere. The one hard symbol blocking a compile.
uiox_jr_plat_fs_write needs a real body. Its stub returns UIOX_JR_OK while writing nothing — recovery would report clean and replay nothing.
What's actually left, in priority order
1	Run the self-test. unfs_selftest.c is written and parked. Everything type-checks; nothing has executed. The group offsets in unfs_alloc.c and the split in bmap_extent_place case 2 are where a silent bug would live.
2	Extent-tree spill (case 4) — fragmented past four inline extents → UNFS_ENOTSUP. The reader exists; the writer doesn't.
3	unfs_cow_block repoint — one call plus a signature change; waits on the scheduler by your call.
4	The ioctl bridge — nine UNFS_IOC_* commands that 50_UIX's unfs_lib.c sends and no kernel handler answers. Makes xattr and fsinfo reachable immediately.
That's a coherent stopping point: the filesystem is whole, two subsystems are correctly parked, and the remaining work is either verification or additive.

Ping me when the scheduler lands — that's what unblocks both COW and the journal, and the glue is small once they're unblocked.


