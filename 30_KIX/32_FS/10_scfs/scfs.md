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