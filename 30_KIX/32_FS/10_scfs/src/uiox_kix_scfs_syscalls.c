/*
 * 30_KIX/32_FS/10_scfs/src/uiox_kix_scfs_syscalls.c
 *
 * SCFS — the sys_* alias bodies.
 * Bach, The Design of the UNIX Operating System, Ch.5 §5.
 *
 * ── why these exist as a separate unit ─────────────────────────────────
 * Bach's kernel has ONE name per system call, and it is the sysent[]
 * entry.  This tree has two layers of naming on purpose:
 *
 *   uiox_kix_scfs_open()   the algorithm, named for the layer, called by
 *                          other kernel code (the shell, an init path,
 *                          a test) that is not entering through the trap
 *   sys_open()             the name arch assembly and the user-space
 *                          stub expect to find in the symbol table
 *
 * The split is a build convenience, not a design: a trap handler that
 * calls the entry directly gets uiox_kix_scfs_*; linking against an arch
 * shim that expects sys_* gets the wrapper.  Each alias is one line of
 * forwarding, so there is exactly one implementation of every algorithm
 * and no chance of the two drifting.
 *
 * ── the ones that return a value rather than a code ────────────────────
 * lseek returns the new offset; read/write return a byte count; umask
 * returns the previous mask; mmap returns an address.  Those aliases
 * forward the return value unchanged — an alias that clamped or
 * re-coded would be a second implementation in disguise.
 *
 * @version 1.0.0  @date 2026-09-23
 */
#include "uiox_kix_scfs_internal.h"

/* ═════════════════════════════════════════════════════════════════════
 * Descriptors and I/O
 * ═════════════════════════════════════════════════════════════════════ */
int  sys_open (const char *path, int flags, uint16_t mode)
{ return uiox_kix_scfs_open(path, flags, mode); }

int  sys_close(int fd)
{ return uiox_kix_scfs_close(fd); }

int32_t sys_read (int fd, char *buf, uint32_t count)
{ return uiox_kix_scfs_read(fd, buf, count); }

int32_t sys_write(int fd, const char *buf, uint32_t count)
{ return uiox_kix_scfs_write(fd, buf, count); }

int32_t sys_pread (int fd, char *buf, uint32_t count, uint32_t off)
{ return uiox_kix_scfs_pread(fd, buf, count, off); }

int32_t sys_pwrite(int fd, const char *buf, uint32_t count, uint32_t off)
{ return uiox_kix_scfs_pwrite(fd, buf, count, off); }

int32_t sys_readv (int fd, const void *iov, int iovcnt)
{ return uiox_kix_scfs_readv(fd, iov, iovcnt); }

int32_t sys_writev(int fd, const void *iov, int iovcnt)
{ return uiox_kix_scfs_writev(fd, iov, iovcnt); }

int  sys_lseek(int fd, int32_t offset, int whence)
{ return uiox_kix_scfs_lseek(fd, offset, whence); }

int  sys_creat(const char *path, uint16_t mode)
{ return uiox_kix_scfs_creat(path, mode); }

int  sys_dup (int fd)
{ return uiox_kix_scfs_dup(fd); }

int  sys_dup2(int oldfd, int newfd)
{ return uiox_kix_scfs_dup2(oldfd, newfd); }

int  sys_fcntl(int fd, int cmd, int arg)
{ return uiox_kix_scfs_fcntl(fd, cmd, arg); }

int  sys_ioctl(int fd, uint32_t cmd, void *arg)
{ return uiox_kix_scfs_ioctl(fd, cmd, arg); }

int  sys_pipe(int *fds)
{ return uiox_kix_scfs_pipe(fds); }

/* ═════════════════════════════════════════════════════════════════════
 * Names and directories
 * ═════════════════════════════════════════════════════════════════════ */
int  sys_link  (const char *oldp, const char *newp)
{ return uiox_kix_scfs_link(oldp, newp); }

int  sys_unlink(const char *path)
{ return uiox_kix_scfs_unlink(path); }

int  sys_rename(const char *oldp, const char *newp)
{ return uiox_kix_scfs_rename(oldp, newp); }

int  sys_mkdir (const char *path, uint16_t mode)
{ return uiox_kix_scfs_mkdir(path, mode); }

int  sys_rmdir (const char *path)
{ return uiox_kix_scfs_rmdir(path); }

int  sys_mknod (const char *path, uint16_t mode, uint8_t major, uint8_t minor)
{ return uiox_kix_scfs_mknod(path, mode, major, minor); }

int  sys_mkfifo(const char *path, uint16_t mode)
{ return uiox_kix_scfs_mkfifo(path, mode); }

int  sys_chdir (const char *path)
{ return uiox_kix_scfs_chdir(path); }

int  sys_fchdir(int fd)
{ return uiox_kix_scfs_fchdir(fd); }

int  sys_chroot(const char *path)
{ return uiox_kix_scfs_chroot(path); }

int  sys_getcwd(char *buf, uint32_t size)
{ return uiox_kix_scfs_getcwd(buf, size); }

int  sys_getdents64(int fd, uint8_t *out, uint32_t cap, uint32_t *off)
{ return uiox_kix_scfs_getdents64(fd, out, cap, off); }

/* ── the *at() family ───────────────────────────────────────────────── */
int  sys_openat(int dirfd, const char *path, int flags, uint16_t mode)
{ return uiox_kix_scfs_openat(dirfd, path, flags, mode); }

int  sys_unlinkat(int dirfd, const char *path, int flags)
{ return uiox_kix_scfs_unlinkat(dirfd, path, flags); }

int  sys_mkdirat(int dirfd, const char *path, uint16_t mode)
{ return uiox_kix_scfs_mkdirat(dirfd, path, mode); }

int  sys_faccessat(int dirfd, const char *path, int mode, int flags)
{ return uiox_kix_scfs_faccessat(dirfd, path, mode, flags); }

int  sys_renameat(int odirfd, const char *oldp, int ndirfd, const char *newp)
{ return uiox_kix_scfs_renameat(odirfd, oldp, ndirfd, newp); }

int  sys_linkat(int odirfd, const char *oldp, int ndirfd, const char *newp, int flags)
{ return uiox_kix_scfs_linkat(odirfd, oldp, ndirfd, newp, flags); }

/* ═════════════════════════════════════════════════════════════════════
 * Status, metadata, durability
 * ═════════════════════════════════════════════════════════════════════ */
int  sys_stat (const char *path, void *buf)
{ return uiox_kix_scfs_stat(path, buf); }

int  sys_fstat(int fd, void *buf)
{ return uiox_kix_scfs_fstat(fd, buf); }

int  sys_lstat(const char *path, void *buf)
{ return uiox_kix_scfs_lstat(path, buf); }

int  sys_chmod(const char *path, uint16_t mode)
{ return uiox_kix_scfs_chmod(path, mode); }

int  sys_fchmod(int fd, uint16_t mode)
{ return uiox_kix_scfs_fchmod(fd, mode); }

int  sys_chown(const char *path, uint16_t uid, uint16_t gid)
{ return uiox_kix_scfs_chown(path, uid, gid); }

int  sys_fchown(int fd, uint16_t uid, uint16_t gid)
{ return uiox_kix_scfs_fchown(fd, uid, gid); }

int  sys_access(const char *path, int mode)
{ return uiox_kix_scfs_access(path, mode); }

uint16_t sys_umask(uint16_t mask)
{ return uiox_kix_scfs_umask(mask); }

int  sys_truncate (const char *path, uint32_t len)
{ return uiox_kix_scfs_truncate(path, len); }

int  sys_ftruncate(int fd, uint32_t len)
{ return uiox_kix_scfs_ftruncate(fd, len); }

int  sys_utime (const char *path, const int64_t *t)
{ return uiox_kix_scfs_utime(path, t); }

int  sys_utimes(const char *path, const int64_t *t)
{ return uiox_kix_scfs_utimes(path, t); }

/* ── the forwarders that were only ever inline in their own unit ────── */
/*
 * These were defined at the bottom of the algorithm unit that owns them
 * and were not carried over when the aliases were consolidated here.
 * Kept, so a caller reaching for them links.
 */
int  sys_futimes (int fd, const int64_t *t)
{ return uiox_kix_scfs_futimes(fd, t); }

int  sys_futimens(int fd, const int64_t *t)
{ return uiox_kix_scfs_futimens(fd, t); }

int  sys_setgid(const char *path, uint16_t gid)
{ return uiox_kix_scfs_setgid(path, gid); }

int  sys_dup3(int oldfd, int newfd, int flags)
{ return uiox_kix_scfs_dup3(oldfd, newfd, flags); }

int  sys_fallocate(int fd, int mode, uint32_t off, uint32_t len)
{ return uiox_kix_scfs_fallocate(fd, mode, off, len); }

void sys_sync(void)
{ uiox_kix_scfs_sync(); }

int  sys_fsync(int fd)
{ return uiox_kix_scfs_fsync(fd); }

int  sys_fdatasync(int fd)
{ return uiox_kix_scfs_fdatasync(fd); }

int  sys_statfs (const char *path, void *buf)
{ return uiox_kix_scfs_statfs(path, buf); }

int  sys_fstatfs(int fd, void *buf)
{ return uiox_kix_scfs_fstatfs(fd, buf); }

int  sys_mount (const char *dev, const char *dir, int flags)
{ return uiox_kix_scfs_mount(dev, dir, flags); }

int  sys_umount(const char *dir)
{ return uiox_kix_scfs_umount(dir); }

/* ── extended attributes ────────────────────────────────────────────── */
int  sys_setxattr(const char *p, const char *n, const void *v, uint32_t sz, int fl)
{ return uiox_kix_scfs_setxattr(p, n, v, sz, fl); }

int  sys_getxattr(const char *p, const char *n, void *v, uint32_t sz)
{ return uiox_kix_scfs_getxattr(p, n, v, sz); }

int  sys_listxattr(const char *p, char *l, uint32_t sz)
{ return uiox_kix_scfs_listxattr(p, l, sz); }

int  sys_removexattr(const char *p, const char *n)
{ return uiox_kix_scfs_removexattr(p, n); }

/* ── the calls whose return VALUE is the answer ─────────────────────── */
/*
 * Bach's lseek returns the new offset; read and write return a count;
 * mmap returns an address.  These forward the value unchanged — an alias
 * that converted or clamped the result would be a second implementation
 * of the algorithm, which is exactly what this unit exists to prevent.
 */
void *sys_mmap(void *addr, uint32_t len, int prot, int flags, int fd, uint32_t off)
{ return uiox_kix_scfs_mmap(addr, len, prot, flags, fd, off); }

int  sys_munmap(void *addr, uint32_t len)
{ return uiox_kix_scfs_munmap(addr, len); }

int  sys_msync(void *addr, uint32_t len, int flags)
{ return uiox_kix_scfs_msync(addr, len, flags); }

int  sys_mprotect(void *addr, uint32_t len, int prot)
{ return uiox_kix_scfs_mprotect(addr, len, prot); }

int  sys_flock(int fd, int op)
{ return uiox_kix_scfs_flock(fd, op); }
