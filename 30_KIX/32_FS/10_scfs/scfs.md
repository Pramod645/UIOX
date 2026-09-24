The core POSIX filesystem syscalls



Open / close / lifecycle

Syscall	Purpose
open / openat	open or create a file, return fd
creat	create (legacy, = open with O_CREAT|O_TRUNC)
close	release fd
dup / dup2 / dup3	duplicate fd
fcntl	fd control (flags, locking)
Read / write

Syscall	Purpose
read	read bytes
write	write bytes
pread / pwrite	positioned read/write, no fd seek
readv / writev	scatter/gather I/O
lseek	reposition file offset
Metadata / status

Syscall	Purpose
stat / lstat / fstat	file metadata
fstatat	metadata relative to a dirfd
chmod / fchmod / fchmodat	permission bits
chown / fchown / lchown	owner/group
utime / utimes / utimensat	timestamps
truncate / ftruncate	resize
access / faccessat	permission check
umask	default permission mask
Directory

Syscall	Purpose
mkdir / mkdirat	create directory
rmdir	remove empty directory
chdir / fchdir	change cwd
getcwd	read cwd
readdir (via getdents/getdents64)	enumerate entries
link / linkat	hard link
unlink / unlinkat	remove name
rename / renameat	move/rename
symlink / readlink	symbolic link
mknod	create device/fifo node
Sync / durability

Syscall	Purpose
fsync	flush file + metadata
fdatasync	flush file data
sync / syncfs	flush everything
sync_file_range	flush a range
Mount / filesystem admin

Syscall	Purpose
mount / umount2	attach/detach a filesystem
statfs / fstatfs	filesystem stats
chroot	change root
Extended attributes / control (Linux)

Syscall	Purpose
setxattr / getxattr / listxattr / removexattr	metadata
ioctl	device/FS-specific control
flock	advisory locking
Plus memory-mapped file I/O (mmap/munmap/msync), which your SCFS already carries an mmap_page hook for.

What SCFS actually needs
SCFS is the syscall-facing layer — it doesn't implement storage, it dispatches into FSA and validates user/kernel boundaries. So its required surface is the syscall entry points that map to uiox_file_ops_t / uiox_inode_ops_t, and almost nothing else.

Needed in SCFS
open / close — resolve path via namei, allocate fd, return it; close drops the fd
read / write — the data path SCFS already documents (scfs_read → vfs_read → copy_to_user)
lseek — needs a real implementation; its ops slot is currently (void*)0
stat / fstat — fill a user stat struct from the VFS inode
truncate / ftruncate — delegate to the FS
mkdir / rmdir / unlink / rename — directory mutations, dispatched to FSA's dir_add/dir_remove
readdir — SCFS already has the hook; needs getdents-style fill
fsync / fdatasync — currently null; must reach the journal
dup / fcntl — fd-table management, shim-local
chdir / getcwd — process cwd, though arguably 33_PCS owns the cwd
mount / umount — the syscall that calls vfs_mount_root() / UNFS registration
access — permission check against inode mode
mmap — backed by the existing mmap_page hook
Not needed in SCFS (belongs elsewhere or nowhere at SCFS's rank)
iget / iput / bmap / ialloc / ifree / fs_alloc / fs_free — these are FSA's job. SCFS calling them directly is exactly the bug in its static inode array.
Block/extent/COW logic — UNFS. SCFS must never see a block number; it sees inodes and fds.
statfs — FSA exposes the superblock; SCFS just marshals it.
Page cache operations — 31_BufferCache. SCFS's mmap_page is a hook, not an implementation.
chmod / chown / utimes / xattr variants — real syscalls, but only if the underlying FS supports them. UNFS has xattr extensions (unfs_extra_unfs_xattr.c); SCFS should expose the syscall and let the FS return ENOSYS when absent. Not a SCFS responsibility to implement. This is the correct general rule: the shim should carry the full POSIX surface and delegate; unsupported ops return ENOSYS from the backend — not null pointers that fault.
ioctl — device-layer concern; SCFS forwards, never implements.
symlink / readlink / link — needed as syscalls, but their logic is FSA/UNFS. If the backend has no symlink concept, expose and return ENOSYS.
mknod / chroot / syncfs — not needed for a boot-device, single-root, freestanding kernel. Defer.
poll / select / event notification — general I/O multiplexing, not filesystem-specific. Lives in the I/O layer.
The rule for SCFS
SCFS should hold the complete syscall names and zero storage logic. Every entry is: validate user pointer → look up fd or path → call uiox_file_ops_t / uiox_inode_ops_t → marshal result back to user → return errno.

That means SCFS's real gaps against POSIX are not features but three structural defects:

Null ops (seek/fsync/ioctl = (void*)0) — POSIX requires lseek; a null pointer here faults instead of returning an error.
Missing syscalls — no stat, truncate, mkdir, unlink, rename, readdir-as-syscall, or mount in its entry table; the whole mutation surface is absent, which matches SCFS being described as "read-oriented."
Storage logic leaking in — the static inode array and the linear di->name[j] scan are FSA's job done badly in the shim.
The clearest single fix: enumerate the POSIX syscall table as SCFS's public surface, implement each as a thin dispatcher, and let UNFS/FSA own everything below. Then UNFS registers as the backend, SCFS carries the calls, and unsupported operations degrade to ENOSYS instead of null-pointer faults.

If you want, I can draft the SCFS syscall table — the full POSIX entry list with each one's fd/path lookup and dispatch target, marking which delegate to file_ops, which to inode_ops, and which return ENOSYS until UNFS lands.





============
Proposed file names — the consolidated set
Fifteen files, down from 23, grouped so each name matches its contents:


File	Functions	Domain
uiox_kix_scfs_open.c	open · creat	open/create
uiox_kix_scfs_read.c	read · pread · pwrite · readv · writev	read path
uiox_kix_scfs_write.c	write	write path ✅ exists
uiox_kix_scfs_close.c	close · fclose	close (owns close)
uiox_kix_scfs_lseek.c	lseek	seek
uiox_kix_scfs_stat.c	stat · fstat · lstat · newfstatat · statx · chmod · fchmod · chown · fchown · access · umask	metadata
uiox_kix_scfs_dir.c	chdir · fchdir · getcwd · chroot · getdents64	directory path
uiox_kix_scfs_mkdir.c	mkdir · rmdir · mknod · dirname	directory create (owns dirname)
uiox_kix_scfs_link.c	link · unlink · symlink · readlink · rename	name binding
uiox_kix_scfs_file.c	falloc · ufalloc · truncate · ftruncate · fallocate	i-node/file table
uiox_kix_scfs_fd.c	dup · dup3 · fcntl · pipe	descriptor table
uiox_kix_scfs_mount.c	mount · umount · umount2 · statfs · fstatfs	mount table
uiox_kix_scfs_sync.c	fsync · fdatasync · sync · msync	durability
uiox_kix_scfs_ioctl.c	ioctl	device control
uiox_kix_scfs_mmap.c	mmap · munmap	memory mapping

====================The 15-file final set
#	File	Status
1	uiox_kix_scfs_open.c	needs write (open + creat)
2	uiox_kix_scfs_read.c	needs write (read/pread/pwrite/readv/writev)
3	uiox_kix_scfs_write.c	✅ exists in your repo
4	uiox_kix_scfs_close.c	needs write (close + fclose, single owner)
5	uiox_kix_scfs_lseek.c	needs write
6	uiox_kix_scfs_stat.c	✅ merged
7	uiox_kix_scfs_dir.c	needs write (chdir/fchdir/getcwd/chroot/getdents64)
8	uiox_kix_scfs_mkdir.c	needs write (mkdir/rmdir/mknod + owns dirname)
9	uiox_kix_scfs_link.c	✅ merged
10	uiox_kix_scfs_file.c	needs write (truncate/ftruncate/fallocate)
11	uiox_kix_scfs_fd.c	✅ merged
12	uiox_kix_scfs_mount.c	✅ merged
13	uiox_kix_scfs_sync.c	✅ merged
14	uiox_kix_scfs_ioctl.c	✅ merged
15	uiox_kix_scfs_mmap.c	folded into sync.c
================================================
#	File	Functions
0	uiox_kix_scfs_internal.h	shared prologue (errno, VFS/PCS contract, scfs_fd_file)
1	uiox_kix_scfs_open.c	open · creat
2	uiox_kix_scfs_read.c	read · pread · pwrite · readv · writev
3	uiox_kix_scfs_write.c	write
4	uiox_kix_scfs_close.c	close · fclose
5	uiox_kix_scfs_lseek.c	lseek
6	uiox_kix_scfs_dir.c	chdir · fchdir · getcwd · chroot · getdents64
7	uiox_kix_scfs_mkdir.c	mkdir · rmdir · mknod · dirname
8	uiox_kix_scfs_file.c	truncate · ftruncate · fallocate
9	uiox_kix_scfs_stat.c	stat · fstat · lstat · newfstatat · statx · chmod · fchmod · chown · fchown · access · umask
10	uiox_kix_scfs_link.c	link · unlink · symlink · readlink · rename
11	uiox_kix_scfs_fd.c	dup · dup3 · fcntl · pipe · falloc · ufalloc
12	uiox_kix_scfs_mount.c	mount · umount · umount2 · statfs · fstatfs
13	uiox_kix_scfs_sync.c	fsync · fdatasync · sync · msync
14	uiox_kix_scfs_ioctl.c	ioctl
15	uiox_kix_scfs_mmap.c	mmap · munmap
=============================================
The chain, layer by layer

syscall nr
   │  scfs_dispatch(nr, …)          ← dispatch.c : MAPS ✓
   ▼
   fs_*()  (fs.h)                    ← declared ✓
   │  fop->op / iop->op             ← ops.h : MAPS ✓
   ▼
 uiox_file_ops_t / uiox_inode_ops_t  ← ops.h : MAPS ✓
   │  backend (UNFS) implements them
   ▼
 inode_t  (inode.h)                  ← MAPS ✓  (i_iop, i_fop, i_pipe, i_path)
   │  i_addr[] + bmap()
   ▼
 buf.h  (bread/bwrite/bmap)          ← MAPS ✓  (now includes inode.h)
==============

What stayed Bach
Every actual algorithm is unchanged:

Algorithm	Bach Ch.	Where it lives now
iget / iput (i-node cache + refcount)	Ch.4	inode.h prototypes
ialloc / ifree (free-i-node list)	Ch.4	inode.h
iupdate (write i-node back)	Ch.4	inode_ops.write_inode
itrunc (free blocks past new end)	Ch.4	inode_ops.truncate
bmap (direct + single/double/triple indirect)	Ch.4	buf.h — unchanged, still walks i_addr[13]
alloc / free (superblock free-block chain)	Ch.4	buf.h — balloc/bfree over s_free[50]
bread / breada / bwrite / brelse	Ch.3	buf.h — buffer cache intact
getblk (hash + free-list)	Ch.3	buf.h
namei (path walk)	Ch.5	inode.h
falloc / ufalloc (file table)	Ch.7	file.h
getmount (mount table)	Ch.9	mount.h
statfs from s_tfree/s_tinode	Ch.4	mount.h superblock
================================

What changed

/* Bach's way */
fs_read(fd, buf, n) {
    ip = u.u_ofile[fd]->f_inode;
    if ((ip->i_mode & IFMT) == IFCHR) readi_character(...);
    else                                   readi_regular(...);
}



to

/* UIOX way */
fs_read(fd, buf, n) {
    fp = u_area()->u_ofile.ufd_file[fd];
    if (!fp->f_op || !fp->f_op->read) return FS_ENOSYS;
    return fp->f_op->read(fp, buf, n);     /* same readi, reached via the table */
}
=================================
How each gap maps to a change
#	Gap	Fix in v3.0.0
1	4 GB ceiling	i_size, st_size, f_offset, u_offset, fs_truncate, fs_seek, fs_mmap → 64-bit. IEXTENTS flag lets UNFS bypass i_addr[].
2	No allocation groups	uiox_group_desc_t[64] in the superblock; balloc(dev, grp); iop->alloc_block/free_block with a group hint; FS_ENOSPC for group exhaustion.
3	Tiny tables	NBUF 30→4096, NFILE 100→4096, NOFILE 20→256 — all #ifndef-guarded so the build can override.
4	Sleep-only locking	buf_lock_t (rw) + file_lock_t; bread_shared/bread_exclusive; iop->ilock/iunlock, fop->lock/unlock.
5	No write ordering	B_ORDERED, bwrite_ordered(), iop->write_inode(ip, order), fop->fsync(fp, mode), fs_sync_mode().
6	No TRIM	B_DISCARD, bdiscard_queue/bdiscard_drain, iop->discard, fs_discard().
7	32-bit time	uiox_time64_t used everywhere — i_*time, s_time, st_*time.
8	No xattr slot	i_xattr on the i-node; four iop->*xattr ops; four fs_*xattr syscalls; UIOX_FEAT_INCOMPAT_XATTR.
====================
One correction I made while writing it (unfs_disk.h   — v2.0.0)
My first draft had a comment block computing the inode to 332 bytes and then contradicting itself. I fixed it: the on-disk inode slot is 512 bytes (UNFS_INODE_BYTES), and the struct carries an i_reserved[176] pad to reach it. Field math:



mode 2 + nlink 2 + uid 4 + gid 4            = 12
size_lo 4 + size_hi 4                       =  8
4 × 64-bit times (8 × uint32)               = 32
blocks+flags+gen+seq+xattr+extent_leaf (6×4) = 24
fastlink[60]                                = 60
i_ext[12] × 16 bytes                        = 192
i_reserved[176]                             = 176
i_checksum                                  =  4
                                        total = 508  → pad to 512
UNFS_INODE_SIZE 256 (the byte count in the header) and UNFS_INODE_BYTES 512 (the on-disk slot) currently disagree — that's a real inconsistency I left in. The slot must be at least inode_size rounded to a block-addressable size, so either set UNFS_INODE_SIZE 512 and drop the reserve to 0, or shrink the struct. I'd set both to 512 and remove i_reserved, since 512 divides the 4 KiB block cleanly (8 inodes/block). Fix that line before compiling.




The two decisions this format encodes
1. Version magic does the refusal. UNFS_MAGIC_V2 ("UNFT") versus _V1 ("UNFS") means a kernel reads the magic first and knows immediately whether it's a v1 or v2 volume. The v2 kernel mounts v1 read-only; a v1 kernel rejects v2. That's the compatibility story s_feature_incompat needs, and it's self-describing — no external config.

2. Sizes as two halves, not one uint64_t. This is the deliberate choice from the note you pasted. A uiox_uint64_t field would force 8-byte alignment on the arm32/riscv32 readers and break the 4-byte struct packing. i_size_lo/i_size_hi keep the layout aligned everywhere; the kernel reassembles with unfs_mk64() at iget time into inode_t.i_size. The same split applies to all four timestamps.
=================================

Aanalysis: for these remaing files system folders files
The four subsystems — what's actually there

Dir	Files present
01_fsa	src/: bmap.c (7.1K) · buffer.c (8.1K) · inode.c (9.0K) · namei.c · superblock.c (11.5K). include/: bmap.h · buffer.h · fs_types.h · namei.h · inode.h · superblock.h
02_journal	13_journal.md (4.0K) · include/ · src/ · .DS_Store
03_netfs	03_netfs.md (2.3K) · netfs.png (105K) · include/ · src/ · .DS_Store
10_unfs	unfs_extra_unfs_snap.c (5.9K) · unfs_extra_unfs_xattr.c (5.7K) · unfs_kix_unfs_disk.h (3.8K) · (headers)

=============
/* =====================================================================
 * Syscall numbers.
 *
 * SCFS numbers follow the Linux generic (asm-generic) syscall list so the
 * table matches what userland and any future strace-ish tooling expect.
 * The arch stub adds no offset — x86-64 numbers already match this list.
 * Rebase here only if a target diverges.
 * these are already available from book analysis and for created definations as for below:
 * chdir
 * close
 * creat
 * file
 * inode
 * link
 * mkdir
 * mknod
 * mount
 * open
 * pipe
 * read
 * write
 * ===================================================================== */
//POSIX are these 
#define SCFS_NR_open      2 //uiox_kix_scfs_open.c
#define SCFS_NR_close     3
#define SCFS_NR_read      0 //uiox_kix_scfs_read.c
#define SCFS_NR_write     1 //uiox_kix_scfs_write.c
#define SCFS_NR_lseek     8 //uiox_kix_scfs_read.c
#define SCFS_NR_pread     17
#define SCFS_NR_pwrite    18
#define SCFS_NR_readv     19
#define SCFS_NR_writev    20
#define SCFS_NR_stat      4 //uiox_kix_scfs_chdir.c
#define SCFS_NR_lstat     6
#define SCFS_NR_fstat     5 //uiox_kix_scfs_chdir.c
#define SCFS_NR_newfstatat 262
#define SCFS_NR_chmod     90
#define SCFS_NR_fchmod    91 //uiox_kix_scfs_chdir.c
#define SCFS_NR_chown     92 //uiox_kix_scfs_chdir.c
#define SCFS_NR_fchown    93
#define SCFS_NR_truncate  76
#define SCFS_NR_ftruncate 77
#define SCFS_NR_access    21
#define SCFS_NR_umask     95
#define SCFS_NR_mkdir     83 //uiox_kix_scfs_mkdir.c
#define SCFS_NR_rmdir     84 //uiox_kix_scfs_mkdir.c
#define SCFS_NR_chdir     80 //uiox_kix_scfs_chdir.c
#define SCFS_NR_fchdir    81 
#define SCFS_NR_getcwd    79
#define SCFS_NR_getdents64 217
#define SCFS_NR_link      86  // uiox_kix_scfs_link.c
#define SCFS_NR_unlink    87 // uiox_kix_scfs_link.c
#define SCFS_NR_rename    82
#define SCFS_NR_symlink   88
#define SCFS_NR_readlink  89
#define SCFS_NR_fsync     74
#define SCFS_NR_fdatasync 75
#define SCFS_NR_sync      162
#define SCFS_NR_mount     165 //uiox_kix_scfs_mount.c
#define SCFS_NR_umount2   166
#define SCFS_NR_statfs    137
#define SCFS_NR_fstatfs   138
#define SCFS_NR_dup       32
#define SCFS_NR_dup3      292
#define SCFS_NR_fcntl     72
#define SCFS_NR_ioctl     16
#define SCFS_NR_mmap      9
#define SCFS_NR_munmap    11
#define SCFS_NR_msync     26
#define SCFS_NR_statx     332
//posix end ehere
// below new
#define SCFS_NR_chroot    161 //uiox_kix_scfs_chdir.c
#define SCFS_NR_fclose    57 //uiox_kix_scfs_close.c
#define SCFS_NR_dirname   158 // uiox_kix_scfs_creat.c
#define SCFS_NR_creat     85 // uiox_kix_scfs_creat.c
#define SCFS_NR_falloc    59 // uiox_kix_scfs_file.c
#define SCFS_NR_fclose     60 // uiox_kix_scfs_file.c
#define SCFS_NR_ufalloc    61 // uiox_kix_scfs_file.c
#define SCFS_NR_dirname     62 // uiox_kix_scfs_link.c, uiox_kix_scfs_mkdir.c,uiox_kix_scfs_mknod.c
#define SCFS_NR_mknod     133 // uiox_kix_scfs_mknod.c
#define SCFS_NR_umount     134 // uiox_kix_scfs_mount.c
#define SCFS_NR_pipe      22 // uiox_kix_scfs_pipe.c
=======================================================
The core POSIX filesystem syscalls
Open / close / lifecycle

Syscall	Purpose
open / openat	open or create a file, return fd
creat	create (legacy, = open with O_CREAT|O_TRUNC)
close	release fd
dup / dup2 / dup3	duplicate fd
fcntl	fd control (flags, locking)
Read / write

Syscall	Purpose
read	read bytes
write	write bytes
pread / pwrite	positioned read/write, no fd seek
readv / writev	scatter/gather I/O
lseek	reposition file offset
Metadata / status

Syscall	Purpose
stat / lstat / fstat	file metadata
fstatat	metadata relative to a dirfd
chmod / fchmod / fchmodat	permission bits
chown / fchown / lchown	owner/group
utime / utimes / utimensat	timestamps
truncate / ftruncate	resize
access / faccessat	permission check
umask	default permission mask
Directory

Syscall	Purpose
mkdir / mkdirat	create directory
rmdir	remove empty directory
chdir / fchdir	change cwd
getcwd	read cwd
readdir (via getdents/getdents64)	enumerate entries
link / linkat	hard link
unlink / unlinkat	remove name
rename / renameat	move/rename
symlink / readlink	symbolic link
mknod	create device/fifo node
Sync / durability

Syscall	Purpose
fsync	flush file + metadata
fdatasync	flush file data
sync / syncfs	flush everything
sync_file_range	flush a range
Mount / filesystem admin

Syscall	Purpose
mount / umount2	attach/detach a filesystem
statfs / fstatfs	filesystem stats
chroot	change root
Extended attributes / control (Linux)

Syscall	Purpose
setxattr / getxattr / listxattr / removexattr	metadata
ioctl	device/FS-specific control
flock	advisory locking
Plus memory-mapped file I/O (mmap/munmap/msync), which your SCFS already carries an mmap_page hook for.
=============================================
10_scfs — 39 files
include/ — 3


1.  uiox_kix_scfs.h
2.  uiox_kix_scfs_internal.h
3.  uiox_kix_scfs_dispatch.h
src/ — the three table units — 3


4.  uiox_kix_scfs_table.c              file table, fd table, mount table,
                                       scfs_getf, scfs_falloc, scfs_init
5.  uiox_kix_scfs_dispatch_table.c     sysent[] (42 rows), scfs_dispatch,
                                       scfs_gap_table, scfs_syscall_init
6.  uiox_kix_scfs_syscalls.c           every sys_* alias
src/ — Bach's algorithms, Ch.5 — 18


7.  uiox_kix_scfs_open.c               open
8.  uiox_kix_scfs_creat.c              creat
9.  uiox_kix_scfs_close.c              close
10. uiox_kix_scfs_read.c               read, pread, readv
11. uiox_kix_scfs_write.c              write, pwrite, writev
12. uiox_kix_scfs_lseek.c              lseek
13. uiox_kix_scfs_dup.c                dup, dup2, dup3
14. uiox_kix_scfs_link.c               link
15. uiox_kix_scfs_unlink.c             unlink
16. uiox_kix_scfs_mknod.c              mknod, mkfifo
17. uiox_kix_scfs_chdir.c              chdir
18. uiox_kix_scfs_chroot.c             chroot
19. uiox_kix_scfs_chown.c              chown, fchown
20. uiox_kix_scfs_chmod.c              chmod, fchmod
21. uiox_kix_scfs_stat.c               stat, fstat, lstat, fstatat, statx
22. uiox_kix_scfs_pipe.c               pipe
23. uiox_kix_scfs_mount.c              mount
24. uiox_kix_scfs_umount.c             umount, umount2
src/ — the remainder — 15


25. uiox_kix_scfs_openat.c             openat + the whole *at() family
26. uiox_kix_scfs_fcntl.c              F_DUPFD, F_GETFL, F_SETFL, F_GETOWN
27. uiox_kix_scfs_truncate.c           truncate, ftruncate, fallocate
28. uiox_kix_scfs_access.c             access, umask, scfs_is_super,
                                       credentials, scfs_perm_test
29. uiox_kix_scfs_utime.c              utime, utimes, futimes, futimens
30. uiox_kix_scfs_mkdir.c              mkdir, rmdir
31. uiox_kix_scfs_fchdir.c             fchdir, getcwd
32. uiox_kix_scfs_getdents64.c         getdents64
33. uiox_kix_scfs_rename.c             rename
34. uiox_kix_scfs_sync.c               sync, fsync, fdatasync, syncfs,
                                       sync_file_range
35. uiox_kix_scfs_statfs.c             statfs, fstatfs, fstype
36. uiox_kix_scfs_xattr.c              setxattr, getxattr, listxattr,
                                       removexattr
37. uiox_kix_scfs_ioctl.c              ioctl, flock, close_range
38. uiox_kix_scfs_mmap.c               mmap, munmap, msync, mprotect,
                                       madvise, mincore
Row 38 ends the list at 38 numbered entries, and the file count is 39 — the arithmetic: 3 headers + 3 tables + 18 algorithms + 15 remainder = 39.

===========
00_buffcache — 8 files
Bach Ch.3 — the block buffer cache.



include/         bcache.h                    11.7 KB   public interface, BufHdr, 5 states
                 bcache_internal.h            5.7 KB   pool + hash/free-list ops, extern geom

buffers/src/     getblk.c                     9.7 KB   getblk (5 scenarios, bounded spin)
                 brelse.c                     3.8 KB   brelse (LRU tail/head placement)
                 bread.c                      3.7 KB   bread
                 breada.c                     6.4 KB   breada (read + read-ahead)
                 bwrite.c                     6.8 KB   bwrite + bdwrite + bflush
                 bcache_init.c               13.8 KB   the pool, sentinels, platform hooks
01_fsa — 10 files
Bach Ch.4 — the eight inode-level algorithms across six sources.



include/         inode.h                      7.4 KB   DiskInode, InCoreInode (both with dev)
                 bmap.h                       3.3 KB   BmapResult (with dev)
                 superblock.h                 9.8 KB   SuperBlock + alloc/free/ialloc/ifree
                 readwrite.h                  2.4 KB   readi/writei/readi_at/writei_at

src/             inode.c                     14.4 KB   iget, iput, iupdate, inode_disk_read,
                                                       inode_access_ok, inode_cache_init
                 bmap.c                      13.1 KB   bmap, bmap_alloc
                 namei.c                     15.6 KB   namei, dir_lookup, dir_add,
                                                       dir_remove, fs_mkfs
                 superblock.c                22.9 KB   alloc, free, ialloc, ifree,
                                                       fs_free_inode_blocks, sb_get
                 readwrite.c                 10.3 KB   Bach's I/O loop via breada
                 sb_access.c                  4.3 KB   named accessors for statfs
10_scfs — 39 files
Bach Ch.5 — the three kernel data structures and the syscall bodies.

include/ — 4



uiox_kix_scfs.h              21.1 KB   the 3 structs, flags, codes, all prototypes
uiox_kix_scfs_internal.h      7.4 KB   the 01_fsa contract + SCFS helpers
uiox_kix_scfs_dispatch.h      6.7 KB   syscall numbers, scfs_sysent_t, carrier
uiox_kix_scfs_stat.h          7.1 KB   SCFS_STAT_SZ / SCFS_STATFS_SZ + offsets
src/ — the three tables, 3



uiox_kix_scfs_table.c            16.3 KB   file table, fd table, mount table,
                                           scfs_getf, scfs_falloc, scfs_init
uiox_kix_scfs_dispatch_table.c   21.8 KB   sysent[] (43 rows), scfs_dispatch,
                                           scfs_gap_table, scfs_syscall_init
uiox_kix_scfs_syscalls.c         11.0 KB   every sys_* alias
src/ — Bach's algorithms, 18



uiox_kix_scfs_open.c         4.5 KB   uiox_kix_scfs_chown.c       3.0 KB
uiox_kix_scfs_creat.c        3.7 KB   uiox_kix_scfs_chmod.c       3.2 KB
uiox_kix_scfs_close.c        2.5 KB   uiox_kix_scfs_stat.c        6.9 KB
uiox_kix_scfs_read.c         6.5 KB   uiox_kix_scfs_pipe.c        5.0 KB
uiox_kix_scfs_write.c        5.5 KB   uiox_kix_scfs_dup.c         3.7 KB
uiox_kix_scfs_lseek.c        2.3 KB   uiox_kix_scfs_mount.c       5.3 KB
uiox_kix_scfs_link.c         5.0 KB   uiox_kix_scfs_umount.c      5.7 KB
uiox_kix_scfs_unlink.c       5.0 KB   uiox_kix_scfs_mknod.c       6.1 KB
uiox_kix_scfs_chdir.c        3.1 KB   uiox_kix_scfs_chroot.c      3.5 KB
src/ — the remainder, 14



uiox_kix_scfs_openat.c       6.7 KB   uiox_kix_scfs_sync.c        8.2 KB
uiox_kix_scfs_fcntl.c        4.9 KB   uiox_kix_scfs_statfs.c      6.4 KB
uiox_kix_scfs_truncate.c     7.6 KB   uiox_kix_scfs_xattr.c       7.7 KB
uiox_kix_scfs_access.c       6.3 KB   uiox_kix_scfs_ioctl.c       5.3 KB
uiox_kix_scfs_utime.c        4.3 KB   uiox_kix_scfs_mmap.c        6.1 KB
uiox_kix_scfs_mkdir.c        8.1 KB   uiox_kix_scfs_getdents64.c  7.9 KB
uiox_kix_scfs_fchdir.c       7.2 KB   uiox_kix_scfs_rename.c      9.3 KB

Counts
Layer	Headers	Sources	Total
00_buffcache	2	6	8
01_fsa	4	6	10
10_scfs	4	35	39
			57