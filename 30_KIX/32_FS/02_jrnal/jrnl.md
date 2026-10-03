

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


==========================================

Files in 02_jrnal — the seven
Two written as complete files, five carrying edits:



include/uiox_kix_jrnl_types.h      full file I authored
include/uiox_kix_jrnl_tx.h         pool field (you added) + accessor prototype
include/uiox_kix_jrnl.h            registration API + fixed include
include/uiox_kix_jrnl_recover.h    rename only
src/uiox_kix_jrnl_tx.c             full file I authored
src/uiox_kix_jrnl_recover.c        fixed include at line 6
src/uiox_kix_jrnl.c                ORDERED refusal + two lb->data regions
Files outside 02_jrnal for journal functionality — five
These are the wiring. Without them the journal compiles but is never called:

File	Change	Why the journal needs it
01_fsa/src/inode.c	inode_block_of helper; inode_disk_read gains out_blkno; iupdate brackets its fill	journals the inode block — the hook that makes bmap_alloc's window atomic
01_fsa/include/inode.h	inode_block_of prototype	declared so bmap.c and the journal can call it
01_fsa/src/namei.c	dir_add / dir_remove bracket their block writes	journals the directory block
01_fsa/src/readwrite.c	writei_at brackets under MODE_DATA	journals data blocks — the only mode-dependent hook
10_unfs/src/unfs_alloc.c	bitmap pre-image captured before bit_set_used	journals the block bitmap — the other half of the allocation pair
32_FS/src/uiox_fs_init.c	three drafts consolidated, uiox_jr_register added	the call that makes uiox_jr_ctx_for(dev) return anything
32_FS/Makefile	JRNL_DIR, sources, vpath, -I paths	parked at your instruction
So that's six files outside the module, not zero. Five source edits plus the Makefile.

What is not touched, and shouldn't be
bmap.c — I initially proposed a hook there and then established it was unnecessary. readwrite.c:235 calls iupdate(ip) in the same write path, so patch 1's iupdate hook covers the allocation window. bmap.c gets a comment, not code.

And two things the journal still needs that no file provides
uiox_jr_force_commit has no body. I wrote uiox_kix_jrnl.c with four call sites for it — uiox_jr_stop, uiox_jr_tick, uiox_jr_unmount, and the commit path — but I never read the original's body, so I didn't invent one. Its data half is jr_write_logged_blocks, which I did write.

MAX_DEVICES — uiox_kix_jrnl.c uses it for s_jr[MAX_DEVICES], and it comes from fs_types.h. If it isn't defined there, that's one line.

The honest summary
Journal functionality spans thirteen files: seven in the module and six outside it. Two of the seven I wrote whole; the rest are patches. Nothing has been compiled, so every claim about correctness rests on reading — and the one thing verified in this whole sequence is the rename, because your grep came back empty.




can you show me what cahnges you have upadted/applied to complete journals and why?File Change Why the journal needs it
01_fsa/src/inode.c inode_block_of helper; inode_disk_read gains out_blkno; iupdate brackets its fill journals the inode block — the hook that makes bmap_alloc's window atomic
01_fsa/include/inode.h inode_block_of prototype declared so bmap.c and the journal can call it
01_fsa/src/namei.c dir_add / dir_remove bracket their block writes journals the directory block
01_fsa/src/readwrite.c writei_at brackets under MODE_DATA journals data blocks — the only mode-dependent hook
10_unfs/src/unfs_alloc.c bitmap pre-image captured before bit_set_used journals the block bitmap — the other half of the allocation pair
32_FS/src/uiox_fs_init.c three drafts consolidated, uiox_jr_register added the call that makes uiox_jr_ctx_for(dev) return anything
32_FS/Makefile JRNL_DIR, sources, vpath, -I paths


Here are the six outside-the-module changes, each written out, with the reason it's needed.

1 — 01_fsa/include/inode.h — the prototype
Added, beside the other inode API:

c


/*
 * The block holding inode @ino on @dev.  DERIVED, not stored — and
 * derived in exactly one place.  inode_disk_read computes this to find
 * the buffer, and the journal needs the same number to name the block it
 * logs.  A second copy of the formula would be a second definition of a
 * derived value.
 *
 * Returns 0 for an out-of-range inode, which is never a valid block.
 */
uint32_t inode_block_of(uint8_t dev, uint32_t ino);
Why: iupdate and the commit path both need to tell the journal which block they're writing. That number is computed inside inode_disk_read at line 188 and thrown away. Without exporting it, each caller would re-derive the formula — and a drift between the two copies would journal the wrong block while the checksum still matched.

2 — 01_fsa/src/inode.c — three changes
2a. The helper, lifted
c


uint32_t inode_block_of(uint8_t dev, uint32_t ino)
{
    (void)dev;      /* the geometry is per-volume */

    if (ino == 0u || ino > (UNFS_ITABLE_BLOCKS * UNFS_INODES_PER_BLOCK))
        return 0u;

    return ((ino - 1u) / UNFS_INODES_PER_BLOCK) + UNFS_GROUP0_ITABLE;
}
2b. inode_disk_read gives the number back
c


DiskInode *inode_disk_read(uint8_t dev, uint32_t ino,
                           BufHdr **out_buf, uint32_t *out_blkno)
and inside, the local computation becomes:

c


    blkno = inode_block_of(dev, ino);
    if (blkno == 0u) return (DiskInode *)0;

    if (out_blkno) *out_blkno = blkno;

    offset = ((ino - 1u) % UNFS_INODES_PER_BLOCK) * (uint32_t)sizeof(DiskInode);
2c. iupdate brackets its fill
c


void iupdate(InCoreInode *ip)
{
    BufHdr    *buf;
    DiskInode *di;
    uint32_t   blkno = 0u;
    uiox_jr_ctx_t *jr;

    if (!ip) return;

    di = inode_disk_read(ip->dev, ip->ino, &buf, &blkno);
    if (!di) return;

    /* BEFORE the fields move: this is the on-disk PRE-IMAGE. */
    jr = uiox_jr_ctx_for(ip->dev);
    if (jr)
        uiox_jr_vfs_get_write_access(jr, blkno, buf->data);

    di->i_mode  = ip->mode;
    di->i_nlink = ip->nlink;
    di->i_uid   = ip->uid;
    di->i_gid   = ip->gid;
    di->i_size  = ip->size;
    memcpy(di->i_extents, ip->i_extents, sizeof ip->i_extents);
    di->i_extent_tree = ip->i_extent_tree;
    di->i_atime_ns = (uint64_t)ip->atime;
    di->i_mtime_ns = (uint64_t)ip->mtime;
    di->i_ctime_ns = (uint64_t)ip->ctime;
    di->dev        = ip->dev;

    /* AFTER, still ours: the post-image. */
    if (jr)
        uiox_jr_vfs_dirty_metadata(jr, blkno, buf->data);

    bwrite(buf, true, false);

    ip->flags &= (uint8_t)~(IFLAG_ACCESSED | IFLAG_CHANGED | IFLAG_MODIFIED);
}
Why: this is the hook that makes bmap_alloc's window atomic. bmap.c:397 warns the block is "allocated but unnamed" between the bitmap clear and the extent placement — and readwrite.c:235 calls iupdate(ip) in the same write path, which persists the extent map. Journalling the inode block makes that pair one unit. Without it, a crash in the window leaks the block permanently: nothing on disk refers to it.

3 — 01_fsa/src/namei.c — the dirent pair
In dir_add and dir_remove, around each block's modification:

c


    BufHdr *buf = bread(dir->dev, sect);

    uiox_jr_ctx_t *jr = uiox_jr_ctx_for(dir->dev);
    if (jr)
        uiox_jr_vfs_get_write_access(jr, dir_block_of(dir, sect), buf->data);

    ... modify buf->data ...

    if (jr)
        uiox_jr_vfs_dirty_metadata(jr, dir_block_of(dir, sect), buf->data);

    bwrite(buf, true, false);
With the conversion helper:

c


static uint32_t dir_block_of(const InCoreInode *dir, uint32_t sect)
{
    (void)dir;
    return sect / (uint32_t)UNFS_SECTORS_PER_BLOCK;
}
Why: a dirent update is the third kind of metadata this filesystem writes. mkdir, unlink, rename, mknod all change a directory block, and a crash mid-update leaves a name that resolves to a half-written entry. The dir_block_of conversion matters because bread takes 512-byte sectors and the journal takes 4096-byte UNFS blocks — passing the sector straight through would journal the wrong block by a factor of eight.

4 — 01_fsa/src/readwrite.c — the data hook
In writei_at, inside the block loop:

c


        buf = bread(bm.dev, bm.blkno);
        if (!buf) break;

        jr = NULL;
        if (uiox_jr_ctx_for(ip->dev) &&
            uiox_jr_ctx_for(ip->dev)->mode == UIOX_JR_MODE_DATA)
            jr = uiox_jr_ctx_for(ip->dev);

        if (jr)
            uiox_jr_vfs_get_write_access(jr, bm.blkno, buf->data);

        for (i = 0u; i < n; i++)
            buf->data[bm.blk_offset + i] = (uint8_t)kbuf[done + i];

        if (jr)
            uiox_jr_vfs_dirty_metadata(jr, bm.blkno, buf->data);

        bwrite(buf, true, false);
Why: this is the only mode-dependent hook. Under METADATA, a crash can leave the inode pointing at blocks whose contents were never written — a file whose size says 4 KB and whose bytes belong to the previous occupant. MODE_DATA closes that by journalling the contents too, at the cost of doubling the I/O. The guard means METADATA and ORDERED mounts pay nothing but a pointer test.

5 — 10_unfs/src/unfs_alloc.c — the bitmap
In unfs_alloc_run, capturing the pre-image before the bits move:

c


        if (unfs_bdev_read(dev, bmap_blk, bmbuf) != UNFS_OK) return UNFS_EIO;

        if (jr) memcpy(prebuf, bmbuf, UNFS_BLOCK_SIZE);   /* ← before */

        if (!find_run(bmbuf, limit, want, &start)) continue;

        for (k = 0u; k < want; k++) bit_set_used(bmbuf, start + k);

        if (jr) {
            uiox_jr_vfs_get_write_access(jr, bmap_blk, prebuf);
            uiox_jr_vfs_dirty_metadata(jr, bmap_blk, bmbuf);
        }

        if (unfs_bdev_write(dev, bmap_blk, bmbuf) != UNFS_OK) return UNFS_EIO;
Why: this is the other half of the allocation pair. unfs_alloc_run clears bits in the group bitmap and decrements s_free_blocks; the function's own comment names the ordering and reasons about crash windows — but doesn't journal them. With patch 2's iupdate hook this is the pair that leaks, and it's the one the journal exists to bracket. The pre-image has to be captured before bit_set_used modifies bmbuf, or the read-back is the post-image.

6 — 32_FS/src/uiox_fs_init.c — consolidate and register
The three drafts collapse into one body, then:

c


    /* 4. Journal init. */
    jrc = uiox_jr_init(&s_root_journal, 0u,
                       UIOX_JR_MIN_LOG_BLOCKS,
                       UIOX_JR_MODE_METADATA,
                       (uint64_t (*)(void))0);
    if (jrc != UIOX_JR_OK) {
        early_puts("[fs] journal init failed\n");
    } else {
        /* 5. Mount — runs recovery.  Must precede step 6. */
        jrc = uiox_jr_mount(&s_root_journal, (uiox_jr_recovery_stats_t *)0);

        if (jrc == UIOX_JR_OK) {
            /* 6. REGISTER. */
            jrc = uiox_jr_register(&s_root_journal, ROOT_DEV);
        }
    }
Why: uiox_jr_register is what puts the context in s_jr[MAX_DEVICES], and uiox_jr_ctx_for(dev) is what every hook calls to find it. Without this one call, all five hooks above test NULL and do nothing — the journal would compile, link, and never record a byte. The order matters: recovery must run before registration, or a hook would log into a log whose tail is still dirty from the last crash.

And the consolidation itself is required because the file held three definitions of uiox_fs_init. Three in one translation unit is a compile error, and two of them used printf(), which -ffreestanding bans.

7 — 32_FS/Makefile — parked
makefile


JRNL_DIR := $(MFDIR)02_jrnal
FWHAL    := $(abspath $(MFDIR)../../02_FwHal)

FS_SRCS_C += $(wildcard $(JRNL_DIR)/src/*.c)
FS_SRCS_C += $(wildcard $(MFDIR)src/*.c)        # uiox_fs_init.c — never in the build

vpath %.c  ... $(JRNL_DIR)/src $(MFDIR)src

COMMON_INCLUDES := ... -I$(JRNL_DIR)/include -I$(FWHAL)/include
Why: two reasons. The journal's three sources aren't in FS_SRCS_C at all, so they'd never compile into libfs.a. And 02_FwHal/include supplies uiox_fw_printf, which every journal .c file declares extern — without that path, all three fail at the first call.

Two things these six don't cover
uiox_jr_force_commit has no body. I wrote uiox_kix_jrnl.c with four call sites for it, and its data half (jr_write_logged_blocks) beside them — but I never read the original's body, so I didn't invent one.

Nothing has been compiled. All six edits plus the seven module files are text. The compile is what would tell you whether they're coherent; reading has only told us what's in the files.

=================

Two things these six don't cover
uiox_jr_force_commit has no body. I wrote uiox_kix_jrnl.c with four call sites for it, and its data half (jr_write_logged_blocks) beside them — but I never read the original's body, so I didn't invent one.

Nothing has been compiled. All six edits plus the seven module files are text. The compile is what would tell you whether they're coherent; reading has only told us what's in the files.

====================
01_fsa/src/inode.c 
01_fsa/include/inode.h 

01_fsa/src/namei.c 

01_fsa/src/readwrite.c 

10_unfs/src/unfs_alloc.c 

32_FS/src/uiox_fs_init.c 

32_FS/Makefile J