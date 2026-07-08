/**
 * @file  uix_posix_io.h
 * @brief UIOX POSIX — I/O syscall wrappers (read, write, open, close,
 *        lseek, dup, dup2, pread, pwrite, readv, writev, pipe).
 *
 * Wires POSIX function signatures → SYS_* numbers → uix_syscallN().
 * Place: 50_UIX/00_libs/00_uixlibs/PoStd/uix_posix_io.h
 */

 #ifndef UIX_POSIX_IO_H
 #define UIX_POSIX_IO_H
 
 #include "uix_syscall.h"
 #include "uix_stddef.h"
 #include "uix_stdint.h"
 #include "uix_fcntl.h"
 #include "../sys/uix_types.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * off_t, ssize_t (defined in sys/uix_types.h — declared here for clarity)
  * ====================================================================== */
 
 #ifndef _UIX_SSIZE_T_DEFINED
 #define _UIX_SSIZE_T_DEFINED
 typedef long ssize_t;
 #endif
 
 #ifndef _UIX_OFF_T_DEFINED
 #define _UIX_OFF_T_DEFINED
 typedef long long off_t;
 #endif
 
 /* =========================================================================
  * readv / writev support
  * ====================================================================== */
 
 struct uix_iovec {
     void   *iov_base;
     size_t  iov_len;
 };
 
 /* =========================================================================
  * POSIX I/O function prototypes
  * ====================================================================== */
 
 /* Core I/O */
 ssize_t  uix_read    (int fd, void *buf, size_t count);
 ssize_t  uix_write   (int fd, const void *buf, size_t count);
 int      uix_open    (const char *path, int flags, ...); /* mode optional */
 int      uix_close   (int fd);
 off_t    uix_lseek   (int fd, off_t offset, int whence);
 
 /* Duplicate file descriptors */
 int      uix_dup     (int oldfd);
 int      uix_dup2    (int oldfd, int newfd);
 int      uix_dup3    (int oldfd, int newfd, int flags);
 
 /* Positioned I/O */
 ssize_t  uix_pread   (int fd, void *buf, size_t count, off_t offset);
 ssize_t  uix_pwrite  (int fd, const void *buf, size_t count, off_t offset);
 
 /* Scatter / gather I/O */
 ssize_t  uix_readv   (int fd, const struct uix_iovec *iov, int iovcnt);
 ssize_t  uix_writev  (int fd, const struct uix_iovec *iov, int iovcnt);
 
 /* Pipes */
 int      uix_pipe    (int pipefd[2]);
 int      uix_pipe2   (int pipefd[2], int flags);
 
 /* File truncation */
 int      uix_truncate  (const char *path, off_t length);
 int      uix_ftruncate (int fd, off_t length);
 
 /* Sync */
 void     uix_sync    (void);
 int      uix_fsync   (int fd);
 int      uix_fdatasync(int fd);
 
 /* fcntl / ioctl */
 int      uix_fcntl   (int fd, int cmd, ...);
 int      uix_ioctl   (int fd, unsigned long request, ...);
 
 /* POSIX aliases (map uix_* → standard names for source compat) */
 #define read        uix_read
 #define write       uix_write
 #define open        uix_open
 #define close       uix_close
 #define lseek       uix_lseek
 #define dup         uix_dup
 #define dup2        uix_dup2
 #define pread       uix_pread
 #define pwrite      uix_pwrite
 #define readv       uix_readv
 #define writev      uix_writev
 #define pipe        uix_pipe
 #define fsync       uix_fsync
 #define fdatasync   uix_fdatasync
 #define fcntl       uix_fcntl
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIX_POSIX_IO_H */
 