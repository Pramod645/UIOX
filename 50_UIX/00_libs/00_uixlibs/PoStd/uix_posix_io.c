/**
 * @file  uix_posix_io.c
 * @brief UIOX POSIX — I/O syscall wrapper implementations.
 *
 * Each wrapper:
 *   1. Calls uix_syscallN with the correct SYS_* number.
 *   2. On error (ret < 0): sets uix_errno = -ret, returns -1.
 *   3. On success: returns the raw kernel value.
 */

 #include "uix_posix_io.h"
 #include "uix_stdarg.h"
 
 /* ── Helper: set errno and return -1 ─────────────────────────── */
 static inline long _ret(long r)
 {
     if (r < 0L) { uix_errno = (int)(-r); return -1L; }
     return r;
 }
 
 /* =========================================================================
  * Core I/O
  * ====================================================================== */
 
 ssize_t uix_read(int fd, void *buf, size_t count)
 {
     return (ssize_t)_ret(uix_syscall3(SYS_READ,
                                        (long)fd, (long)buf, (long)count));
 }
 
 ssize_t uix_write(int fd, const void *buf, size_t count)
 {
     return (ssize_t)_ret(uix_syscall3(SYS_WRITE,
                                        (long)fd, (long)buf, (long)count));
 }
 
 int uix_open(const char *path, int flags, ...)
 {
     va_list ap;
     va_start(ap, flags);
     int mode = va_arg(ap, int);
     va_end(ap);
     return (int)_ret(uix_syscall3(SYS_OPEN,
                                    (long)path, (long)flags, (long)mode));
 }
 
 int uix_close(int fd)
 {
     return (int)_ret(uix_syscall1(SYS_CLOSE, (long)fd));
 }
 
 off_t uix_lseek(int fd, off_t offset, int whence)
 {
     return (off_t)_ret(uix_syscall3(SYS_LSEEK,
                                      (long)fd, (long)offset, (long)whence));
 }
 
 /* =========================================================================
  * Duplicate file descriptors
  * ====================================================================== */
 
 int uix_dup(int oldfd)
 {
     return (int)_ret(uix_syscall1(SYS_DUP, (long)oldfd));
 }
 
 int uix_dup2(int oldfd, int newfd)
 {
     return (int)_ret(uix_syscall2(SYS_DUP2, (long)oldfd, (long)newfd));
 }
 
 int uix_dup3(int oldfd, int newfd, int flags)
 {
     return (int)_ret(uix_syscall3(SYS_DUP3,
                                    (long)oldfd, (long)newfd, (long)flags));
 }
 
 /* =========================================================================
  * Positioned I/O
  * ====================================================================== */
 
 ssize_t uix_pread(int fd, void *buf, size_t count, off_t offset)
 {
     return (ssize_t)_ret(uix_syscall4(SYS_PREAD,
                           (long)fd, (long)buf, (long)count, (long)offset));
 }
 
 ssize_t uix_pwrite(int fd, const void *buf, size_t count, off_t offset)
 {
     return (ssize_t)_ret(uix_syscall4(SYS_PWRITE,
                           (long)fd, (long)buf, (long)count, (long)offset));
 }
 
 /* =========================================================================
  * Scatter / Gather
  * ====================================================================== */
 
 ssize_t uix_readv(int fd, const struct uix_iovec *iov, int iovcnt)
 {
     return (ssize_t)_ret(uix_syscall3(SYS_READV,
                                        (long)fd, (long)iov, (long)iovcnt));
 }
 
 ssize_t uix_writev(int fd, const struct uix_iovec *iov, int iovcnt)
 {
     return (ssize_t)_ret(uix_syscall3(SYS_WRITEV,
                                        (long)fd, (long)iov, (long)iovcnt));
 }
 
 /* =========================================================================
  * Pipes
  * ====================================================================== */
 
 int uix_pipe(int pipefd[2])
 {
     return (int)_ret(uix_syscall1(SYS_PIPE, (long)pipefd));
 }
 
 int uix_pipe2(int pipefd[2], int flags)
 {
     return (int)_ret(uix_syscall2(SYS_PIPE2, (long)pipefd, (long)flags));
 }
 
 /* =========================================================================
  * Truncation
  * ====================================================================== */
 
 int uix_truncate(const char *path, off_t length)
 {
     return (int)_ret(uix_syscall2(SYS_TRUNCATE,
                                    (long)path, (long)length));
 }
 
 int uix_ftruncate(int fd, off_t length)
 {
     return (int)_ret(uix_syscall2(SYS_FTRUNCATE,
                                    (long)fd, (long)length));
 }
 
 /* =========================================================================
  * Sync
  * ====================================================================== */
 
 void uix_sync(void)
 {
     uix_syscall0(SYS_SYNC);
 }
 
 int uix_fsync(int fd)
 {
     return (int)_ret(uix_syscall1(SYS_FSYNC, (long)fd));
 }
 
 int uix_fdatasync(int fd)
 {
     return (int)_ret(uix_syscall1(SYS_FDATASYNC, (long)fd));
 }
 
 /* =========================================================================
  * fcntl / ioctl
  * ====================================================================== */
 
 int uix_fcntl(int fd, int cmd, ...)
 {
     va_list ap;
     va_start(ap, cmd);
     long arg = va_arg(ap, long);
     va_end(ap);
     return (int)_ret(uix_syscall3(SYS_FCNTL,
                                    (long)fd, (long)cmd, arg));
 }
 
 int uix_ioctl(int fd, unsigned long request, ...)
 {
     va_list ap;
     va_start(ap, request);
     long arg = va_arg(ap, long);
     va_end(ap);
     return (int)_ret(uix_syscall3(SYS_IOCTL,
                                    (long)fd, (long)request, arg));
 }
 