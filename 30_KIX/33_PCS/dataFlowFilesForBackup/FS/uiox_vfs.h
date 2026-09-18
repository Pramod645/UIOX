/*
 * 30_KIX/32_FS/include/uiox_vfs.h
 *
 * UIOX Virtual File System — core types and operations.
 *
 * Layer model:
 *   userspace
 *      ↓  SVC/ECALL/SYSCALL
 *   33_PCS/uiox_syscall.c   — sys_open/read/write/close/mmap
 *      ↓  calls
 *   32_FS/01_fsa/vfs.c      — vfs_open/read/write/close  (THIS LAYER)
 *      ↓  calls via ops vtable
 *   32_FS/10_scfs/scfs.c    — concrete filesystem
 *      ↓  calls
 *   31_BufferCache / 30_DeviceDrivers — block I/O
 *
 * @version 1.0.0  @date 2026-07-29
 */

 #ifndef UIOX_VFS_H
 #define UIOX_VFS_H
 
 #include "uiox_base_types.h"   /* uint8_t, uint32_t, uint64_t, size_t */
 
 /* ── Limits ────────────────────────────────────────────────────────── */
 #define UIOX_NAME_MAX       255u
 #define UIOX_PATH_MAX       4096u
 #define UIOX_FD_MAX         64u       /* max open files per process    */
 #define UIOX_INODE_MAX      1024u     /* inode table size              */
 #define UIOX_DENTRY_MAX     256u      /* dentry cache size             */
 #define UIOX_PAGE_CACHE_MAX 128u      /* cached pages per inode        */
 
 /* ── Error codes ───────────────────────────────────────────────────── */
 #define UIOX_FS_OK          0
 #define UIOX_FS_ENOENT     -2
 #define UIOX_FS_EBADF      -9
 #define UIOX_FS_ENOMEM    -12
 #define UIOX_FS_EACCES    -13
 #define UIOX_FS_EFAULT    -14
 #define UIOX_FS_EEXIST    -17
 #define UIOX_FS_ENOTDIR   -20
 #define UIOX_FS_EISDIR    -21
 #define UIOX_FS_EINVAL    -22
 #define UIOX_FS_ENOSPC    -28
 #define UIOX_FS_ENOSYS    -38
 
 /* ── File types ────────────────────────────────────────────────────── */
 #define UIOX_DT_REG       0x01u   /* regular file     */
 #define UIOX_DT_DIR       0x02u   /* directory        */
 #define UIOX_DT_BLK       0x04u   /* block device     */
 #define UIOX_DT_CHR       0x08u   /* char device      */
 #define UIOX_DT_LNK       0x10u   /* symbolic link    */
 #define UIOX_DT_FIFO      0x20u   /* named pipe       */
 
 /* ── Open flags ────────────────────────────────────────────────────── */
 #define UIOX_O_RDONLY     0x0000u
 #define UIOX_O_WRONLY     0x0001u
 #define UIOX_O_RDWR       0x0002u
 #define UIOX_O_CREAT      0x0040u
 #define UIOX_O_TRUNC      0x0200u
 #define UIOX_O_APPEND     0x0400u
 #define UIOX_O_NONBLOCK   0x0800u
 
 /* ── Seek origins ──────────────────────────────────────────────────── */
 #define UIOX_SEEK_SET     0
 #define UIOX_SEEK_CUR     1
 #define UIOX_SEEK_END     2
 
 /* ── Forward declarations ──────────────────────────────────────────── */
 typedef struct uiox_inode    uiox_inode_t;
 typedef struct uiox_dentry   uiox_dentry_t;
 typedef struct uiox_file     uiox_file_t;
 typedef struct uiox_fs_ops   uiox_fs_ops_t;
 typedef struct uiox_inode_ops uiox_inode_ops_t;
 typedef struct uiox_file_ops uiox_file_ops_t;
 typedef struct uiox_superblock uiox_superblock_t;
 typedef struct uiox_dirent   uiox_dirent_t;
 
 /* ── stat structure ────────────────────────────────────────────────── */
 typedef struct {
     uint32_t st_ino;        /* inode number                          */
     uint32_t st_mode;       /* file type + permissions               */
     uint32_t st_nlink;      /* hard link count                       */
     uint32_t st_uid;        /* owner user ID                         */
     uint32_t st_gid;        /* owner group ID                        */
     uint64_t st_size;       /* file size in bytes                    */
     uint64_t st_blksize;    /* preferred block size                  */
     uint64_t st_blocks;     /* number of 512-byte blocks allocated   */
     uint64_t st_atime;      /* last access time (seconds since epoch)*/
     uint64_t st_mtime;      /* last modification time                */
     uint64_t st_ctime;      /* last status change time               */
 } uiox_stat_t;
 
 /* ── Directory entry ───────────────────────────────────────────────── */
 struct uiox_dirent {
     uint32_t  d_ino;
     uint8_t   d_type;
     char      d_name[UIOX_NAME_MAX + 1u];
 };
 
 /* ── File operations vtable ────────────────────────────────────────── */
 struct uiox_file_ops {
     /*
      * read — copy file data into kernel buffer kbuf.
      * Caller (sys_read) then uses copy_to_user to cross to userspace.
      * Returns bytes read, negative on error.
      */
     ssize_t (*read) (uiox_file_t *file,
                      void        *kbuf,
                      size_t       count,
                      uint64_t    *pos);
     /*
      * write — copy from kernel buffer kbuf into file.
      * Caller (sys_write) used copy_from_user first.
      * Returns bytes written, negative on error.
      */
     ssize_t (*write)(uiox_file_t       *file,
                      const void        *kbuf,
                      size_t             count,
                      uint64_t          *pos);
 
     int     (*open) (uiox_inode_t *inode, uiox_file_t *file);
     int     (*close)(uiox_inode_t *inode, uiox_file_t *file);
     int     (*seek) (uiox_file_t *file, int64_t offset, int whence);
     int     (*ioctl)(uiox_file_t *file,
                      unsigned long cmd, unsigned long arg);
     /*
      * mmap_page — return the physical address of the page
      * at byte offset 'off' in the file.
      * 33_PCS/uiox_mmap.c uses this to insert zero-copy PTEs.
      */
     uintptr_t (*mmap_page)(uiox_file_t *file, uint64_t off);
 
     int     (*readdir)(uiox_file_t  *file,
                        uiox_dirent_t *de,
                        uint32_t      idx);
     int     (*fsync)  (uiox_file_t *file);
 };
 
 /* ── Inode operations vtable ───────────────────────────────────────── */
 struct uiox_inode_ops {
     int  (*lookup)(uiox_inode_t  *dir,
                    const char    *name,
                    uiox_inode_t **out);
     int  (*create)(uiox_inode_t  *dir,
                    const char    *name,
                    uint32_t       mode,
                    uiox_inode_t **out);
     int  (*mkdir) (uiox_inode_t  *dir,
                    const char    *name,
                    uint32_t       mode);
     int  (*unlink)(uiox_inode_t  *dir,  const char *name);
     int  (*rmdir) (uiox_inode_t  *dir,  const char *name);
     int  (*rename)(uiox_inode_t  *old_dir, const char *old_name,
                    uiox_inode_t  *new_dir, const char *new_name);
     int  (*stat)  (uiox_inode_t  *inode, uiox_stat_t *out);
     int  (*truncate)(uiox_inode_t *inode, uint64_t size);
 };
 
 /* ── Superblock operations vtable ──────────────────────────────────── */
 struct uiox_fs_ops {
     const char *name;                    /* filesystem type name        */
     int  (*mount)  (uiox_superblock_t *sb, uint32_t dev_id);
     int  (*unmount)(uiox_superblock_t *sb);
     int  (*sync)   (uiox_superblock_t *sb);
     int  (*statfs) (uiox_superblock_t *sb, void *out);
 };
 
 /* ── Inode ─────────────────────────────────────────────────────────── */
 struct uiox_inode {
     uint32_t              i_ino;        /* inode number                */
     uint32_t              i_mode;       /* type + permissions          */
     uint32_t              i_nlink;      /* hard link count             */
     uint32_t              i_uid;
     uint32_t              i_gid;
     uint64_t              i_size;       /* file size in bytes          */
     uint64_t              i_atime;
     uint64_t              i_mtime;
     uint64_t              i_ctime;
     uint32_t              i_blksize;
     uint32_t              i_blocks;
     uint8_t               i_inuse;      /* reference count             */
     uiox_superblock_t    *i_sb;         /* owning superblock           */
     const uiox_inode_ops_t *i_ops;      /* inode operations            */
     const uiox_file_ops_t  *i_fops;     /* file operations             */
     void                 *i_private;    /* fs-specific data            */
 };
 
 /* ── Dentry (directory entry cache) ────────────────────────────────── */
 struct uiox_dentry {
     char            d_name[UIOX_NAME_MAX + 1u];
     uiox_inode_t   *d_inode;
     uiox_dentry_t  *d_parent;
     uint8_t         d_inuse;
 };
 
 /* ── Open file ─────────────────────────────────────────────────────── */
 struct uiox_file {
     uiox_inode_t          *f_inode;
     uint64_t               f_pos;       /* current read/write position */
     uint32_t               f_flags;     /* O_RDONLY / O_WRONLY etc.    */
     uint32_t               f_mode;
     uint8_t                f_inuse;
     const uiox_file_ops_t *f_ops;
 };
 
 /* ── Superblock ────────────────────────────────────────────────────── */
 struct uiox_superblock {
     uint32_t               s_dev;       /* block device id             */
     uint32_t               s_blocksize;
     uint64_t               s_maxbytes;
     uiox_inode_t          *s_root;      /* root inode                  */
     const uiox_fs_ops_t   *s_fsops;
     void                  *s_private;   /* fs-specific superblock data */
     uint8_t                s_dirty;
 };
 
 /* ── VFS public API — implemented in 01_fsa/vfs.c ─────────────────── */
 void  vfs_init(void);
 int   vfs_mount_root(void);
 int   vfs_mount(const char *path,
                 const char *fstype,
                 uint32_t    dev_id);
 int   vfs_umount(const char *path);
 
 /* Core file operations — these are what sys_open/read/write call */
 int     vfs_open (const char *path, uint32_t flags,
                   uint32_t mode, uiox_file_t **out);
 int     vfs_close(uiox_file_t *file);
 ssize_t vfs_read (uiox_file_t *file, void *kbuf,
                   size_t count);
 ssize_t vfs_write(uiox_file_t *file, const void *kbuf,
                   size_t count);
 int     vfs_seek (uiox_file_t *file,
                   int64_t offset, int whence);
 int     vfs_stat (const char *path, uiox_stat_t *out);
 int     vfs_mkdir(const char *path, uint32_t mode);
 int     vfs_unlink(const char *path);
 int     vfs_readdir(uiox_file_t *dir,
                     uiox_dirent_t *de, uint32_t idx);
 int     vfs_fsync(uiox_file_t *file);
 uintptr_t vfs_mmap_page(uiox_file_t *file, uint64_t off);
 
 /* Filesystem type registration */
 int  vfs_register_fs  (const uiox_fs_ops_t *ops);
 int  vfs_unregister_fs(const char *name);
 
 #endif /* UIOX_VFS_H */
 