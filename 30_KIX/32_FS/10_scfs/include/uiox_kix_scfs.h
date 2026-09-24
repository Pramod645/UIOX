/*
 *  30_KIX/32_FS/10_scfs/include/uiox_kix_scfs.h
 *
 *  SCFS — the System Call File System layer.  FILE level.
 *  Bach, The Design of the UNIX Operating System, Ch.5.
 *
 *  ── what this layer is, and what it is NOT ────────────────────────────
 *  10_scfs is the FILE level.  01_fsa is the INODE level.  Bach draws the
 *  line in the same place:
 *
 *    FILE level    the three kernel data structures — the file table, the
 *                  user file descriptor table, the mount table — and the
 *                  system call bodies that use them (Ch.5)
 *    INODE level   the inode cache, the buffer cache, the block map, the
 *                  super block, allocation (Ch.3, Ch.4)
 *
 *  ── SYSTEM CALL NUMBERING IS NOT HERE ─────────────────────────────────
 *  This header once carried its own syscall numbers and a sysent[] of its
 *  own, in a private 1000-upward range.  That is REMOVED.
 *
 *  SCIX owns the syscall ABI:
 *
 *      40_SCIX/uix_sys.h
 *          SYS_OPEN 5 · SYS_READ 3 · SYS_WRITE 4 · SYS_CLOSE 6
 *          SYS_STAT 38 · SYS_FCNTL 92 · SYS_GETDENTS 99 … SYS_TMPFD 164
 *
 *      50_UIX/00_libs/00_uixlibs*.c
 *          sys_*()   ← the entry points
 *              └── fs_*()  ← this layer
 *
 *  Two competing numberings cannot coexist: a call would dispatch to the
 *  wrong handler with no error.  So this layer exports ALGORITHMS ONLY.
 *  Whatever SCIX needs to call is what it reaches from uix_sys.h; the
 *  adapter between the two is SCIX's file to own, not this one's.
 *
 *  What this layer provides, and nothing more:
 *    · the three Bach data structures
 *    · the system call BODIES, named uiox_kix_scfs_* (open, read, write,
 *      close, fsync-shaped entry points, plus the rest of Bach's Ch.5 set)
 *    · scfs_getf and the table helpers those bodies need
 *
 *  There is no SCFS_SYS_* number, no sysent_t, no gap table and no
 *  sys_* alias in this header or anywhere in this layer.
 *
 *  ── the three structures, in Bach's words ────────────────────────────
 *  "The kernel maintains three data structures for file I/O: the file
 *   table, the user file descriptor table, and the mount table."
 *
 *  @version 3.0.0  @date 2026-09-24
 */
#ifndef UIOX_KIX_SCFS_H
#define UIOX_KIX_SCFS_H

/* 01_fsa's own types.  SCFS redeclares NONE of them. */
#include "fs_types.h"     /* BLOCK_SIZE, MAX_*, NDIRECT, FileType, PERM_* */
#include "buffer.h"       /* BufHdr                                        */
#include "inode.h"        /* DiskInode, InCoreInode                        */
#include "namei.h"        /* DirEntry, ROOT_INO                            */
#include "superblock.h"   /* SuperBlock                                    */
#include "bmap.h"         /* BmapResult                                    */

/* ═════════════════════════════════════════════════════════════════════
 * Result codes
 *
 * Bach's kernel returns -1 and sets u.u_error; a freestanding kernel with
 * no u area returns the code directly.  These are the conventional
 * numbers, and they match what SCIX reports upward.
 *
 * NOTE: these are SCFS's internal returns.  A syscall wrapper in
 * 50_UIX/00_libs/ translates them to whatever the hosted ABI expects.
 * ═════════════════════════════════════════════════════════════════════ */
#define SCFS_OK         0
#define SCFS_EPERM      1
#define SCFS_ENOENT     2
#define SCFS_ESRCH      3
#define SCFS_EINTR      4
#define SCFS_EIO        5
#define SCFS_ENXIO      6
#define SCFS_E2BIG      7
#define SCFS_ENOEXEC    8
#define SCFS_EBADF      9
#define SCFS_ECHILD    10
#define SCFS_EAGAIN    11
#define SCFS_ENOMEM    12
#define SCFS_EACCES    13
#define SCFS_EFAULT    14
#define SCFS_ENOTBLK   15
#define SCFS_EBUSY     16
#define SCFS_EEXIST    17
#define SCFS_EXDEV     18
#define SCFS_ENODEV    19
#define SCFS_ENOTDIR   20
#define SCFS_EISDIR    21
#define SCFS_EINVAL    22
#define SCFS_ENFILE    23
#define SCFS_EMFILE    24
#define SCFS_ENOTTY    25
#define SCFS_ETXTBSY   26
#define SCFS_EFBIG     27
#define SCFS_ENOSPC    28
#define SCFS_ESPIPE    29
#define SCFS_EROFS     30
#define SCFS_EMLINK    31
#define SCFS_EPIPE     32
#define SCFS_EDOM      33
#define SCFS_ERANGE    34
#define SCFS_ENAMETOOLONG 36
#define SCFS_ENOSYS    38
#define SCFS_ENOTEMPTY 39
#define SCFS_ELOOP     40
#define SCFS_ENODATA   61
#define SCFS_EREMOTE   66
#define SCFS_EOPNOTSUPP 95

/* ═════════════════════════════════════════════════════════════════════
 * Limits
 * ═════════════════════════════════════════════════════════════════════ */
#define NFILE          256u   /* entries in the system-wide FILE TABLE    */
#define NOFILE          64u   /* entries in the per-process fd table      */
#define NMOUNT           8u   /* entries in the MOUNT TABLE               */

/* namei.h fixes these; SCFS uses the same values, not its own. */
#define SCFS_PATH_MAX  MAX_PATH_LEN   /* 256 */
#define SCFS_NAME_MAX  MAX_NAME_LEN   /* 28  */
#define SCFS_DIRENT_SZ ((uint32_t)sizeof(DirEntry))

/* ═════════════════════════════════════════════════════════════════════
 * STRUCTURE 1 — an entry in the FILE TABLE
 *
 * Bach: "one entry allocated for every opened file in the system."
 *
 * f_count is not the inode's reference count.  It counts DESCRIPTORS
 * pointing at this entry: dup() raises it, close() lowers it, the entry is
 * freed at zero.  The inode has its own count, held by the entry.
 *
 * f_offset lives HERE — not in the descriptor, not in the inode.  That is
 * why two descriptors from dup() share a position and two from separate
 * open() calls do not.
 * ═════════════════════════════════════════════════════════════════════ */
/* the status flags, in Bach's f_flag */
#define FREAD     0x01u
#define FWRITE    0x02u
#define FAPPEND   0x04u
#define FNONBLOCK 0x08u
#define FSYNC     0x10u

typedef struct scfs_file {
    InCoreInode *f_inode;    /* the inode — NULL when the slot is free   */
    uint32_t     f_offset;   /* the shared read/write position           */
    uint16_t     f_count;    /* descriptors pointing at this entry       */
    uint16_t     f_flag;     /* FREAD | FWRITE | FAPPEND | ...           */
    uint16_t     f_owner;    /* who gets SIGIO                           */
    uint8_t      f_inuse;    /* slot occupied                            */
    uint8_t      f_locked;   /* flock state — see ioctl.c                */
    uint8_t      f_pad[2];
} scfs_file_t;

/* ═════════════════════════════════════════════════════════════════════
 * STRUCTURE 2 — the USER FILE DESCRIPTOR TABLE
 *
 * Bach: "one entry allocated for every file descriptor known to a
 * process."  The slot INDEX is the descriptor number.
 *
 * Per PROCESS, which is why Bach keeps it in the u area.  One instance
 * lives here for bring-up; 33_PCS rebinds it with scfs_ufdt_bind().
 * ═════════════════════════════════════════════════════════════════════ */
typedef struct scfs_ufdt {
    scfs_file_t *ufd_file[NOFILE];
} scfs_ufdt_t;

/* ═════════════════════════════════════════════════════════════════════
 * STRUCTURE 3 — the MOUNT TABLE
 *
 * Bach: "containing information for every active file system."  The
 * mount-point inode keeps the reference namei gave it, so a path walk
 * that crosses the mount still holds the directory the mount replaced.
 * ═════════════════════════════════════════════════════════════════════ */
typedef struct scfs_mount {
    InCoreInode *m_mountpt;   /* the directory mounted ON                */
    InCoreInode *m_root;      /* root inode of the mounted filesystem    */
    BufHdr      *m_sb_buf;    /* the super block buffer, held open       */
    uint8_t      m_dev;       /* the device holding the mounted fs       */
    uint8_t      m_inuse;
    uint8_t      m_rdonly;
    char         m_fstype[8];
    uint8_t      m_pad;
} scfs_mount_t;

/* ═════════════════════════════════════════════════════════════════════
 * The algorithm bodies — Bach's Ch.5 set
 *
 * These are the functions SCIX calls.  They take plain C arguments and
 * return a result code (or a value, where Bach's algorithm returns one).
 * No syscall number, no register carrier, no argument-count check — those
 * are the caller's concern.
 * ═════════════════════════════════════════════════════════════════════ */

/* ── open.c — the entry points Bach names for a new descriptor ─────── */
int uiox_kix_scfs_open (const char *path, int flags, uint16_t perm);
int uiox_kix_scfs_creat(const char *path, uint16_t perm);
int uiox_kix_scfs_close(int fd);

/* ── read.c / write.c — the FS-relevant calls SCIX reaches most ────── */
int32_t uiox_kix_scfs_read  (int fd, char *buf, uint32_t count);
int32_t uiox_kix_scfs_write (int fd, const char *buf, uint32_t count);
int32_t uiox_kix_scfs_pread (int fd, char *buf, uint32_t count, uint32_t off);
int32_t uiox_kix_scfs_pwrite(int fd, const char *buf, uint32_t count, uint32_t off);
int32_t uiox_kix_scfs_readv (int fd, const void *iov, int iovcnt);
int32_t uiox_kix_scfs_writev(int fd, const void *iov, int iovcnt);
int     uiox_kix_scfs_lseek (int fd, int32_t offset, int whence);

/* ── status — the buffer size travels WITH the call ─────────────────
 * See uiox_kix_scfs_stat.h. */
int uiox_kix_scfs_stat  (const char *path, void *buf, uint32_t bufsz);
int uiox_kix_scfs_fstat (int fd, void *buf, uint32_t bufsz);
int uiox_kix_scfs_lstat (const char *path, void *buf, uint32_t bufsz);

/* ── attributes ─────────────────────────────────────────────────────── */
int      uiox_kix_scfs_chown (const char *path, uint16_t uid, uint16_t gid);
int      uiox_kix_scfs_fchown(int fd, uint16_t uid, uint16_t gid);
int      uiox_kix_scfs_chmod (const char *path, uint16_t perm);
int      uiox_kix_scfs_fchmod(int fd, uint16_t perm);
int      uiox_kix_scfs_setgid(const char *path, uint16_t gid);
int      uiox_kix_scfs_access(const char *path, int mode);
uint16_t uiox_kix_scfs_umask (uint16_t mask);
int      uiox_kix_scfs_utime (const char *path, const int64_t *times);
int      uiox_kix_scfs_utimes(const char *path, const int64_t *times_us);
int      uiox_kix_scfs_futimes (int fd, const int64_t *times);
int      uiox_kix_scfs_futimens(int fd, const int64_t *times_us);
int      uiox_kix_scfs_truncate (const char *path, uint32_t len);
int      uiox_kix_scfs_ftruncate(int fd, uint32_t len);

/* ── directories ────────────────────────────────────────────────────── */
int uiox_kix_scfs_mkdir (const char *path, uint16_t perm);
int uiox_kix_scfs_rmdir (const char *path);
int uiox_kix_scfs_chdir (const char *path);
int uiox_kix_scfs_fchdir(int fd);
int uiox_kix_scfs_chroot(const char *path);
int uiox_kix_scfs_getcwd(char *buf, uint32_t size);
int uiox_kix_scfs_getdents64(int fd, uint8_t *out, uint32_t cap, uint32_t *off);
int uiox_kix_scfs_link  (const char *oldpath, const char *newpath);
int uiox_kix_scfs_unlink(const char *path);
int uiox_kix_scfs_rename(const char *oldpath, const char *newpath);
int uiox_kix_scfs_mknod (const char *path, uint16_t mode,
                         uint8_t major, uint8_t minor);
int uiox_kix_scfs_mkfifo(const char *path, uint16_t perm);
int uiox_kix_scfs_symlink (const char *target, const char *linkpath);
int uiox_kix_scfs_readlink(const char *path, char *buf, uint32_t size);

/* ── descriptors ────────────────────────────────────────────────────── */
int uiox_kix_scfs_pipe(int *fds);
int uiox_kix_scfs_dup (int fd);
int uiox_kix_scfs_dup2(int oldfd, int newfd);
int uiox_kix_scfs_dup3(int oldfd, int newfd, int flags);
int uiox_kix_scfs_fcntl(int fd, int cmd, int arg);
int uiox_kix_scfs_ioctl(int fd, uint32_t cmd, void *arg);
int uiox_kix_scfs_flock(int fd, int operation);
int uiox_kix_scfs_close_range(unsigned int first, unsigned int last, int flags);

/* ── durability — what sys_sync and sys_fsync reach ───────────────── */
void uiox_kix_scfs_sync(void);
int  uiox_kix_scfs_fsync(int fd);
int  uiox_kix_scfs_fdatasync(int fd);
int  uiox_kix_scfs_syncfs(int fd);
int  uiox_kix_scfs_sync_file_range(int fd, uint32_t off, uint32_t len, int flags);

/* ── file systems ───────────────────────────────────────────────────── */
int uiox_kix_scfs_mount (const char *dev, const char *dir, int flags);
int uiox_kix_scfs_umount(const char *dir);
int uiox_kix_scfs_umount2(const char *dir, int flags);
int uiox_kix_scfs_statfs (const char *path, void *buf, uint32_t bufsz);
int uiox_kix_scfs_fstatfs(int fd, void *buf, uint32_t bufsz);
int uiox_kix_scfs_fstype (const char *path);

/* ── extended attributes — no storage in 01_fsa; see xattr.c ────────── */
int uiox_kix_scfs_setxattr   (const char *path, const char *name,
                              const void *value, uint32_t size, int flags);
int uiox_kix_scfs_getxattr   (const char *path, const char *name,
                              void *value, uint32_t size);
int uiox_kix_scfs_listxattr  (const char *path, char *list, uint32_t size);
int uiox_kix_scfs_removexattr(const char *path, const char *name);

/* ── memory-mapped I/O — file side only; no MMU paging in 33_PCS ────── */
void *uiox_kix_scfs_mmap(void *addr, uint32_t length, int prot,
                         int flags, int fd, uint32_t offset);
int   uiox_kix_scfs_munmap(void *addr, uint32_t length);
int   uiox_kix_scfs_msync(void *addr, uint32_t length, int flags);
int   uiox_kix_scfs_mprotect(void *addr, uint32_t length, int prot);
int   uiox_kix_scfs_madvise(void *addr, uint32_t length, int advice);
int   uiox_kix_scfs_mincore(void *addr, uint32_t length, uint8_t *vec);

/* ── the layer's lifecycle and shared helpers ───────────────────────── */
int      scfs_init(void);
int      scfs_is_super(void);
void     scfs_cred_set(uint16_t uid, uint16_t gid);
uint16_t scfs_uid_get(void);
uint16_t scfs_gid_get(void);
uint16_t scfs_apply_umask(uint16_t perm);
int      scfs_perm_test(InCoreInode *ip, int mode);
void     scfs_ufdt_bind(scfs_ufdt_t *t);
void     scfs_time_set(int64_t t);
int64_t  scfs_time_now(void);

/* the shared tables — read by a debug command or by SCIX's adapter */
extern scfs_file_t   scfs_file_table[NFILE];
extern scfs_ufdt_t   scfs_u_default;
extern scfs_ufdt_t  *scfs_u;
extern scfs_mount_t  scfs_mount_table[NMOUNT];

#endif /* UIOX_KIX_SCFS_H */
