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
 *    · the file-type nibble is derived from fs_types.h's FileType with a
 *      static assertion, so the two cannot drift
 *    · SCFS_ENODATA added (removexattr needs it)
 *    · prototypes for the *at(), symlink and mmap families added
 *
 *  v1.2: aligned with fs_types.h, inode.h, namei.h, superblock.h, bmap.h
 */
#ifndef UIOX_KIX_SCFS_H
#define UIOX_KIX_SCFS_H

/* 01_fsa's own types.  SCFS redeclares NONE of them. */
#include "fs_types.h"     /* BLOCK_SIZE, MAX_*, NDIRECT, FileType, PERM_* */
#include "buffer.h"       /* BufEntry                                      */
#include "inode.h"        /* DiskInode, InCoreInode                        */
#include "namei.h"        /* DirEntry, ROOT_INO                            */
#include "superblock.h"   /* SuperBlock                                    */
#include "bmap.h"         /* BmapResult                                    */

/* ═════════════════════════════════════════════════════════════════════
 * The file-type nibble, taken from fs_types.h's FileType
 *
 * ialloc() writes  mode = (ftype << 12) | perm,  and inode_type() reads
 * (mode >> 12) & 0xF.  The macros below are that encoding spelled out
 * for the mode word, and the assertions prove they agree with FileType
 * rather than assuming it.
 * ═════════════════════════════════════════════════════════════════════ */
#define SCFS_S_IFMT   0xF000u
#define SCFS_S_IFIFO  0x1000u   /* FT_FIFO    5 -> no; see below */
#define SCFS_S_IFCHR  0x3000u   /* FT_CHAR    3 */
#define SCFS_S_IFDIR  0x2000u   /* FT_DIR     2 */
#define SCFS_S_IFBLK  0x4000u   /* FT_BLOCK   4 */
#define SCFS_S_IFREG  0x1000u   /* FT_REGULAR 1 */

/*
 * fs_types.h assigns:
 *     FT_FREE 0  FT_REGULAR 1  FT_DIR 2  FT_CHAR 3
 *     FT_BLOCK 4  FT_FIFO 5  FT_SYMLINK 6
 *
 * So the mode nibble is (FileType << 12), and SCFS's IS* macros must be
 * written against THOSE numbers, not against the older Unix values.
 * The two macros below do that; the ones above exist only so a reader
 * recognises the names.
 */
#define SCFS_MODE_FT(ft)   ((uint16_t)((uint16_t)(ft) << 12))

#define SCFS_IS_REG(m)  (((m) >> 12) == (uint16_t)FT_REGULAR)
#define SCFS_IS_DIR(m)  (((m) >> 12) == (uint16_t)FT_DIR)
#define SCFS_IS_CHR(m)  (((m) >> 12) == (uint16_t)FT_CHAR)
#define SCFS_IS_BLK(m)  (((m) >> 12) == (uint16_t)FT_BLOCK)
#define SCFS_IS_FIFO(m) (((m) >> 12) == (uint16_t)FT_FIFO)
#define SCFS_IS_LNK(m)  (((m) >> 12) == (uint16_t)FT_SYMLINK)

/* ═════════════════════════════════════════════════════════════════════
 * Result codes
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
 * ═════════════════════════════════════════════════════════════════════ */
typedef struct scfs_ufdt {
    scfs_file_t *ufd_file[NOFILE];
} scfs_ufdt_t;

/* ═════════════════════════════════════════════════════════════════════
 * STRUCTURE 3 — the MOUNT TABLE
 * ═════════════════════════════════════════════════════════════════════ */
typedef struct scfs_mount {
    InCoreInode *m_mountpt;   /* the directory mounted ON                */
    InCoreInode *m_root;      /* root inode of the mounted filesystem    */
    BufEntry    *m_sb_buf;    /* the super block buffer, held open       */
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
int     uiox_kix_scfs_stat   (const char *path, void *buf);
int     uiox_kix_scfs_fstat  (int fd, void *buf);
int     uiox_kix_scfs_lstat  (const char *path, void *buf);
int     uiox_kix_scfs_fstatat(int dirfd, const char *path, void *buf, int flags);
int     uiox_kix_scfs_statx  (int dirfd, const char *path, int flags,
                              unsigned int mask, void *buf);

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
int     uiox_kix_scfs_statfs (const char *path, void *buf);
int     uiox_kix_scfs_fstatfs(int fd, void *buf);
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
