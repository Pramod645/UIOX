/*
 *  30_KIX/32_FS/10_scfs/include/uiox_kix_scfs.h
 *
 *  SCFS — the System Call File System layer.
 *  Bach, The Design of the UNIX Operating System.
 *
 *  ── what this layer is ───────────────────────────────────────────────
 *  10_scfs is the FILE level.  01_fsa is the INODE level.  Bach draws the
 *  line in the same place:
 *
 *    FILE level    the three kernel data structures — the file table, the
 *                  user file descriptor table, the mount table — and the
 *                  system call bodies that use them (Ch.5)
 *    INODE level   the inode cache, the buffer cache, the block map, the
 *                  super block, allocation (Ch.3, Ch.4)
 *
 *  A system call enters here, manipulates a file table entry, and calls
 *  01_fsa for anything touching an inode.  Direct call: no bridge, no
 *  second inode type, no vtable between the two.
 *
 *  ── the three structures, in Bach's words ────────────────────────────
 *  "The kernel maintains three data structures for file I/O: the file
 *   table, the user file descriptor table, and the mount table."
 *
 *  ── CORRECTIONS versus the first cut ─────────────────────────────────
 *    · BLKSIZE  -> BLOCK_SIZE   (fs_types.h defines BLOCK_SIZE = 512)
 *    · file types use unfs_format.h's UNFS_IF* for i_mode and UNFS_DT_*
 *      for dirent d_type — the FileType nibble is gone (it collided)
 *    · SCFS_ENODATA added (removexattr needs it)
 *    · prototypes for the *at(), symlink and mmap families added
 *
 *  v1.3: FileType removed.  SCFS_S_IF* now alias UNFS_IF* and SCFS_IS_*
 *        mask with UNFS_IFMT; SCFS_DT_* added for the dirent encoding.
 *  v1.2: aligned with fs_types.h, inode.h, namei.h, superblock.h, bmap.h
 */
#ifndef UIOX_KIX_SCFS_H
#define UIOX_KIX_SCFS_H

/* 01_fsa's own types.  SCFS redeclares NONE of them. */
#include "fs_types.h"     /* brings unfs_format.h: UNFS_IF*, UNFS_DT_*  */
#include "buffer.h"       /* BufHdr                                      */
#include "inode.h"        /* DiskInode, InCoreInode                        */
#include "namei.h"        /* DirEntry, ROOT_INO                            */
#include "superblock.h"   /* SuperBlock                                    */
#include "bmap.h"         /* BmapResult                                    */

/* ═════════════════════════════════════════════════════════════════════
 * File type — the ON-DISK encoding, from unfs_format.h
 *
 * ── CHANGED: the FileType nibble is GONE ──────────────────────────────
 * This block used to spell out its own type table:
 *
 *      #define SCFS_S_IFDIR  0x2000u   // FT_DIR  2 
 *      #define SCFS_S_IFREG  0x1000u   // FT_REGULAR 1
 *      #define SCFS_IS_DIR(m)  (((m) >> 12) == (uint16_t)FT_DIR)
 *
 * and ialloc(wrote) mode = (ftype << 12) | perm.  That is two encodings
 * sharing one set of bit positions, so they COLLIDED:
 *
 *      (FT_DIR     << 12) = 0x2000 = UNFS_IFCHR
 *      (FT_REGULAR << 12) = 0x1000 = UNFS_IFIFO
 *      (FT_CHAR    << 12) = 0x3000 = not a valid type at all
 *      (FT_BLOCK   << 12) = 0x4000 = UNFS_IFDIR   ← a block device as a dir
 *
 * Every SCFS_S_IF* value above was therefore WRONG for its own name, and
 * a directory created through this path landed on disk as a character
 * device.  unfs_format.h says of its type block "These are the ON-DISK
 * values and must not change", so the format wins and these aliases now
 * point at it instead of restating it.
 *
 * ── two encodings, two jobs ───────────────────────────────────────────
 *      i_mode in the inode      UNFS_IF*   — via SCFS_S_IF* / SCFS_IS_*
 *      d_type in a dirent       UNFS_DT_*  — for getdents64's d_type
 *
 * They are NOT interchangeable: UNFS_IFDIR is 0040000 while UNFS_DT_DIR
 * is 4.  Use the right one for the field you are writing.
 * ═════════════════════════════════════════════════════════════════════ */
#define SCFS_S_IFMT   ((uint16_t)UNFS_IFMT)
#define SCFS_S_IFIFO  ((uint16_t)UNFS_IFIFO)
#define SCFS_S_IFCHR  ((uint16_t)UNFS_IFCHR)
#define SCFS_S_IFDIR  ((uint16_t)UNFS_IFDIR)
#define SCFS_S_IFBLK  ((uint16_t)UNFS_IFBLK)
#define SCFS_S_IFREG  ((uint16_t)UNFS_IFREG)
#define SCFS_S_IFLNK  ((uint16_t)UNFS_IFLNK)

/*
 * The macros below read the FORMAT bits out of i_mode.  They used to
 * compare a shifted nibble against a FileType value; they now mask with
 * UNFS_IFMT, which is what the on-disk word actually carries.
 *
 * The FileType names are still accepted as ARGUMENTS where a type must be
 * NAMED rather than tested — see SCFS_IF_OF_FT() — so a caller that has
 * not been converted yet is a visible mapping rather than a silent one.
 */
#define SCFS_IS_REG(m)  (((uint16_t)(m) & UNFS_IFMT) == UNFS_IFREG)
#define SCFS_IS_DIR(m)  (((uint16_t)(m) & UNFS_IFMT) == UNFS_IFDIR)
#define SCFS_IS_CHR(m)  (((uint16_t)(m) & UNFS_IFMT) == UNFS_IFCHR)
#define SCFS_IS_BLK(m)  (((uint16_t)(m) & UNFS_IFMT) == UNFS_IFBLK)
#define SCFS_IS_FIFO(m) (((uint16_t)(m) & UNFS_IFMT) == UNFS_IFIFO)
#define SCFS_IS_LNK(m)  (((uint16_t)(m) & UNFS_IFMT) == UNFS_IFLNK)

/* The mode bits a caller passes to creat()/mkdir() carry ONLY the
 * permission half; the type is added by ialloc().  Kept as a named zero
 * so the old call shape still reads, and so nothing re-introduces a
 * shifted type nibble. */
#define SCFS_MODE_FT(ft)   ((uint16_t)0u)

/* ═════════════════════════════════════════════════════════════════════
 * d_type values a dirent carries — the SECOND encoding
 *
 * UNFS_DT_* is not UNFS_IF*: a directory is 4 here and 0040000 in
 * i_mode.  getdents64 emits these; dir_add() stores them.
 * ═════════════════════════════════════════════════════════════════════ */
#define SCFS_DT_UNKNOWN ((uint8_t)UNFS_DT_UNKNOWN)
#define SCFS_DT_FIFO    ((uint8_t)UNFS_DT_FIFO)
#define SCFS_DT_CHR     ((uint8_t)UNFS_DT_CHR)
#define SCFS_DT_DIR     ((uint8_t)UNFS_DT_DIR)
#define SCFS_DT_BLK     ((uint8_t)UNFS_DT_BLK)
#define SCFS_DT_REG     ((uint8_t)UNFS_DT_REG)
#define SCFS_DT_LNK     ((uint8_t)UNFS_DT_LNK)

/* i_mode -> the d_type a dirent records.  One conversion, one place,
 * because the two encodings are unrelated numeric systems. */
static inline uint8_t scfs_dtype_of_mode(uint16_t mode)
{
    switch (mode & UNFS_IFMT) {
    case UNFS_IFDIR:  return (uint8_t)UNFS_DT_DIR;
    case UNFS_IFCHR:  return (uint8_t)UNFS_DT_CHR;
    case UNFS_IFBLK:  return (uint8_t)UNFS_DT_BLK;
    case UNFS_IFREG:  return (uint8_t)UNFS_DT_REG;
    case UNFS_IFIFO:  return (uint8_t)UNFS_DT_FIFO;
    case UNFS_IFLNK:  return (uint8_t)UNFS_DT_LNK;
    default:          return (uint8_t)UNFS_DT_UNKNOWN;
    }
}

/* ═════════════════════════════════════════════════════════════════════
 * Result codes
 *
 * Bach's kernel returns -1 and sets u.u_error; a freestanding kernel with
 * no u area returns the code directly.  The numbers are the conventional
 * ones so user space needs no translation table.
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
#define SCFS_EREMOTE   66
#define SCFS_ENODATA   61
#define SCFS_EOPNOTSUPP 95

/* ═════════════════════════════════════════════════════════════════════
 * Limits
 *
 * NFILE and NOFILE are Bach's names for the two table sizes.  The values
 * here are larger than his 100 / 20 because this kernel runs fewer,
 * bigger processes; the ratio is kept so the fd table stays a small
 * array indexed by a small int.
 * ═════════════════════════════════════════════════════════════════════ */
#define NFILE          256u   /* entries in the system-wide FILE TABLE    */
#define NOFILE          64u   /* entries in the per-process fd table      */
#define NMOUNT           8u   /* entries in the MOUNT TABLE               */
#define SCFS_MAX_ARGS    6u   /* argument registers the trap handler saves */

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
 * pointing at this entry: dup() raises it, close() lowers it, the entry
 * is freed at zero.  The inode has its own count, held by the entry.
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
    uint32_t     f_offset;   /* the shared read/write position          */
    uint16_t     f_count;    /* descriptors pointing at this entry      */
    uint16_t     f_flag;     /* FREAD | FWRITE | FAPPEND | ...          */
    uint16_t     f_owner;    /* who gets SIGIO                          */
    uint8_t      f_inuse;    /* slot occupied                           */
    uint8_t      f_locked;   /* flock state — see ioctl.c               */
    uint8_t      f_pad[2];
} scfs_file_t;

/* ═════════════════════════════════════════════════════════════════════
 * STRUCTURE 2 — the USER FILE DESCRIPTOR TABLE
 *
 * Bach: "one entry allocated for every file descriptor known to a
 * process."  The slot INDEX is the descriptor number a program passes in.
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
 *
 * NOTE: there is no device field on InCoreInode, so m_dev is filled from
 * the mount call's own argument and is the only device identity the
 * system has.
 * ═════════════════════════════════════════════════════════════════════ */
typedef struct scfs_mount {
    InCoreInode *m_mountpt;   /* the directory mounted ON                */
    InCoreInode *m_root;      /* root inode of the mounted filesystem    */
    BufHdr    *m_sb_buf;    /* the super block buffer, held open       */
    uint16_t     m_dev;       /* caller-supplied device id               */
    uint8_t      m_inuse;
    uint8_t      m_rdonly;
    char         m_fstype[8];
} scfs_mount_t;

/* ═════════════════════════════════════════════════════════════════════
 * open() flags
 * ═════════════════════════════════════════════════════════════════════ */
#define O_RDONLY    0x000
#define O_WRONLY    0x001
#define O_RDWR      0x002
#define O_ACCMODE   0x003
#define O_CREAT     0x040
#define O_EXCL      0x080
#define O_TRUNC     0x200
#define O_APPEND    0x400
#define O_NONBLOCK  0x800
#define O_NDELAY    O_NONBLOCK
#define O_SYNC      0x1000
#define O_DIRECTORY 0x20000
#define O_NOFOLLOW  0x40000
#define O_CLOEXEC   0x80000

/* lseek */
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

/* access() */
#define SCFS_F_OK 0
#define SCFS_R_OK 4
#define SCFS_W_OK 2
#define SCFS_X_OK 1

/* the *at() family */
#define SCFS_AT_FDCWD            (-100)
#define SCFS_AT_SYMLINK_NOFOLLOW 0x100
#define SCFS_AT_REMOVEDIR        0x200
#define SCFS_AT_SYMLINK_FOLLOW   0x400

/* fcntl() commands */
#define SCFS_F_DUPFD         0
#define SCFS_F_GETFD         1
#define SCFS_F_SETFD         2
#define SCFS_F_GETFL         3
#define SCFS_F_SETFL         4
#define SCFS_F_GETLK         5
#define SCFS_F_SETLK         6
#define SCFS_F_SETLKW        7
#define SCFS_F_SETOWN        8
#define SCFS_F_GETOWN        9
#define SCFS_F_DUPFD_CLOEXEC 1030

/* flock() */
#define SCFS_LOCK_SH  1
#define SCFS_LOCK_EX  2
#define SCFS_LOCK_NB  4
#define SCFS_LOCK_UN  8

/* mmap() */
#define SCFS_MAP_FAILED  ((void *)-1)
#define SCFS_MAP_SHARED  0x1
#define SCFS_MAP_PRIVATE 0x2
#define SCFS_MAP_FIXED   0x10
#define SCFS_PROT_READ   0x1
#define SCFS_PROT_WRITE  0x2
#define SCFS_PROT_EXEC   0x4

/* the umount2() flag Bach does not have */
#define SCFS_MNT_FORCE 0x2

#define SCFS_XATTR_MAX_VALUE 1024u

/* ═════════════════════════════════════════════════════════════════════
 * The syscall bodies
 *
 * Bach's algorithm names, with a sys_* alias alongside (the aliases
 * themselves live in uiox_kix_scfs_syscalls.c, one place).
 * ═════════════════════════════════════════════════════════════════════ */

/* open.c */
int uiox_kix_scfs_open (const char *path, int flags, uint16_t perm);

/* read.c / write.c */
int32_t uiox_kix_scfs_read  (int fd, char *buf, uint32_t count);
int32_t uiox_kix_scfs_write (int fd, const char *buf, uint32_t count);
int32_t uiox_kix_scfs_pread (int fd, char *buf, uint32_t count, uint32_t off);
int32_t uiox_kix_scfs_pwrite(int fd, const char *buf, uint32_t count, uint32_t off);
int32_t uiox_kix_scfs_readv (int fd, const void *iov, int iovcnt);
int32_t uiox_kix_scfs_writev(int fd, const void *iov, int iovcnt);

/* lseek.c / close.c */
int     uiox_kix_scfs_lseek(int fd, int32_t offset, int whence);
int     uiox_kix_scfs_close(int fd);

/* creat.c / mknod.c */
int     uiox_kix_scfs_creat (const char *path, uint16_t perm);
int     uiox_kix_scfs_mknod (const char *path, uint16_t mode,
                             uint8_t major, uint8_t minor);
int     uiox_kix_scfs_mkfifo(const char *path, uint16_t perm);

/* chdir.c / chroot.c */
int     uiox_kix_scfs_chdir (const char *path);
int     uiox_kix_scfs_fchdir(int fd);
int     uiox_kix_scfs_chroot(const char *path);
int     uiox_kix_scfs_getcwd(char *buf, uint32_t size);

/* chown.c / chmod.c / access.c */
int      uiox_kix_scfs_chown (const char *path, uint16_t uid, uint16_t gid);
int      uiox_kix_scfs_fchown(int fd, uint16_t uid, uint16_t gid);
int      uiox_kix_scfs_chmod (const char *path, uint16_t perm);
int      uiox_kix_scfs_fchmod(int fd, uint16_t perm);
int      uiox_kix_scfs_setgid(const char *path, uint16_t gid);
int      uiox_kix_scfs_access(const char *path, int mode);
uint16_t uiox_kix_scfs_umask (uint16_t mask);

/* stat.c */
int     uiox_kix_scfs_stat   (const char *path, void *buf, uint32_t bufsz);
int     uiox_kix_scfs_fstat  (int fd, void *buf, uint32_t bufsz);
int     uiox_kix_scfs_lstat  (const char *path, void *buf, uint32_t bufsz);
int     uiox_kix_scfs_fstatat(int dirfd, const char *path, void *buf, uint32_t bufsz, int flags);
int     uiox_kix_scfs_statx  (int dirfd, const char *path, int flags,
                              unsigned int mask, void *buf, uint32_t bufsz);

/* pipe.c / dup.c */
int     uiox_kix_scfs_pipe(int *fds);
int     uiox_kix_scfs_dup (int fd);
int     uiox_kix_scfs_dup2(int oldfd, int newfd);
int     uiox_kix_scfs_dup3(int oldfd, int newfd, int flags);

/* mount.c / umount.c */
int     uiox_kix_scfs_mount (const char *dev, const char *dir, int flags);
int     uiox_kix_scfs_umount(const char *dir);
int     uiox_kix_scfs_umount2(const char *dir, int flags);

/* link.c / unlink.c */
int     uiox_kix_scfs_link  (const char *oldpath, const char *newpath);
int     uiox_kix_scfs_unlink(const char *path);

/* mkdir.c / rmdir.c / rename.c */
int     uiox_kix_scfs_mkdir (const char *path, uint16_t perm);
int     uiox_kix_scfs_rmdir (const char *path);
int     uiox_kix_scfs_rename(const char *oldpath, const char *newpath);

/* getdents64.c */
int     uiox_kix_scfs_getdents64(int fd, uint8_t *out, uint32_t cap,
                                 uint32_t *off);

/* openat.c and the *at() family */
int     uiox_kix_scfs_openat    (int dirfd, const char *path, int flags, uint16_t perm);
int     uiox_kix_scfs_faccessat (int dirfd, const char *path, int mode, int flags);
int     uiox_kix_scfs_fchmodat  (int dirfd, const char *path, uint16_t mode, int flags);
int     uiox_kix_scfs_fchownat  (int dirfd, const char *path, uint16_t uid,
                                 uint16_t gid, int flags);
int     uiox_kix_scfs_unlinkat  (int dirfd, const char *path, int flags);
int     uiox_kix_scfs_mkdirat   (int dirfd, const char *path, uint16_t mode);
int     uiox_kix_scfs_linkat    (int odirfd, const char *oldp, int ndirfd,
                                 const char *newp, int flags);
int     uiox_kix_scfs_renameat  (int odirfd, const char *oldp, int ndirfd,
                                 const char *newp);
int     uiox_kix_scfs_symlinkat (const char *target, int dirfd, const char *linkpath);
int     uiox_kix_scfs_readlinkat(int dirfd, const char *path, char *buf, uint32_t sz);

/* fcntl.c */
int     uiox_kix_scfs_fcntl(int fd, int cmd, int arg);

/* truncate.c */
int     uiox_kix_scfs_truncate (const char *path, uint32_t len);
int     uiox_kix_scfs_ftruncate(int fd, uint32_t len);
int     uiox_kix_scfs_fallocate(int fd, int mode, uint32_t off, uint32_t len);

/* utime.c */
int     uiox_kix_scfs_utime   (const char *path, const int64_t *times);
int     uiox_kix_scfs_utimes  (const char *path, const int64_t *times_us);
int     uiox_kix_scfs_futimes (int fd, const int64_t *times);
int     uiox_kix_scfs_futimens(int fd, const int64_t *times_us);

/* sync.c */
void    uiox_kix_scfs_sync(void);
int     uiox_kix_scfs_fsync(int fd);
int     uiox_kix_scfs_fdatasync(int fd);
int     uiox_kix_scfs_syncfs(int fd);
int     uiox_kix_scfs_sync_file_range(int fd, uint32_t off, uint32_t len, int flags);

/* statfs.c */
int     uiox_kix_scfs_statfs (const char *path, void *buf, uint32_t bufsz);
int     uiox_kix_scfs_fstatfs(int fd, void *buf, uint32_t bufsz);
int     uiox_kix_scfs_fstype (const char *path);
int     uiox_kix_scfs_quotactl(int cmd, const char *dev, int id, void *addr);

/* xattr.c */
int     uiox_kix_scfs_setxattr   (const char *path, const char *name,
                                  const void *value, uint32_t size, int flags);
int     uiox_kix_scfs_getxattr   (const char *path, const char *name,
                                  void *value, uint32_t size);
int     uiox_kix_scfs_listxattr  (const char *path, char *list, uint32_t size);
int     uiox_kix_scfs_removexattr(const char *path, const char *name);
int     uiox_kix_scfs_fsetxattr  (int fd, const char *name,
                                  const void *value, uint32_t size, int flags);
int     uiox_kix_scfs_fgetxattr  (int fd, const char *name,
                                  void *value, uint32_t size);

/* ioctl.c */
int     uiox_kix_scfs_ioctl(int fd, uint32_t cmd, void *arg);
int     uiox_kix_scfs_flock(int fd, int operation);
int     uiox_kix_scfs_close_range(unsigned int first, unsigned int last, int flags);

/* mmap.c */
void   *uiox_kix_scfs_mmap(void *addr, uint32_t length, int prot,
                           int flags, int fd, uint32_t offset);
int     uiox_kix_scfs_munmap(void *addr, uint32_t length);
int     uiox_kix_scfs_msync(void *addr, uint32_t length, int flags);
int     uiox_kix_scfs_mprotect(void *addr, uint32_t length, int prot);
int     uiox_kix_scfs_madvise(void *addr, uint32_t length, int advice);
int     uiox_kix_scfs_mincore(void *addr, uint32_t length, uint8_t *vec);

/* symlink — defined in the 10_unfs integration unit: FT_SYMLINK exists in
 * fs_types.h, so the type is real; the target storage is 10_unfs's. */
int     uiox_kix_scfs_symlink (const char *target, const char *linkpath);
int     uiox_kix_scfs_readlink(const char *path, char *buf, uint32_t size);

/* ── the layer's lifecycle and shared helpers ───────────────────────── */
int      scfs_init(void);
int      scfs_syscall_init(void);
int      scfs_is_super(void);
void     scfs_cred_set(uint16_t uid, uint16_t gid);
uint16_t scfs_uid_get(void);
uint16_t scfs_gid_get(void);
uint16_t scfs_apply_umask(uint16_t perm);
int      scfs_perm_test(InCoreInode *ip, int mode);
void     scfs_ufdt_bind(uint16_t unused_);
void     scfs_time_set(int64_t t);
int64_t  scfs_time_now(void);

/* the shared tables, exported so the arch stubs and a debug command can
 * read them */
extern scfs_file_t   scfs_file_table[NFILE];
extern scfs_ufdt_t   scfs_u_default;
extern scfs_ufdt_t  *scfs_u;
extern scfs_mount_t  scfs_mount_table[NMOUNT];

#endif /* UIOX_KIX_SCFS_H */
