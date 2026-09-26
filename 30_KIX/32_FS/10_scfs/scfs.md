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

      =========


      What I recommend, and why
Rather than renumbering 43 rows to match, delete the dispatch from 10_scfs entirely and let SCXI own it. That's your stated design, and it removes the duplicate rather than synchronising it — a synchronised duplicate drifts again the moment either side changes.

Concretely, across the seven files:

File	Action
1. uiox_kix_scfs.h	keep the 3 structs and the uiox_kix_scfs_* prototypes; delete the O_*/F_*/SCFS_F_* blocks if SCXI's PoStd/ headers own them
2. uiox_kix_scfs_internal.h	unchanged — it declares no syscall content
3. uiox_kix_scfs_dispatch.h	delete — SCXI owns the numbers
4. uiox_kix_scfs_stat.h	keep SCFS_STAT_SZ/SCFS_STATFS_SZ; drop the five sys_stat-style declarations
5. uiox_kix_scfs_table.c	unchanged — the three Bach structures, no syscall content
6. uiox_kix_scfs_dispatch_table.c	delete — s_syscall_table[] is SCXI's
7. uiox_kix_scfs_syscalls.c	delete — sys_*() lives in 50_UIX/00_libs/
10_scfs then exports exactly what your transcript says it should: open, read, write, close, fsync-shaped entry points that SCXI calls into, plus the three tables and the algorithm bodies.


=============================================
okey can you tell me now why need unfs files as mentioned here https://github.com/Pramod645/UIOX/tree/main/30_KIX/32_FS/10_unfs wehn we have files and for kernel file systen in 00_buffCache, 01_fsa and 10_scfs?


Short answer: they're two different layers of the same stack, and 10_unfs sits below the three you've built.

What each layer does
00_buffcache → blocks. 01_fsa → inodes and directory entries. 10_scfs → the syscall bodies that manipulate file descriptors.

None of those three knows what's actually on the disk. 01_fsa's bmap() can tell you a file's byte 4096 lives in block 37. It cannot tell you how block 37 is laid out — where the inode table starts, whether there's a block bitmap, how many block groups exist, whether there's a journal, what the super block's magic number is.

That's 10_unfs's job. It's the on-disk format — the thing that turns a bare block device into a filesystem.

Why it can't be folded into 01_fsa
Bach's 01_fsa is deliberately format-agnostic. iget, bmap, namei, ialloc describe algorithms, not a layout. Bach's own kernel had exactly this split: the inode cache and block map were generic, while the disk layout underneath was the filesystem's business.

If you put the format inside 01_fsa, you get one filesystem and no way to have a second. Your tree already has 03_netfs (a 9P client) sitting beside it — that only makes sense if the format layer is separable.

The concrete things that would have nowhere to live
Concern	Who owns it
super block magic, geometry, feature flags	10_unfs
inode table location and stride	10_unfs
block allocation bitmap	10_unfs
directory entry on-disk encoding	10_unfs
journal / crash consistency	10_unfs
snapshots, copy-on-write, xattr storage	10_unfs
Several of these are already gaps in what you have. Your xattr.c returns ENOSYS because InCoreInode has no i_xattr field — that field is an unfs concept. Your pipe returns ENOSYS for the same reason. And 02_jrnal exists in the tree precisely because crash consistency is a storage-layer problem, not an inode-layer one.

The layering


10_scfs      file descriptors, the 3 Bach tables, syscall bodies
01_fsa       inode cache, block map, namei, alloc — FORMAT-AGNOSTIC
10_unfs      the on-disk format: layout, bitmap, journal, xattr storage
00_buffcache 512-byte blocks, cache, device I/O
01_fsa calls down into 10_unfs for "what's the layout of this block"; 10_unfs calls down into 00_buffcache for the bytes. So 10_unfs is a layer between 01_fsa and 00_buffcache — which is why it isn't redundant.

One caveat about your current build
Your Makefile builds only the three layers you named, and Section 3 says 10_unfs is "not built here — sits UNDER 01_fsa once integrated." That's the accurate state: 01_fsa today assumes a layout implicitly (the geometry in fs_types.h — INODE_START_BLOCK, DATA_START_BLOCK, BLOCK_SIZE) rather than calling into a format layer. That's fine for bring-up, and it's why 10_unfs can wait.

So it isn't needed yet — it's needed the moment you want a second filesystem, a journal, or xattrs.




understand , analysis this unfs also with regard to what we have in bootloader https://github.com/Pramod645/UIOX/tree/main/01_uBoot/include have uiox_boot_unfs.h and uiox_boot_unfs.c files


UNFS exists in two places in your tree, and that's the interesting part: the bootloader has its own copy.

Why a bootloader needs unfs at all
The bootloader must read a kernel image off the storage device before any kernel exists. It has no inode cache, no buffer cache, no 01_fsa — none of that is running yet. So it needs a minimal, standalone reader for the same on-disk format.

That's 01_uBoot/include/uiox_boot_unfs.h + .c: enough UNFS to walk the super block, find an inode, and read its data blocks. Not ialloc, not bmap_alloc — just open, read, close against a read-only device.

The duplication is deliberate, and it's the normal pattern
Layer	What it has	Why
01_uBoot	a minimal UNFS reader	no kernel services exist yet
32_FS/10_unfs	the full UNFS implementation	runs under 01_fsa, full read/write
A bootloader cannot call into 10_unfs, because 10_unfs calls 00_buffcache, which calls device drivers that the kernel brings up later. The boot path is strictly earlier than all of it.

GRUB has its own ext4 reader for exactly this reason. U-Boot has its own FAT. Neither calls the kernel's filesystem code.

What the bootloader copy must get right
The on-disk format is the contract between the two. If 10_unfs writes a super block one way and uiox_boot_unfs.c reads it another, the bootloader silently loads garbage. So the two must agree on:

the super block offset and magic
the inode number → disk location arithmetic (INODES_PER_BLOCK, INODE_START_BLOCK — the same formula in your inode_disk_read)
the block size (512, from BCACHE_SECTOR_SIZE)
the directory entry encoding
any feature flag that changes layout — this is where versioning matters
That last point is the risk. If 10_unfs gains journaling or an xattr area and moves the data start block, the bootloader's copy must know, or it reads the wrong block.

What this tells you about your build today
Your fs_types.h already fixes the geometry — BLOCK_SIZE, INODE_START_BLOCK 2, DATA_START_BLOCK 16, MAX_INODES 128. Those constants are the de facto UNFS format right now, even though no 10_unfs file exists. Which means:

The bootloader and the kernel are both reading whatever fs_types.h says. That's the real format definition at this moment, and it works only while both sides include the same header.

So the ordering when you do write 10_unfs is: decide the format first, write it into one shared header both sides include, then implement. If 10_unfs defines its own geometry separately from the bootloader's, you'll have two formats and no way to detect the drift until a board fails to boot.

The concrete recommendation
10_unfs should export the format as a header that the bootloader also includes — super block struct, magic, geometry, feature bits — with the implementations separate. That's how you keep the two readers in sync without either depending on the other's code.

Can you paste uiox_boot_unfs.h? That tells me what format the bootloader already commits to, and whether it disagrees with fs_types.h anywhere — which is the thing worth checking before 10_unfs is written.




you can analysis in the way that unfs have replace the fat32 in my kernel and os system


UNFS replacing FAT32 is a defensible design decision, and worth understanding why it's the right one for your system rather than just accepting it.

What FAT32 gives you, and why it stops being enough
FAT32 is the default boot filesystem because it has one enormous virtue: everybody implements it. Firmware, bootloaders, cameras, SD card controllers — anything with a card slot can read it. UEFI mandates it for the EFI system partition. That's a compatibility argument, not a technical one.

But FAT32 has hard ceilings that bite as the OS grows:

No permissions, no ownership. There is no uid, gid, or mode. Any access control has to live somewhere else, which means it can't be enforced by the filesystem.

No hard links, no real symlinks. FAT has neither. Your link() and symlink() syscalls have nothing to build on.

No atomic operations. Renaming is a copy-and-delete. There is no journal, so a power loss during a write leaves a corrupt cluster chain with no recovery path. That is the failure mode embedded systems actually hit.

4 GB per-file, 2 TB per-volume. The 32-bit cluster arithmetic caps both.

Fragmentation degrades badly because the allocation table is a linked list of clusters. A fragmented file costs one seek per cluster.

Metadata is a strip of directory entries. Atime/mtime only — no ctime. No extended attributes. No way to record a device number.

What UNFS gives you instead
Every one of those ceilings is a thing your kernel already needs. UNFS is a Unix filesystem in Bach's lineage, so it has:

FAT32	UNFS
no permissions	uid/gid/mode in the inode — which your inode_access_ok reads
no links	i_nlink, so link() and unlink() work
no symlinks	FT_SYMLINK is a real FileType in your fs_types.h
no journal	02_jrnal sits beside it
linked-list allocation	a super block free list with a chain block — Bach's alloc/free
atime/mtime only	atime, mtime and ctime — your DiskInode carries all three
no xattr	an xattr chain (i_xattr — the field your xattr.c currently lacks)
device numbers nowhere	the dev field you added to InCoreInode
4 GB per file	direct + single + double + triple indirect, in bmap()
That last row is the point. Your bmap() already implements four levels of indirection — the same structure ext2 uses. That's a 4 GB+ addressing scheme, and it's already written. On FAT32 the equivalent would be a different disk format, not a code change.

The bootloader argument, which is the real one
You asked about 01_uBoot's uiox_boot_unfs.c earlier. That file exists because the boot path needs to read the kernel image, and — this is the crux — the same format serves both roles.

With FAT32 you have two filesystems: FAT on the boot partition, something else on the root partition. With UNFS you have one. The bootloader reads UNFS, the kernel runs UNFS, and mkfs produces one format.

That's a real simplification. It means one bmap() implementation, one inode layout, one set of geometry constants in fs_types.h — and no translation layer between a FAT boot image and a Unix root.

The one cost you're accepting
Nothing else can read your disk. Pull the SD card and put it in a laptop — it won't mount. fsck is yours to write, not something you can reach for. Recovery when the filesystem corrupts is your problem entirely.

FAT32's whole purpose is being universally readable. You're trading that for capabilities your OS actually uses. For an OS that owns its hardware end to end, that's usually the right trade — but it's worth naming, because it shapes your tooling: you'll need an fsck and a host-side image tool eventually, and FAT32 would have given you both for free.

Where this lands in your build
Your fs_types.h already commits to the UNFS layout — BLOCK_SIZE 512, INODE_START_BLOCK 2, DATA_START_BLOCK 16, MAX_INODES 128. Those constants are UNFS as far as both the bootloader and 01_fsa are concerned.

So the format is already decided, in effect. What 10_unfs would formalise is the super block (magic, geometry, feature flags) as a header both 01_uBoot and 32_FS include — so the two readers can never drift apart on layout. That's the piece worth writing first, before the full read/write implementation.




i glad you understand this ---> That file exists because the boot path needs to read the kernel image, and — this is the crux — the same format serves both roles. and 1_uBoot and 32_FS include — so the two readers can never drift apart on layout. That's the piece worth writing first, before the full read/write implementation.


Glad it landed. The shared format header is the piece to write first — and it's small.

What it needs to contain
One header, included by both 01_uBoot and 32_FS, holding the things that must never drift:

Item	Why both sides need it
magic number + version	the bootloader must reject a format it doesn't know
BLOCK_SIZE	the arithmetic both readers do
INODE_START_BLOCK, DATA_START_BLOCK	where the inode list ends and data begins
INODES_PER_BLOCK, MAX_INODES	inode number → disk offset
DiskInode layout	the on-disk struct, byte for byte
DirEntry layout	{ uint32 ino; char name[28]; }
feature bits	so a layout change is detectable, not silent
Right now those live in fs_types.h — which the kernel sees. Whether 01_uBoot includes the same file, or has its own copy, is the question worth checking before writing anything.

Two things I'd want to see first
uiox_boot_unfs.h — what format does the bootloader already commit to?

uiox_boot_unfs.c — specifically its inode-number arithmetic and super block read, so I can compare against fs_types.h line by line.

If they already agree, the job is extracting the shared constants into one header both include — a move, not a rewrite. If they disagree, that's a boot failure waiting to happen and the header fixes it before it bites.

Paste either file and I'll do the comparison, or say the word and I'll write the shared header from fs_types.h as the reference format.




On-disk layout:

Block 0 Superblock (unfs_sb_t)
Block 1 Journal superblock (reuse uiox_jr_sb_disk_t)
Block 2..257 Journal log area (256 × 4 KB = 1 MB)
Block 258 Block group 0 descriptor
Block 259 Block bitmap (group 0)
Block 260 Inode bitmap (group 0)
Block 261..268 Inode table (group 0, 8 blocks = 512 inodes)
Block 269+ Data blocks (group 0)
... Repeat for group 1, 2, …

This layout is ext2-shaped with a journal grafted on, and that's a good sign — it means the format is already designed, and the pieces map cleanly onto what you've built.

Reading the layout
Blocks 0–2..257 — the journal. 1 MB reserved at the front of the volume. That's a deliberate choice: the journal's position is fixed and known before anything else is parsed, so a recovery pass can run with no other metadata. uiox_jr_sb_disk_t reused as-is means your 02_jrnal and UNFS share one journal format.

Blocks 258–268 — block group 0. Descriptor, block bitmap, inode bitmap, inode table. Classic ext2 group structure: 8 blocks × 512 inodes = 4096 inodes per group at 64 bytes each.

Block 269+ — data. First data block of group 0, and the group repeats from there.

Three things worth checking against what you have
1. INODE_START_BLOCK and DATA_START_BLOCK are now wrong. Your fs_types.h says:

c


#define INODE_START_BLOCK   2
#define DATA_START_BLOCK    16
But this layout puts group 0's inode table at block 261 and data at 269. So inode_disk_read's arithmetic — ((ino-1)/INODES_PER_BLOCK) + INODE_START_BLOCK — currently reads block 2 for inode 1, which is now the journal log area. That's the drift I flagged: the bootloader and 01_fsa would read different bytes from what mkfs writes.

2. INODES_PER_BLOCK differs. At 512-byte blocks and a 64-byte DiskInode, that's 8 per block. Your fs_types.h computes it as BLOCK_SIZE / sizeof(DiskInode) — so it's already right if DiskInode is exactly 64 bytes. Worth a _Static_assert on that, because it silently changes if anyone adds a field.

3. The journal is 256 × 4 KB blocks — but your block size is 512. The layout text says "256 × 4 KB = 1 MB" yet places it at blocks 2..257, which at 512 bytes is 128 KB. Either the journal is 4 KB-unitised while the filesystem is 512-byte-unitised, or the block range should be 2..2050. That's an off-by-8 arithmetic mismatch worth resolving before mkfs writes it.

The header this argues for
One unfs_format.h, included by 01_uBoot, 10_unfs, and 02_jrnal, holding:

c


#define UNFS_MAGIC           0x554E4653u   /* 'UNFS' */
#define UNFS_VERSION         1u

#define UNFS_SB_BLOCK        0u
#define UNFS_JR_SB_BLOCK     1u
#define UNFS_JR_LOG_FIRST    2u
#define UNFS_JR_LOG_BLOCKS   256u
#define UNFS_GROUP0_DESC     258u
#define UNFS_GROUP0_BBMAP    259u
#define UNFS_GROUP0_IBMAP    260u
#define UNFS_GROUP0_ITABLE   261u
#define UNFS_ITABLE_BLOCKS   8u
#define UNFS_GROUP0_DATA     269u
#define UNFS_GROUP_BLOCKS    (UNFS_GROUP0_DATA - UNFS_GROUP0_DESC)

#define UNFS_MKFS_GEOMETRY   ...
plus unfs_sb_t, and a _Static_assert(sizeof(DiskInode) == 64, ...).

That makes the format one artefact, and both readers include it — which is the thing you said is worth writing first. It's also the only way that off-by-8 journal range gets caught before it's baked into an image.

Want me to write unfs_format.h from this layout? I'd need to see uiox_boot_unfs.h first so the constants match what the bootloader already assumes — that's the file that would otherwise drift.




so tell me now is unfs is equavalant to 00_buffCache and 01_fsa or 00_buffCache or 01_fsa?


Neither. UNFS isn't equivalent to either — it sits between them.

The three layers are at different heights


10_scfs        file descriptors, syscall bodies
01_fsa         inode cache, bmap, namei, ialloc   ← format-AGNOSTIC
10_unfs        the on-disk format                  ← UNFS is here
00_buffcache   512-byte blocks, cache, device I/O
00_buffcache moves blocks. It has no idea what's in them — a block is 512 bytes and a block number, full stop.

01_fsa moves inodes and offsets. Its bmap() turns a byte offset into a block number. Its iget() fills an InCoreInode. But ask it where the inode table lives and it has no answer — that arithmetic comes from constants in fs_types.h, not from anything 01_fsa understands.

10_unfs answers the questions neither can: where the super block is, where the inode table starts, how a block group is organised, what the journal does.

Why it can't be either one
Not 00_buffcache — the buffer cache operates at 512-byte granularity and knows nothing about grouping. Its job is "give me block 261, and cache it." Whether block 261 is an inode table block or file data is not its concern, and making it one would put filesystem semantics inside a device-caching layer.

Not 01_fsa — 01_fsa implements Bach's algorithms, which are deliberately layout-independent. iget, bmap, namei, ialloc would work over any Unix-like layout. Fold UNFS into it and you get exactly one filesystem, with no way to add 03_netfs beside it, and no place for the journal.

A concrete way to see it
Your layout says group 0's inode table is at block 261. Trace what each layer knows about that:

Layer	What it can say about block 261
00_buffcache	"it's cached, 512 bytes, device 0"
01_fsa	"inode 1 is at ((1-1)/8) + INODE_START_BLOCK" — and that constant is currently 2, which is wrong
10_unfs	"261 is UNFS_GROUP0_ITABLE; inode 1 is at offset 0 within it; the bitmap that says it's allocated is block 260"
01_fsa computes the offset. 10_unfs supplies the base. 00_buffcache fetches the bytes.

The practical consequence
That INODE_START_BLOCK 2 in fs_types.h is the drift I flagged. It's the value 01_fsa uses, and it predates this layout — under UNFS, block 2 is the journal log area.

Which is precisely why the shared format header matters: 01_uBoot, 10_unfs and 01_fsa all need the same answer to "where does the inode table start." If 01_fsa keeps its own constant while 10_unfs defines another, they disagree silently and the bootloader reads the journal as inodes.

So: UNFS is a fourth layer between 01_fsa and 00_buffcache, and the format header is the contract that keeps all three readers agreeing.




but you said unfs also have inode similar to 00_buffeCache


I didn't say that — and I want to correct it rather than let it stand, because the distinction matters.

What I actually said
I said 01_fsa has an inode cache — iget, iput, InCoreInode. That's 01_fsa. I never said 00_buffcache has inodes; it doesn't.

If a phrase of mine read that way, it was badly worded. Here's the precise split:

Layer	Has inodes?	Has the on-disk inode layout?
10_scfs	no — it has file table entries	no
01_fsa	yes — InCoreInode, iget, iput	no
10_unfs	no — it defines the format	yes — where the inode table is, its stride, its bitmap
00_buffcache	no — 512-byte blocks only	no
The distinction that matters
01_fsa's inode_disk_read() contains this:

c


blkno  = ((ino - 1u) / INODES_PER_BLOCK) + INODE_START_BLOCK;
offset = ((ino - 1u) % INODES_PER_BLOCK) * (uint32_t)sizeof(DiskInode);
buf    = bread(dev, blkno);
It knows the arithmetic. It does not know the values — INODE_START_BLOCK comes from fs_types.h, and INODES_PER_BLOCK is derived from sizeof(DiskInode).

That's exactly the seam UNFS fills. Under your layout, group 0's inode table is at block 261. But fs_types.h says INODE_START_BLOCK 2. So 01_fsa today computes block 2 for inode 1 — which your layout assigns to the journal log area.

01_fsa has inodes. 10_unfs says where they live. Two different jobs.

Why they can't be one layer
If the layout lived inside 01_fsa, then 01_fsa would know that group 0's inode table starts at 261 — and it would also need to know about block groups, the block bitmap, the inode bitmap, and the journal's position. That's the entire UNFS format inside Bach's algorithm layer, which is supposed to be format-agnostic.

It's also why the bootloader can't call into 01_fsa: 01_uBoot needs to find inode 1's block before any cache exists, so it needs the layout — the UNFS constants — without the algorithm layer around them.

So: an inode is a concept both 01_fsa and UNFS deal with, but at different levels. 01_fsa manipulates in-core inodes. UNFS says where the on-disk ones are stored, how big they are, and which bitmap tracks them.




then can you draw layout how calls happens from 10_scfs to 01_fas to 10_unfs to 00_buffCache?


Here's the call flow, from a syscall down to a 512-byte block.








Here's the call flow, from a syscall down to a 512-byte block.


                 USER PROGRAM
                      │   read(fd, buf, 4096)
                      ▼
┌─────────────────────────────────────────────────────────────────────┐
│ 40_SCIX  /  50_UIX                                                  │
│   SYS_READ = 3        sys_read()                                    │
└──────────────────────────────┬──────────────────────────────────────┘
                               │  sys_read → uiox_kix_scfs_read(fd,buf,n)
                               ▼
╔═════════════════════════════════════════════════════════════════════╗
║ 10_scfs   —  FILE LEVEL                  Bach Ch.5                 ║
║                                                                     ║
║   uiox_kix_scfs_read()                                              ║
║      │  scfs_getf(fd)              → file table entry               ║
║      │  check f_flag & FREAD                                        ║
║      │  readi_at(f->f_inode, buf, n, f->f_offset)                   ║
╚══════╪══════════════════════════════════════════════════════════════╝
       │            ┌──────────────────────────────────────────┐
       │            │  f_offset lives HERE — in the file table  │
       │            │  entry, not the inode. That's why dup()   │
       │            │  shares a position.                       │
       │            └──────────────────────────────────────────┘
       ▼
╔═════════════════════════════════════════════════════════════════════╗
║ 01_fsa   —  INODE LEVEL                  Bach Ch.3 + Ch.4          ║
║                                                                     ║
║   readi_at()                      ← readwrite.c                     ║
║      │                                                              ║
║      │  ┌── LOOP, once per block ─────────────────────────────┐     ║
║      │  │                                                      │     ║
║      │  │   ①  bmap(ip, off)          → BmapResult             │     ║
║      │  │        · direct block   ip->addr[n]                  │     ║
║      │  │        · 1× indirect    addr[10] → bread → ptrs[n]   │     ║
║      │  │        · 2× indirect    addr[11] → …                 │     ║
║      │  │        · 3× indirect    addr[12] → …                 │     ║
║      │  │        sets: .dev .blkno .blk_offset .io_bytes       │     ║
║      │  │              .readahead_blk                          │     ║
║      │  │                                                      │     ║
║      │  │   ②  rw_bread(&bm)                                   │     ║
║      │  │        if readahead_blk → breada(dev,blkno,ra)       │     ║
║      │  │        else            → bread (dev,blkno)           │     ║
║      │  │                                                      │     ║
║      │  │   ③  for i < n:  kbuf[i] = buf->data[blk_off+i]     │     ║
║      │  │   ④  brelse(buf)                                     │     ║
║      │  └──────────────────────────────────────────────────────┘     ║
╚══════╪══════════════════════════════════════════════════════════════╝
       │
       │  ═══════════ THE SEAM — where 10_unfs belongs ═══════════
       │
       │  bmap() computes  ((off / BLOCK_SIZE) → blkno)  and reads
       │  ip->addr[]. It knows the ARITHMETIC.
       │
       │  It does NOT know:  which block the inode table starts at,
       │  how big a block group is, where the bitmaps live, whether
       │  the journal has moved the data start.
       │
       │      fs_types.h today:  INODE_START_BLOCK 2
       │      UNFS layout:        group 0 inode table = block 261
       │
       │                    ▲ THIS IS THE DRIFT
       │
       ▼
╔═════════════════════════════════════════════════════════════════════╗
║ 10_unfs   —  ON-DISK FORMAT              (the missing layer)        ║
║                                                                     ║
║   block 0        superblock          unfs_sb_t                      ║
║   block 1        journal superblock  uiox_jr_sb_disk_t              ║
║   block 2..257   journal log area    256 blocks                     ║
║   block 258      block group 0 descriptor                           ║
║   block 259      block bitmap        (group 0)                      ║
║   block 260      inode bitmap        (group 0)                      ║
║   block 261..268 inode table         (group 0, 8 blks = 512 inodes) ║
║   block 269+     data blocks                                        ║
║   ...            group 1, 2, …                                       ║
║                                                                     ║
║   Answers:  where is inode N?  is block B free?  what's the magic?  ║
╚══════╪══════════════════════════════════════════════════════════════╝
       │  bread(dev, blkno)   /   bwrite(buf, sync, delayed)
       ▼
╔═════════════════════════════════════════════════════════════════════╗
║ 00_buffcache  —  BLOCK LEVEL             Bach Ch.3                  ║
║                                                                     ║
║   getblk(dev,blkno)   5 scenarios                                    ║
║      ├─ S1  on hash queue, free      → lock, return                 ║
║      ├─ S2  not cached, free buf     → reassign                     ║
║      ├─ S3  not cached, buf DELWRITE → flush, retry                 ║
║      ├─ S4  free list EMPTY          → spin (bounded)               ║
║      └─ S5  on queue but LOCKED      → spin (bounded)               ║
║                                                                     ║
║   ┌──────────────┐   ┌──────────────┐   ┌──────────────────┐        ║
║   │ hash_heads[] │   │ free_head    │   │ bcache_pool[256] │        ║
║   │ 64 queues    │   │ LRU doubly   │   │ BufHdr × 256     │        ║
║   │ sentinel-    │   │ circular     │   │ status word of   │        ║
║   │ terminated   │   │ sentinel     │   │ BUF_* bits       │        ║
║   └──────────────┘   └──────────────┘   └──────────────────┘        ║
╚══════╪══════════════════════════════════════════════════════════════╝
       │  bcache_plat_read_block(dev, blkno, buf)   ← WEAK, 10_BSP wins
       ▼
┌─────────────────────────────────────────────────────────────────────┐
│ DEVICE       eMMC / SD / DRAM stub                                  │
│              512-byte sectors                                       │
└─────────────────────────────────────────────────────────────────────┘



The return path, and where each field comes from
   buf->data[blk_off … +n]      ← 00_buffcache: 512 bytes off the device
          │
          ▼  copied into kbuf
   kbuf[0..n]                   ← 01_fsa: offset within the block resolved
          │
          ▼  back up through scfs
   f->f_offset += moved         ← 10_scfs: the file table entry advances
          │
          ▼
   return (int32_t)moved        ← SCIX reports the byte count



Three things this picture makes visible
bmap() is the hinge. It is the only place where a file offset becomes a block number — and it's the only caller that needs both ip->addr[] (inode layout) and BLOCK_SIZE / INODE_START_BLOCK (format geometry). That's why the 10_unfs layer sits exactly there.

The drift is arithmetic, not structural. fs_types.h says the inode table starts at block 2. The UNFS layout says block 261. 01_fsa's inode_disk_read() will happily compute block 2 — because it trusts a constant. Nothing detects the disagreement; the read just returns the wrong bytes.

10_unfs is a peer of 01_fsa, not a child. 01_fsa calls down into it for layout, and 10_unfs calls down into 00_buffcache for bytes. Two layers both touch bread(), which is why Buffer.h is the shared contract on that side.

The one entry point that skips a layer
01_uBoot calls 10_unfs's layout constants directly, with no 01_fsa and no 00_buffcache — because at boot there is no cache. That's why the format must live in a header both include, rather than inside 01_fsa's code.