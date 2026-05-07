#ifndef UIOX_FILE_IO_H
#define UIOX_FILE_IO_H

/*
 * include/file_io.h
 *
 * Unix File I/O System Implementation
 * Based on "Advanced Programming in the UNIX Environment" 
 *
 * Provides the five fundamental file I/O functions:
 *   open, read, write, lseek, close
 * Plus supporting functions:
 *   creat, dup, dup2, fcntl, sync, fsync, fdatasync, ioctl
 *   pread, pwrite, openat
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>

/* =============================================================
 * File descriptor constants (POSIX.1)
 * ============================================================= */
#define STDIN_FILENO        0       /* standard input              */
#define STDOUT_FILENO       1       /* standard output             */
#define STDERR_FILENO       2       /* standard error              */
#define OPEN_MAX            256     /* max open files per process  */

/* =============================================================
 * File open flags (from <fcntl.h>)
 * ============================================================= */

/* Access mode flags — exactly one must be specified */
#define O_RDONLY            0x0000  /* open for reading only       */
#define O_WRONLY            0x0001  /* open for writing only       */
#define O_RDWR              0x0002  /* open for reading and writing*/
#define O_EXEC              0x0003  /* open for execute only       */
#define O_SEARCH            0x0004  /* open directory for search   */

#define O_ACCMODE           0x0007  /* mask for access mode bits   */

/* Optional flags */
#define O_APPEND            0x0008  /* append on each write        */
#define O_CLOEXEC           0x0010  /* set FD_CLOEXEC flag         */
#define O_CREAT             0x0020  /* create if doesn't exist     */
#define O_DIRECTORY         0x0040  /* error if not directory      */
#define O_EXCL              0x0080  /* error if O_CREAT + exists   */
#define O_NOCTTY            0x0100  /* don't make controlling tty  */
#define O_NOFOLLOW          0x0200  /* error if symbolic link      */
#define O_NONBLOCK          0x0400  /* nonblocking mode            */
#define O_SYNC              0x0800  /* sync data and attributes    */
#define O_TRUNC             0x1000  /* truncate to zero length     */
#define O_TTY_INIT          0x2000  /* set termios for terminal    */

/* Synchronized I/O flags (Single UNIX Specification) */
#define O_DSYNC             0x4000  /* sync data only              */
#define O_RSYNC             0x8000  /* sync reads and writes       */

/* Platform-specific flags */
#define O_FSYNC             0x0800  /* FreeBSD/Mac OS X (≡ O_SYNC) */
#define O_ASYNC             0x10000 /* asynchronous I/O            */
#define O_NDELAY            O_NONBLOCK /* legacy System V no-delay */

/* =============================================================
 * lseek whence values
 * ============================================================= */
#define SEEK_SET            0       /* offset from beginning       */
#define SEEK_CUR            1       /* offset from current position*/
#define SEEK_END            2       /* offset from end of file     */

/* =============================================================
 * fcntl command values
 * ============================================================= */
#define F_DUPFD             0       /* duplicate file descriptor   */
#define F_DUPFD_CLOEXEC     1       /* dup + set FD_CLOEXEC        */
#define F_GETFD             2       /* get file descriptor flags   */
#define F_SETFD             3       /* set file descriptor flags   */
#define F_GETFL             4       /* get file status flags       */
#define F_SETFL             5       /* set file status flags       */
#define F_GETOWN            6       /* get SIGIO/SIGURG owner      */
#define F_SETOWN            7       /* set SIGIO/SIGURG owner      */
#define F_GETLK             8       /* get record lock info        */
#define F_SETLK             9       /* set record lock             */
#define F_SETLKW            10      /* set record lock (blocking)  */

/* File descriptor flags */
#define FD_CLOEXEC          1       /* close-on-exec flag          */

/* =============================================================
 * openat special fd value
 * ============================================================= */
#define AT_FDCWD            -100    /* use current working dir     */

/* =============================================================
 * Error codes
 * ============================================================= */
#define ENOENT              2       /* no such file or directory   */
#define EBADF               9       /* bad file descriptor         */
#define ENAMETOOLONG        36      /* file name too long          */
#define ESPIPE              29      /* illegal seek                */
#define ENOSYS              38      /* function not implemented    */
#define EACCES              39      /* Permission denied */
#define EEXIST              40      /* File exists */
#define ENFILE              45      /* Too many open files in system */
#define EMFILE              46      /* Too many open files */
#define ENOTDIR             56      /* Not a directory */
#define EINVAL              57      /* Invalid argument */
#define ENOTTY              58      /* Inappropriate ioctl for device */

/* =============================================================
 * File table entry — kernel side of an open file descriptor
 * ============================================================= */
typedef struct file_entry {
    int         fe_flags;           /* file status flags           */
    off_t       fe_offset;          /* current file offset         */
    uint32_t    fe_ino;             /* inode number                */
    int         fe_refcount;        /* reference count (dup/fork)  */
    bool        fe_active;          /* entry is in use             */
    uint32_t    fe_dev;             /* device (for device files)   */
} file_entry_t;

/* =============================================================
 * File descriptor entry — per-process descriptor table
 * ============================================================= */
typedef struct fd_entry {
    int         fde_flags;          /* file descriptor flags       */
    file_entry_t *fde_file;         /* pointer to file table entry */
} fd_entry_t;

/* =============================================================
 * Process file descriptor table
 * ============================================================= */
typedef struct proc_fd_table {
    fd_entry_t  pft_fds[OPEN_MAX];  /* per-process descriptor table*/
    int         pft_max_fd;         /* highest fd currently open   */
} proc_fd_table_t;

/* =============================================================
 * V-node / inode structure (simplified)
 * ============================================================= */
typedef struct vnode {
    uint32_t    v_ino;              /* inode number                */
    uint16_t    v_mode;             /* file mode                   */
    uint32_t    v_size;             /* file size in bytes          */
    uint32_t    v_blocks;           /* blocks allocated            */
    time_t      v_atime;            /* last access time            */
    time_t      v_mtime;            /* last modify time            */
    time_t      v_ctime;            /* last change time            */
    uint16_t    v_uid;              /* owner user ID               */
    uint16_t    v_gid;              /* owner group ID              */
    uint32_t    v_nlink;            /* link count                  */
    uint32_t    v_rdev;             /* device number (device files)*/
    void        *v_data;            /* file system specific data   */
} vnode_t;

/* =============================================================
 * Global file I/O system state
 * ============================================================= */
extern file_entry_t  file_table[512];      /* global file table       */
extern proc_fd_table_t *current_fd_table;  /* current process fd table */

/* =============================================================
 * Core file I/O functions
 * ============================================================= */

/*
 * open — open or create a file
 * @path:  pathname of file to open
 * @oflag: O_RDONLY | O_WRONLY | O_RDWR plus optional flags
 * @mode:  permission bits (used if O_CREAT specified)
 * Returns: file descriptor if OK, -1 on error
 */
int file_open(const char *path, int oflag, mode_t mode);

/*
 * openat — open file relative to directory fd
 * @fd:    directory fd or AT_FDCWD
 * @path:  pathname (relative or absolute)
 * @oflag: same as open
 * @mode:  same as open
 */
int file_openat(int fd, const char *path, int oflag, mode_t mode);

/*
 * creat — create a new file (equivalent to open with specific flags)
 * @path: pathname of file to create
 * @mode: permission bits
 * Returns: file descriptor opened for write-only if OK, -1 on error
 */
int file_creat(const char *path, mode_t mode);

/*
 * read — read from a file
 * @fd:     file descriptor
 * @buf:    buffer to read into
 * @nbytes: number of bytes to read
 * Returns: number of bytes read, 0 if EOF, -1 on error
 */
ssize_t file_read(int fd, void *buf, size_t nbytes);

/*
 * write — write to a file  
 * @fd:     file descriptor
 * @buf:    buffer to write from
 * @nbytes: number of bytes to write
 * Returns: number of bytes written if OK, -1 on error
 */
ssize_t file_write(int fd, const void *buf, size_t nbytes);

/*
 * lseek — reposition file offset
 * @fd:     file descriptor
 * @offset: byte offset
 * @whence: SEEK_SET, SEEK_CUR, or SEEK_END
 * Returns: new file offset if OK, -1 on error
 */
off_t file_lseek(int fd, off_t offset, int whence);

/*
 * close — close file descriptor
 * @fd: file descriptor to close
 * Returns: 0 if OK, -1 on error
 */
int file_close(int fd);

/* =============================================================
 * Atomic I/O functions
 * ============================================================= */

/*
 * pread — atomic seek + read (does not update file offset)
 * @fd:     file descriptor
 * @buf:    buffer to read into
 * @nbytes: number of bytes to read
 * @offset: file offset to read from
 */
ssize_t file_pread(int fd, void *buf, size_t nbytes, off_t offset);

/*
 * pwrite — atomic seek + write (does not update file offset)
 */
ssize_t file_pwrite(int fd, const void *buf, size_t nbytes, off_t offset);

/* =============================================================
 * File descriptor manipulation
 * ============================================================= */

/*
 * dup — duplicate file descriptor (lowest available number)
 * @fd: file descriptor to duplicate
 * Returns: new file descriptor if OK, -1 on error
 */
int file_dup(int fd);

/*
 * dup2 — duplicate file descriptor to specific number
 * @fd:  source file descriptor
 * @fd2: target file descriptor number
 * Returns: fd2 if OK, -1 on error
 */
int file_dup2(int fd, int fd2);

/* =============================================================
 * fcntl — file control operations
 * ============================================================= */

/*
 * fcntl — manipulate file descriptor
 * @fd:  file descriptor
 * @cmd: command (F_DUPFD, F_GETFD, F_SETFD, etc.)
 * @arg: command-dependent argument
 * Returns: depends on cmd if OK, -1 on error
 */
int file_fcntl(int fd, int cmd, long arg);

/* =============================================================
 * Synchronization functions
 * ============================================================= */

/*
 * sync — flush all modified buffers to disk (non-blocking)
 */
void file_sync(void);

/*
 * fsync — flush specific file to disk (blocking)
 * @fd: file descriptor
 * Returns: 0 if OK, -1 on error
 */
int file_fsync(int fd);

/*
 * fdatasync — flush file data to disk (not attributes)
 * @fd: file descriptor  
 * Returns: 0 if OK, -1 on error
 */
int file_fdatasync(int fd);

/* =============================================================
 * ioctl — device control
 * ============================================================= */

/*
 * ioctl — device-specific control operations
 * @fd:      file descriptor
 * @request: device-specific command
 * @argp:    command argument (usually pointer)
 * Returns: depends on request if OK, -1 on error
 */
int file_ioctl(int fd, unsigned long request, void *argp);

/* =============================================================
 * /dev/fd support
 * ============================================================= */

/* Check if path is a /dev/fd/n reference */
bool is_devfd_path(const char *path);

/* Extract fd number from /dev/fd/n path */
int devfd_extract_fd(const char *path);

/* =============================================================
 * File I/O system initialization and utilities
 * ============================================================= */

/* Initialize file I/O subsystem */
int fileio_init(void);

/* Allocate/free file table entries */
file_entry_t *file_table_alloc(void);
void file_table_free(file_entry_t *fe);

/* Process fd table operations */
int proc_fd_alloc(proc_fd_table_t *pft, file_entry_t *fe);
void proc_fd_free(proc_fd_table_t *pft, int fd);
int proc_fd_lowest_available(proc_fd_table_t *pft);

/* File permission checking */
bool file_access_ok(vnode_t *vp, int flags, uint16_t uid, uint16_t gid);

/* Vnode operations */
vnode_t *vnode_lookup(const char *path);
int vnode_create(const char *path, mode_t mode);
ssize_t vnode_read(vnode_t *vp, void *buf, size_t count, off_t offset);
ssize_t vnode_write(vnode_t *vp, const void *buf, size_t count, off_t offset);

/* =============================================================
 * Debugging and statistics
 * ============================================================= */
typedef struct {
    uint64_t    fs_open_calls;
    uint64_t    fs_read_calls;
    uint64_t    fs_write_calls;
    uint64_t    fs_lseek_calls;
    uint64_t    fs_close_calls;
    uint64_t    fs_bytes_read;
    uint64_t    fs_bytes_written;
    uint32_t    fs_open_files;
    uint32_t    fs_max_fd_used;
} file_stats_t;

extern file_stats_t file_stats;

void fileio_print_tables(void);
void fileio_print_stats(void);

/* =============================================================
 * Helper functions for flag manipulation
 * ============================================================= */
void set_fl(int fd, int flags);     /* turn on file status flags   */
void clr_fl(int fd, int flags);     /* turn off file status flags  */

/* =============================================================
 * Error handling
 * ============================================================= */
extern int file_errno;              /* last error code             */
const char *file_strerror(int errnum);

#endif /* UIOX_FILE_IO_H */
