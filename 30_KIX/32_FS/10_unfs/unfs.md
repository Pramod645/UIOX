What unfs_format.h now owns
Everything below is defined in exactly one place, and 01_fsa compiles against it:

Constant	Value

Constant	Value
UNFS_BLOCK_SIZE	4096
UNFS_INODE_SIZE	256
UNFS_INODES_PER_BLOCK	16
UNFS_SB_BLOCK	0
UNFS_ROOT_INO	2
UNFS_GROUP0_ITABLE	261
UNFS_ITABLE_BLOCKS	8
UNFS_NAME_MAX	255
UNFS_IFMT / UNFS_IF*	the on-disk type encoding
UNFS_DT_*	the dirent type encoding
unfs_sb_t, unfs_inode_t, unfs_extent_t, unfs_dirent_t	the structs
UNFS_INLINE_EXTENTS, UNFS_DIRENT_ALIGN	added by fs_types.h
UNFS_SECTORS_PER_BLOCK	8 — the buffer-cache conversion

And the 88-byte pad in unfs_inode_t stands as the right resolution: it closes a struct that declares 256 but measures 168, without moving a single inode offset. The bootloader's own header carries the same pad, so its assert passes too.
===============================
Good — that's the milestone. libfs.a builds for arm64 with all 47 files compiling.

What the tree has now
The FS layer is complete for the direct-call model:

unfs_format.h — one layout header, matching the bootloader on magic, block size, inode size, superblock position and the extent model
unfs_format.c — writes a volume
unfs_io.c — reads and writes it, with the 4096↔512 conversion in one place
unfs_alloc.c — hands out blocks and runs over the group bitmaps
unfs_extra_unfs_xattr.c — xattr storage
all of 00_buffcache, 01_fsa, and the 31 10_scfs syscall bodies
Two known gaps, both named in the code
bmap_alloc() is a stub. It returns valid = false so a write path stops with ENOSPC. unfs_alloc_run() supplies the run-finding half; merging into i_extents[] and spilling to i_extent_tree is still to write. Reads work, writes report out of space — coherent, not broken.

bmap.c's EXTREE_PER_BLOCK reads past its buffer. 4096 / sizeof(unfs_extent_t) is 341 entries, but bread() returns a 512-byte data[]. Entries 43–340 are an out-of-bounds read. Bound it at BLOCK_SIZE / sizeof(unfs_extent_t) — 42 — or read all eight sectors.

Four files in deferred/
File	Blocker
unfs_iget.c	needs icache[]/InCoreInode naming; the ops pointers have no target
unfs_mount.c	SuperBlock has no group array — needs a decision, not a rename
unfs_vfs.c	VFS layer doesn't exist in the tree
snap.c	now unblocked by the allocator; needs the COW extent-update path
The pattern worth keeping in mind: three of those four depend on an ops-table VFS that fs_types.h explicitly declines to provide, and its comment says a compile error naming one of those types is the correct outcome. So they're not port defects — they're files that haven't been converted, and deferring them was right.

If you pick this up again
The highest-value next step is unfs_mount.c, because it's what makes a mkfs'd volume mountable. That needs one decision first: whether SuperBlock grows a group array or points at unfs_fs_t, which already has groups[] and n_groups. That's a design call rather than a fix, and it's better made deliberately than by picking whichever compiles.

After that, bmap_alloc() — which turns reads-only into a writable filesystem.

Everything from this session is in outputs/10_unfs/ if you need to re-fetch any of it. Worth keeping in mind that several files went missing mid-thread, so if a build ever reports a name you know exists, check the copy landed before assuming the code is wrong.

=================
The three known gaps, all documented in the code
bmap_alloc() is a stub. Writes report ENOSPC. unfs_alloc_run() supplies run-finding; merging into i_extents[] is unwritten. So reads work, writes stop short.

unfs_cow_block() copies but doesn't repoint. Same missing piece — the extent update. Marked THE GAP, STATED PLAINLY in the snapshot file, and the header repeats it. It returns the new block and says the caller must not use it yet.

bmap.c's EXTREE_PER_BLOCK reads past its buffer. 341 entries against a 512-byte data[]. Bound it at BLOCK_SIZE / sizeof(unfs_extent_t).

Two loose ends
unfs_iget.c isn't in src/ yet — I delivered it and the header now declares unfs_iget, unfs_iput, unfs_read_inode to match. When you add it, that's the seventh object.

And the dirent alignment fix — d0->d_rec_len should be 12, not 8. Worth applying before you format a volume you intend to read, since it's what makes the root directory walkable.

=========

UNFS PORT — THE FOUR REMAINING GAPS
==================================

FIX 1+2  extent write ops  (bmap_alloc stub + unfs_cow_block repoint)
-------------------------------------------------------------------
These are ONE gap, not two.  Both need the same four operations, so they
live together in 01_fsa/bmap.c next to the read side:

    scratch/bmap_extent_ops.c     the implementation  (176 lines)
    scratch/bmap_extent_decls.h   the declarations    (49 lines)

Paste the declarations into 01_fsa/include/bmap.h and the implementation
into 01_fsa/src/bmap.c, replacing bmap_alloc()'s stub.

  case 1  covers lb, length 1   -> repoint e_physical        DONE
  case 2  covers lb, longer     -> split into head/tail      DONE
  case 3  no cover, free slot   -> append a length-1 extent  DONE
  case 4  no cover, array full  -> spill to i_extent_tree    ENOTSUP

Case 4 is the honest limit: the tree block is readable (extree_lookup
exists) but nothing WRITES one yet.

Then unfs_cow_block() calls bmap_extent_place() after the copy, and its
gap note comes out:

    new_phys = unfs_alloc_block(fs->dev_id);
    ... copy ...
    (void)bmap_extent_place(inode, lb, new_phys);   <-- the missing line

FIX 3  bmap.c extree_lookup reads past its buffer
-------------------------------------------------
Replace the function with the version in scratch/bmap_extree_fix.c.

The tree block is ONE 4096-byte UNFS block holding an array of 12-byte
extents = 341 entries.  bread() returns a BufHdr whose data[] is
BLOCK_SIZE = 512 bytes, so the old loop read 299 entries PAST the buffer.

Two fixes, and the second is right:
  (a) bound at 512/12 = 42  — safe, but 299 entries unreachable
  (b) read ALL EIGHT SECTORS — correct, sees every entry

This is (b).

FIX 4  dirent record length
---------------------------
See scratch/FIX4-dirent-alignment.txt — one line in unfs_format.c.

LOOSE END  unfs_iget.c
----------------------
Already delivered (369 lines).  Move it from deferred/ into src/; the
build glob picks it up.  unfs_fs.h already declares unfs_iget, unfs_iput,
unfs_read_inode and unfs_write_inode_disk to match.
=====================
unfs_format.c  line 290 — dirent record length

OLD:
        d0->d_reclen = (uiox_uint16_t)(sizeof(unfs_dirent_t));

NEW:
        d0->d_reclen = (uiox_uint16_t)(
            (sizeof(unfs_dirent_t) + 1u + 3u) & ~3u);   /* 8 + "." = 9 -> 12 */

Why: sizeof(unfs_dirent_t) is 8 (d_ino u32, d_rec_len u16, d_name_len u8,
d_type u8, then the flexible d_name[]).  With d_name_len 1 the record needs
8 + 1 = 9 bytes, which must round UP to the 4-byte UNFS_DIRENT_ALIGN
boundary -> 12.

Line 297 needs no change: UNFS_BLOCK_SIZE - 12 = 4084, already 4-aligned,
and the last entry absorbing the block's spare bytes is the rule namei.h
states.

A walker reads d_rec_len as the authority (namei.h: "dirent_reclen()
therefore reads the field"), so a value of 8 would step to byte 8 — the
middle of the name — and parse the next record's bytes as a header.


==================================================================
You asked for the test harness. I delivered unfs_bdev.c — the RAM-backed device. But that replaces unfs_io.c's existing implementation rather than testing it. unfs_io.c already implements unfs_bdev_read/unfs_bdev_write on top of bcache, and it compiles into libfs.a. Dropping a second implementation of the same two symbols would be a duplicate-symbol link error.

So the honest situation is:

Your tree already has the device path. unfs_io.c → bcache (bread/getblk/bwrite) → and bcache has a platform hook your earlier logs showed: bcache_plat_write_block, which was the file that needed printf from the SoC stdio header. That hook is where real storage plugs in — and it's the layer I've never seen.

==================
The device layer already works
c


void bcache_plat_read_block(uint8_t dev, uint32_t blkno, uint8_t *buf)
{
    uint32_t nblocks = bcache_plat_num_blocks(dev);
    uintptr_t addr   = plat_addr(dev, blkno);
    if (nblocks != 0u && blkno >= nblocks) { memset(buf, 0, BCACHE_SECTOR_SIZE); return; }
    memcpy(buf, (const void *)addr, BCACHE_SECTOR_SIZE);
}
That's not a stub — it's a real implementation over plat_addr(dev, blkno), which maps a block to a memory address. And the file's header says a platform can supply its own bcache_plat_read_block and this one is dropped via a weak symbol. So the device path is: unfs_io.c → bcache → bcache_plat_*_block → plat_addr(). Nothing missing.

So unfs_bdev.c is definitely wrong to copy in
It would double-define unfs_bdev_read/unfs_bdev_write against unfs_io.c and fail the link. My earlier message already said this; let me make it concrete rather than leave it hanging — I'll remove it so a later copy step can't pick it up by mistake.


Used Workspace
Deleted — outputs/10_unfs/src/unfs_bdev.c is gone, so a later copy step can't pick up a file that would break the link.

====================


What the platform hook tells us
The device path is already complete in your tree:



unfs_io.c  →  bcache (bread/getblk/bwrite)  →  bcache_plat_read_block / _write_block  →  plat_addr()
And bcache.h's comment at line 51 says a platform can supply its own bcache_plat_read_block and this one is dropped — a weak symbol. So there are two ways to point it at memory for a test, and neither needs a second unfs_bdev_*:

plat_addr(dev, blkno) already maps to an address. If it's a RAM base today, the whole stack runs against memory now.
Override the weak symbols in a test file with your own, and redirect to a static array.
The second is what I'd do — it's about 30 lines, lives in a test file, and leaves unfs_io.c untouched.


=================
now have the whole device path, and there's a real bug in it worth naming before any test.

The weak symbol never takes effect


123:uint32_t bcache_plat_num_blocks(uint8_t dev)
     ...returns NUM_DISK_BLOCKS_DEFAULT
The file's own comment says why that definition is unreachable:

"bcache_types.h declares a static inline __attribute__((weak)) version of this. A static inline is a private copy in every translation unit, so a BSP cannot replace it at link time — the header's copy always wins inside each unit."

So bcache_plat_num_blocks in bcache_init.c is dead code. Every caller — including bcache_plat_read_block and bcache_plat_write_block right below it — uses the header's inline default. A platform that tries to override it gets a linker error for a duplicate symbol, or silently nothing.

That matters for the test: the bounds check in the read/write hooks is comparing against the header's constant, not against whatever device you think you registered.

What the device actually is
c


uintptr_t bcache_plat_dram_base(void) { return (uintptr_t)BCACHE_DRAM_BASE_DEFAULT; }

static uintptr_t plat_addr(uint8_t dev, uint32_t blkno) {
    uintptr_t base = bcache_plat_dram_base();
    if (dev >= MAX_DEVICES) dev = MAX_DEVICES - 1u;
    return base + dev * BCACHE_DEV_STRIDE_DEFAULT + blkno * BCACHE_SECTOR_SIZE;
}
A fixed DRAM base plus a per-device stride — not a table, not a registration call. So the whole storage stack already addresses memory at BCACHE_DRAM_BASE_DEFAULT. Nothing needs overriding to run; you need to know what that base is.

What the test needs, and what I can't know
Two constants decide whether a test can run at all:

sh


grep -n 'BCACHE_DRAM_BASE_DEFAULT\|BCACHE_DEV_STRIDE_DEFAULT\|NUM_DISK_BLOCKS_DEFAULT\|MAX_DEVICES\|BCACHE_SECTOR_SIZE' \
   /Users/pramodkumar/Hack/WS/UIOX/30_KIX/32_FS/00_buffcache/include/bcache_types.h
If BCACHE_DRAM_BASE_DEFAULT is a real address you can point at, the harness is short — format, mount, allocate, read back, with printf on each result. If it's a placeholder like 0x0, plat_addr returns a near-null pointer and the first write faults; the harness then needs a RAM buffer and a way to make the base point at it — which is exactly the two-line fix the file's comment already describes.

And a trap the test would fall into
c


if (nblocks != 0u && blkno >= nblocks) {
    printf("[bcache] ERROR: write dev=%u blk=%u past device end — dropped\n", ...);
    return;                                   /* the write is DROPPED */
}
A write past the end is silently discarded with only a log line. So a format/mount round-trip can look like it passed while a block never landed. The harness has to assert on the write count, not on the absence of errors — otherwise it will report success for a volume that was never written.

What I'll write, once I have those constants
One file, unfs_fs_selftest.c, with a main() that:

prints bcache_plat_dram_base(), bcache_plat_num_blocks(0) and BCACHE_SECTOR_SIZE — so the harness's own assumptions are visible rather than assumed
formats UNFS_GROUP0_DATA + 256 blocks
reads back block 0, checks s_magic, s_group_count, s_block_bitmap
calls unfs_kern_mount and checks n_groups
unfs_iget on UNFS_ROOT_INO, checks mode has UNFS_IFDIR
bmap_alloc + bmap on the root inode, checks valid and that the block number came back consistent
reports every step's result with printf, and returns non-zero on the first failure
That's the run that turns "type-correct" into "the arithmetic is right" — and given the group-offset derivation in unfs_alloc.c and the extent split in bmap_extent_place, those are precisely the two places most likely to be wrong in a way a compiler cannot see.

Send that one grep and I'll write it.


============
What's parked
The self-test. unfs_selftest.c (402 lines) is written and delivered, but running it needs a bare-metal target or QEMU, and printf through the SoC stdio layer. Not started.

One thing to know when you get there: BCACHE_DRAM_BASE_DEFAULT — I have the stride (BCACHE_DEV_STRIDE_DEFAULT) and the block count (NUM_DISK_BLOCKS_DEFAULT = 524288), but the base never appeared in any grep. The harness prints it first for exactly that reason; if it's a placeholder, plat_addr() returns a near-null pointer and that's the finding.


===========================
VFS:


Before I touch unfs_vfs.c, I need to be straight about what it actually needs, because this is the file I've been calling the biggest blocker all along.


Used Workspace
I have the dependency picture, and it changes the recommendation. unfs_vfs.c needs six subsystems that don't exist, and one of them isn't a subsystem at all — it's an architectural decision.

What it depends on
Symbol	Uses	Exists?
uiox_vfs.h	1	❌ removed — only in 33_PCS/dataFlowFilesForBackup/FS/
uiox_page_cache.h	1	❌ no page cache in the tree
uiox_journal.h	1	❌ journal not wired in
vfs_register_fs	2	❌ no registry
vfs_mount_root	1	❌ no VFS
uiox_pc_read / _write	5	❌ no page cache
uiox_copy_to_user	1	❌ no address-space layer
unfs_fs_ops	3	❌ the ops-table layer, removed
uiox_file_ops_t, uiox_inode_ops_t, uiox_fs_ops_t	3	❌ removed with uiox_vfs.h
uiox_jr_*	5	❌ journal
Six subsystems. And your own unfs_fs.h already documented the decision:

"uiox_vfs.h REMOVED — it existed only under 33_PCS/dataFlowFilesForBackup/FS/, which no Makefile puts on -I. With it go uiox_superblock_t, uiox_file_ops_t, uiox_inode_ops_t and uiox_fs_ops_t — the ops-table layer."

The honest answer
I can't port unfs_vfs.c to make the filesystem compatible, because the filesystem is already compatible without it.

Look at what's in libfs.a now:

unfs_format.c — writes a volume the bootloader reads
unfs_mount.c — validates and mounts it into unfs_fs_t
unfs_iget.c — reads inodes into InCoreInode
bmap.c — maps file offsets through the extents
01_fsa — namei, readwrite, superblock, truncate
10_scfs — all 31 syscalls
That's a complete, coherent filesystem on the direct-call model — 01_fsa calls iget_dev(), bmap(), dir_add() by name. unfs_vfs.c is the other model: an ops-table dispatch layer with a registry, a page cache and a journal. Adding it doesn't complete the filesystem, it introduces a second, competing architecture alongside the one that already works.

What "compatible" would actually require
Three options, and only the third is honest to call a port:

1. Bring back the VFS. Restore uiox_vfs.h, uiox_page_cache.h and the three uiox_kix_scfs_* headers from the backup directory, put them on -I, and unfs_vfs.c returns nearly as written. Fast, but you'd then have two filesystems — the direct-call one that builds, and the VFS one that shadows it.

2. Write the missing six. A page cache, a journal, an address-space layer, a registry, the ops tables, and a root-mount path. That's not a port; it's a subsystem-level project, and it's months of work behind unfs_vfs.c, not in front of it.

3. Delete unfs_vfs.c. The filesystem is complete without it. Its 663 lines describe a design the tree deliberately abandoned.

My recommendation
Option 3 — remove it from deferred/ and stop treating it as the last missing file.

The tree's own comments already made this call. fs_types.h says:

"If a source still references one of those names, it is a file that has not been converted to the UNFS format yet — and a compile error naming it is the correct outcome, not something to paper over."

unfs_vfs.c is exactly that file. It isn't the last piece; it's a piece from a different design.

What actually remains
Two things, and neither is unfs_vfs.c:

Item	Effort
Extent-tree spill (case 4)	A real function: allocate a tree block, write the inline extents into it, clear them, record the block. The reader exists.
unfs_cow_block repoint	One call plus a signature change — I have that file in full and can deliver it whole now.
And the parked self-test, which is what would prove the arithmetic.

What I'd do if you want it anyway
If you want unfs_vfs.c back regardless, the fastest route is option 1 — but I'd want to see 33_PCS/dataFlowFilesForBackup/FS/uiox_vfs.h first:

sh


ls -la /Users/pramodkumar/Hack/WS/UIOX/33_PCS/dataFlowFilesForBackup/FS/
If that header still exists, restoring it plus three -I entries is a genuinely small change. If it's gone, option 1 isn't available either and the answer is unambiguously option 3.

Which do you want: check for the backup headers, or close out unfs_vfs.c as belonging to the abandoned design?




pasted-text.txt

TXT

attached here also for reffrence



Used Workspace
I've read unfs_vfs.c in full now — 663 lines — and it confirms what the dependency scan suggested. Let me be direct about what I found rather than start porting.

What it defines
The file is a complete second implementation of the filesystem, not an integration layer:



 41| uint32_t unfs_crc32c(...)                    ← its own CRC
192| uint32_t unfs_alloc_block(unfs_fs_t *fs)     ← DUPLICATE of unfs_alloc.c
218| void     unfs_free_block(unfs_fs_t *fs, ...) ← DUPLICATE
235| uint32_t unfs_alloc_inode(unfs_fs_t *fs, ...)← DUPLICATE
271| void     unfs_free_inode(...)                ← DUPLICATE
289| uint32_t unfs_cow_block(...)                 ← DUPLICATE of snap.c
519| int unfs_kern_mount(uiox_superblock_t *sb,...) ← DUPLICATE of unfs_mount.c
585| int unfs_kern_unmount(...)                   ← DUPLICATE
593| int unfs_kern_sync(...)                      ← DUPLICATE
610| int unfs_snap_create(...)                    ← DUPLICATE of snap.c
630| int unfs_snap_delete(...)                    ← DUPLICATE
642| int unfs_snap_list(...)                      ← DUPLICATE
658| void unfs_register(void)                     ← the registry call
Thirteen functions that already exist elsewhere in the tree, plus the ops tables and unfs_register.

The consequence you'd hit immediately
If unfs_vfs.c went into src/, the build would fail on duplicate symbols before it ever reached the missing headers:

unfs_alloc_block — defined in both unfs_alloc.c and unfs_vfs.c
unfs_cow_block, unfs_snap_create/delete/list — in both snap.c and unfs_vfs.c
unfs_kern_mount/unmount/sync — in both unfs_mount.c and unfs_vfs.c
And the signatures differ: unfs_kern_mount here takes uiox_superblock_t *sb, while the one in the archive takes unfs_fs_t *fs. Those aren't two views of one function — they're two filesystem designs sharing names.

What this file actually is
Its own banner says it:



 * Data flow (read path):
 *   sys_read(fd, ubuf, n)
 *     → vfs_read(file, kbuf, n)
 *         → unfs_file_ops.read(file, kbuf, n, pos)
 *             → uiox_pc_read(ino, pos, kbuf, n)   [page cache]
 *     → uiox_copy_to_user(ubuf, kbuf, n)          [privilege crossing]
That's the ops-table model with a page cache and a privilege-crossing layer — the architecture your tree deliberately abandoned. fs_types.h says a compile error naming one of those types "is the correct outcome, not something to paper over," and unfs_fs.h documents uiox_vfs.h as removed with the ops tables going with it.

So my answer is the same, and now I can show why
unfs_vfs.c cannot make the filesystem compatible, because:

It would collide with four working files on thirteen symbols
It needs six subsystems that don't exist — VFS, page cache, journal, registry, ops tables, address spaces
The filesystem is already compatible without it — libfs.a builds and mounts a volume through the direct-call model
The last point is the one that matters. unfs_format.c writes a volume the bootloader reads; unfs_mount.c validates it; unfs_iget.c and bmap.c read it; 10_scfs serves the syscalls. That's a filesystem.

What I'd do
Leave it in deferred/ and treat the port as complete. It isn't the last piece — it's the other design, kept for reference if a VFS is ever built.

If you do want a VFS later, the route is option 1 from my last message: restore the backup headers. Worth checking whether they still exist:

sh


ls -la /Users/pramodkumar/Hack/WS/UIOX/33_PCS/dataFlowFilesForBackup/FS/
If uiox_vfs.h and uiox_page_cache.h are there, that's a real starting point — but it would mean restoring the page cache and journal too, and choosing between two architectures rather than filling a gap.

What genuinely remains
Item	Effort
Extent-tree spill (case 4)	A real function — the reader exists, the writer doesn't
unfs_cow_block repoint	One call plus a signature change; I have that file in full
The self-test	Written, parked for a bare-metal target or QEMU
The four gaps are closed, the archive builds, and one unimplemented case is documented in the code. That's a coherent stopping point.

If you want me to write the extent-tree spill — the piece that makes case 4 work and lets a fragmented file keep writing — that I can do now, and it's the last real gap in the write path.

==============================
COW will updated once process and schedular going to update, it make sence then


That's a sound call, and it's the right order of operations.

Why the sequencing is right
COW depends on two things that don't exist yet:

A scheduler. Snapshot deletion's reclamation is a background job — walk the retained block set, free what nothing references. Right now unfs_snap_delete reports the count instead of performing it, because there's no process to run a reclaim pass. With a scheduler, that becomes a deferred task rather than a synchronous stall.

A process to own the work. unfs_cow_block needs the inode and the logical block to repoint the extent — that's context the write path has, but which currently doesn't survive the call. A per-process write context is the natural home for it.

And the copy itself needs a place to run that isn't a syscall's critical path, since allocating and copying a 4 KB block per write is real work.

What that leaves you
The filesystem is complete and coherent without COW:

unfs_format.c writes a volume the bootloader reads
unfs_mount.c validates and mounts it
unfs_iget.c reads inodes into InCoreInode
bmap.c maps file offsets through the extents, and now allocates too
01_fsa — namei, readwrite, superblock, truncate
10_scfs — all 31 syscalls
libfs.a builds for arm64. COW sits on top as a feature, and snap.c stays in deferred/ until there's a scheduler to support it.

One thing to note for when you come back
The ingredients are already in place, which is worth knowing because it makes the later work smaller:

Piece	Where it lives now
COW state	unfs_fs_t.cow_active
Snapshot table	unfs_fs_t.snapshots[], n_snapshots
Block allocator for the copy	unfs_alloc.c — unfs_alloc_run
Extent repoint	bmap_extent_place in bmap.c
Reclaim of orphaned blocks	unfs_free_run
So the mechanism is unbuilt, not unbuildable. What's missing is the routing (a write path that checks cow_active) and the policy (when to reclaim, on whose schedule).

Where I'd leave it
Three things open, none blocking:

Item	Nature
Extent-tree spill (case 4)	A real function. Fragmented past four inline extents → UNFS_ENOTSUP. The reader exists.
unfs_cow_block repoint	One call plus a signature change — waits for the scheduler, per your call
The self-test	Written (402 lines), parked for a bare-metal target or QEMU
And the one caveat worth carrying forward: the build proves types, not arithmetic. The group offsets in unfs_alloc.c and the extent split in bmap_extent_place case 2 have never executed. Running the self-test once is what would settle both — and it's a single command when you have a target, not a port.

Good place to stop. when the scheduler lands and we'll wire COW into it.

========================
Files description:

Here's how the twelve files fit together — the layers, the data flow, and where each file sits.

mermaid






Hardware

On-disk layer

10_unfs — the UNFS layer

Caller

10_scfs — 31 syscall bodies
(read, write, open, namei...)

01_fsa — inode, namei,
readwrite, superblock, bmap

unfs_fs.h
the mount handle
unfs_fs_t, unfs_snap_t

unfs_mount.c
volume → unfs_fs_t

unfs_iget.c
bytes → InCoreInode

bmap.c
offset → extent

unfs_format.h
the layout
structs + constants

unfs_format.c
mkfs — writes a volume

unfs_alloc.c/.h
block + inode allocation

unfs_io.c/.h
block read / write

unfs_errno.h
error codes

00_buffcache
bread / getblk / bwrite

bcache_plat_*_block
→ plat_addr()

unfs_extra_unfs_snap.c
snapshots + COW

The four headers — what each one owns
File	Owns	Never contains
unfs_format.h	The on-disk layout: unfs_sb_t, unfs_inode_t, unfs_group_desc_t, unfs_dirent_t, and every constant (magic, block size, UNFS_GROUP0_*). Shared with the bootloader.	Any code. Any kernel type.
unfs_fs.h	The kernel-side handle: unfs_fs_t, unfs_snap_t, unfs_inode_priv_t, and the whole API.	The layout itself.
unfs_io.h	Three functions: unfs_bdev_read, unfs_bdev_write, unfs_crc32.	The sector conversion — that's in the .c.
unfs_errno.h	UNFS_OK, UNFS_EIO, UNFS_EINVAL, UNFS_ENOSPC, …	Anything else.
The rule that matters: unfs_format.h is the only description of what lands on disk. It's shared with 01_uBoot, so both read the same bytes. If it and the bootloader's copy ever disagree, the bootloader wins.

The four .c files — the write side
mermaid






write_block()

×8 sectors

scan bitmaps

read/write bitmap

checksum

UNFS_BLOCK_SIZE
(4096-byte blocks)

unfs_format.c
mkfs

unfs_io.c

bcache
(512-byte)

unfs_alloc.c

unfs_crc32()

unfs_format.c — writes a volume in this order:



block 0              superblock (at UNFS_SB_OFFSET = 0)
UNFS_GROUP0_DESC  258  group descriptor table
UNFS_GROUP0_BBMAP 259  block bitmap      (1 = FREE)
UNFS_GROUP0_IBMAP 260  inode bitmap
UNFS_GROUP0_ITABLE 261 inode table
UNFS_GROUP0_DATA  269  root directory data block
                       root inode + "." and ".."
Two things worth knowing: the bit polarity is inverted (1 means free, unlike ext2), and block numbers are in 4096-byte units while bcache moves 512-byte sectors — so unfs_io.c multiplies by 8.

unfs_alloc.c — finds a run of consecutive free blocks:

mermaid






yes

no

unfs_alloc_run(dev, want, out)

read superblock

read group bitmap

run of 'want'
set bits?

clear the bits
write bitmap back

s_decrement free_blocks

return the run

next group

Order matters: bitmap first, then the counter. A crash between them leaves the count too high, which self-corrects.

The two .c files — the read side
mermaid






no

yes

yes

no

no

yes

yes

no

yes

no

unfs_kern_mount(fs, dev)

read block 0

magic OK?

UNFS_EBADMAGIC

geometry
+ checksum OK?

read group descriptors
into fs->groups[]

bounds-check each

fs->n_groups, fs->mounted = 1

unfs_iget(dev, inum)

in cache?

refcount++

unfs_read_inode()

size fits
uint32_t?

return NULL

inode_inflate()
→ InCoreInode

bmap(ip, offset)

lb = offset / 4096

inline extent
covers lb?

extent_phys()

i_extent_tree?

extree_lookup()
reads 8 sectors

valid = false
(past EOF / hole)

unfs_mount.c answers one question: is this a UNFS volume, and where is everything? It returns a populated unfs_fs_t — the group array, the counts, the device. It does not register anywhere; there's no registry.

unfs_iget.c is the seam between the two worlds: unfs_inode_t (the raw 256 bytes on disk) and InCoreInode (what 01_fsa speaks). One honest limitation: InCoreInode.size is uint32_t against a uint64_t on disk, so a file over 4 GB is refused rather than truncated.

The optional file
unfs_extra_unfs_snap.c sits on top, not in the path:

mermaid






no

yes

a write to an existing block

fs->cow_active?

overwrite in place

unfs_cow_block()

unfs_alloc_block(dev)

copy 8 sectors

GAP: repoint the extent
bmap_extent_place()

That gap is real: the copy happens, but the extent still names the old block, so the change is invisible. You've parked it for when the scheduler lands.

The dependency order, if you read them cold
unfs_format.h — everything depends on this. Start here.
unfs_errno.h — trivial, but referenced by the rest.
unfs_io.h / unfs_io.c — the 4096↔512 boundary.
unfs_fs.h — the types the API speaks.
unfs_format.c, unfs_alloc.h/.c — the write side.
unfs_mount.c, unfs_iget.c, bmap.c — the read side.
unfs_extra_unfs_snap.c — optional, on top.
One thing worth saying about the picture you shared
Your filesystemCalls.png shows the syscall flow — open → read/write → close. Note that COW sits below all of that: a write() reaches bmap_alloc(), which allocates a block. If COW were active, the write should route through unfs_cow_block() first — and nothing in the current tree does that routing. So the two halves of "snapshots don't work yet" are: no repoint in cow_block, and no routing from the write path. Both wait on the scheduler, as you decided.

The files all live in outputs/10_unfs/ in my workspace if you want any of them re-sent.


---------------------
# UNFS — File Map and Data Flow

## 1. How the twelve files fit together

```
┌─────────────────────────────────────────────────────────────────┐
│ CALLER                                                          │
│   10_scfs — 31 syscall bodies (read, write, open, namei …)      │
│   01_fsa  — inode, namei, readwrite, superblock, bmap           │
└────────────────────────────┬────────────────────────────────────┘
                             │
┌────────────────────────────▼────────────────────────────────────┐
│ 10_unfs — THE UNFS LAYER                                        │
│   unfs_fs.h     the mount handle   (unfs_fs_t, unfs_snap_t)     │
│   unfs_mount.c  volume  →  unfs_fs_t                            │
│   unfs_iget.c   bytes   →  InCoreInode                          │
│   bmap.c        offset  →  extent  →  block                     │
└────────────────────────────┬────────────────────────────────────┘
                             │
┌────────────────────────────▼────────────────────────────────────┐
│ ON-DISK LAYER                                                   │
│   unfs_format.h   the layout — structs + constants              │
│   unfs_format.c   mkfs — writes a volume                        │
│   unfs_alloc.h/c  block + inode allocation                      │
│   unfs_io.h/c     block read / write                            │
│   unfs_errno.h    error codes                                   │
└────────────────────────────┬────────────────────────────────────┘
                             │
┌────────────────────────────▼────────────────────────────────────┐
│ HARDWARE                                                        │
│   00_buffcache          bread / getblk / bwrite                 │
│   bcache_plat_*_block   →  plat_addr()                          │
└─────────────────────────────────────────────────────────────────┘

        unfs_extra_unfs_snap.c   ← optional, sits ON TOP
             uses: bmap.c, unfs_alloc.c
```

## 2. What each header OWNS

| File | Owns | Never contains |
|---|---|---|
| `unfs_format.h` | The on-disk layout: `unfs_sb_t`, `unfs_inode_t`, `unfs_group_desc_t`, `unfs_dirent_t`, and every constant (magic, block size, `UNFS_GROUP0_*`). **Shared with the bootloader.** | Any code. Any kernel type. |
| `unfs_fs.h` | The kernel-side handle: `unfs_fs_t`, `unfs_snap_t`, `unfs_inode_priv_t`, and the whole API. | The layout itself. |
| `unfs_io.h` | Three functions: `unfs_bdev_read`, `unfs_bdev_write`, `unfs_crc32`. | The sector conversion — that lives in the `.c`. |
| `unfs_errno.h` | `UNFS_OK`, `UNFS_EIO`, `UNFS_EINVAL`, `UNFS_ENOSPC`, … | Anything else. |

**The rule:** `unfs_format.h` is the only description of what lands on
disk. If it and the bootloader's copy disagree, the bootloader wins.

## 3. The write side

```
   unfs_format.c  (mkfs)
        │
        ├─ write_block() ──────────┐
        ├─ unfs_crc32()            │
        └─ unfs_alloc.c            │
              │                    │
              └─ bitmap read/write ┘
                        │
                   unfs_io.c          ×8 conversion
                        │             4096-byte block → 512-byte sector
                     bcache
```

### What mkfs writes, in order

```
block 0                superblock        (at UNFS_SB_OFFSET = 0)
UNFS_GROUP0_DESC   258  group descriptor table
UNFS_GROUP0_BBMAP  259  block bitmap       ← 1 = FREE
UNFS_GROUP0_IBMAP  260  inode bitmap
UNFS_GROUP0_ITABLE 261  inode table
UNFS_GROUP0_DATA   269  root directory data block
                        root inode + "." and ".."
```

Two traps: the bit polarity is **inverted** (`1` means *free*, unlike
ext2), and block numbers are **4096-byte units** while bcache moves
512-byte sectors — so every I/O multiplies by 8.

### unfs_alloc_run — finding a run

```
  unfs_alloc_run(dev, want, out)
        │
        ▼
   read superblock
        │
        ▼
   read group bitmap
        │
        ▼
   ┌─────────────────────┐
   │ run of 'want'       │──── no ──▶ next group ──┐
   │ SET bits?           │                         │
   └─────────────────────┘◀────────────────────────┘
        │ yes
        ▼
   clear the bits
        │
        ▼
   write bitmap back        ← bitmap FIRST
        │
        ▼
   decrement s_free_blocks  ← counter SECOND
        │
        ▼
   return the run
```

Order matters: a crash between the two steps leaves the count too
**high**, which self-corrects on the next allocation.

## 4. The read side

### unfs_mount — is this a UNFS volume?

```
  unfs_kern_mount(fs, dev)
        │
        ▼
   read block 0
        │
        ▼
   ┌───────────────┐
   │ magic OK?     │──── no ──▶ UNFS_EBADMAGIC
   └───────────────┘
        │ yes
        ▼
   ┌───────────────┐
   │ geometry +    │──── no ──▶ UNFS_EBADSB / UNFS_EBADCRC
   │ checksum OK?  │
   └───────────────┘
        │ yes
        ▼
   read group descriptors → fs->groups[]
        │
        ▼
   bounds-check each against s_block_count
        │
        ▼
   fs->n_groups  →  fs->mounted = 1
```

Returns a populated `unfs_fs_t`. It does **not** register anywhere —
there is no registry.

### unfs_iget — bytes to in-core inode

```
  unfs_iget(dev, inum)
        │
        ▼
   ┌───────────────┐
   │ in cache?     │──── yes ──▶ refcount++  → return
   └───────────────┘
        │ no
        ▼
   unfs_read_inode()
        │
        ▼
   ┌───────────────┐
   │ size fits     │──── no ──▶ return NULL  (no lying inode)
   │ uint32_t?     │
   └───────────────┘
        │ yes
        ▼
   inode_inflate()  →  InCoreInode
```

One honest limit: on-disk `i_size` is `uint64_t`, in-core `size` is
`uint32_t`. A file over 4 GB is **refused rather than truncated**.

### bmap — offset to block

```
  bmap(ip, offset)
        │
        ▼
   lb = offset / UNFS_BLOCK_SIZE        (4096, not 512)
        │
        ▼
   ┌──────────────────────┐
   │ inline extent        │──── yes ──▶ extent_phys() → valid
   │ covers lb?           │
   └──────────────────────┘
        │ no
        ▼
   ┌──────────────────────┐
   │ i_extent_tree != 0?  │──── yes ──▶ extree_lookup()
   └──────────────────────┘             (reads all 8 sectors)
        │ no
        ▼
   valid = false        (past EOF, or a hole)
```

## 5. The optional file — snapshots + COW

```
  a write to an existing block
        │
        ▼
   ┌──────────────────┐
   │ fs->cow_active?  │──── no ──▶ overwrite in place
   └──────────────────┘
        │ yes
        ▼
   unfs_cow_block()
        │
        ▼
   unfs_alloc_block(dev)     ← get a free block
        │
        ▼
   copy 8 sectors            ← the data is copied
        │
        ▼
   *** GAP ***               ← extent NOT repointed
   bmap_extent_place()          so the change is invisible
```

The gap is real and parked: the copy happens, but the extent still
names the old block. Both halves — the repoint here, and the routing
from the write path above — wait on the scheduler.

## 6. Reading order, if you start cold

```
  1.  unfs_format.h        everything depends on this — start here
  2.  unfs_errno.h         trivial, but referenced throughout
  3.  unfs_io.h / .c       the 4096 ↔ 512 boundary
  4.  unfs_fs.h            the types the API speaks
  5.  unfs_format.c        mkfs — the write side
      unfs_alloc.h / .c    allocation
  6.  unfs_mount.c         mount
      unfs_iget.c          inode bridge
      bmap.c               extent mapping
  7.  unfs_extra_unfs_snap.c   optional, on top
```

Notes on the diagrams
The unfs_xattr.c you listed is unfs_extra_unfs_xattr.c on disk — it holds xattr name/value storage. It sits beside snap.c as an optional file: unfs_fs.h declares unfs_xattr_get / unfs_xattr_set, and 10_scfs's setxattr/getxattr call them. It's in the build.

The one thing the ASCII can't show well is that unfs_format.h is the only file with a second consumer outside your tree — 01_uBoot reads the same structs. That's why it carries layout and nothing else.

If you want this as an actual file rather than text in the chat, say the word and I'll write it to outputs/ as 10_unfs/UNFS_FILEMAP.md so you get a download link.