/**
 * @file  uiox_nfs_vfs.h
 * @brief UIOX NFS — VFS mount layer (plugs into 32_FileSystem namei).
 * @version 1.0.0
 */

 #ifndef UIOX_NFS_VFS_H
 #define UIOX_NFS_VFS_H
 
 #include "uiox_nfs_proto.h"
 #include "uiox_nfs_9p.h"
 #include "uiox_nfs_virtfs.h"
 #include "uiox_nfs_cache.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 #define UIOX_NFS_MAX_MOUNTS  8u
 #define UIOX_NFS_MAX_OPEN    64u
 
 /* =========================================================================
  * Open file descriptor (per open() call on a remote file)
  * ====================================================================== */
 
 typedef struct {
     uiox_nfs_fh_t   fh;        /**< NFS file handle (or nodeid for 9P) */
     uint64_t        nodeid;    /**< VirtIO-FS / 9P nodeid              */
     uint64_t        virt_fh;   /**< VirtIO-FS open file handle         */
     uint32_t        p9_fid;    /**< 9P FID                             */
     uint64_t        offset;    /**< Current file position              */
     uint32_t        flags;     /**< Open flags                         */
     uint8_t         mount_idx; /**< Index into s_mounts[]              */
     bool            in_use;
 } uiox_nfs_fd_t;
 
 /* =========================================================================
  * Mount point descriptor
  * ====================================================================== */
 
 typedef struct {
     char              mount_point[UIOX_NFS_PATH_MAX]; /**< e.g. "/nfs" */
     uiox_netfs_type_t type;
     /* Protocol contexts (one active based on type) */
     uiox_nfs3_ctx_t   nfs3;
     uiox_9p_ctx_t     p9;
     uiox_virtfs_ctx_t virtfs;
     /* Cache */
     uiox_nfs_cache_t  cache;
     /* Open file table */
     uiox_nfs_fd_t     fds[UIOX_NFS_MAX_OPEN];
     /* Flags */
     bool              read_only;
     bool              mounted;
     /* Stats */
     uint64_t          bytes_read;
     uint64_t          bytes_written;
     uint32_t          open_files;
 } uiox_nfs_mount_t;
 
 /* =========================================================================
  * Mount parameters
  * ====================================================================== */
 
 typedef struct {
     uiox_netfs_type_t type;
     char              mount_point[UIOX_NFS_PATH_MAX];
     /* NFS v3 */
     uint8_t           server_ip[4];
     char              export_path[UIOX_NFS_PATH_MAX];
     /* VirtIO-FS */
     uintptr_t         virtio_mmio_base;
     /* 9P */
     char              p9_aname[128];
     /* Network send/recv for NFS/9P */
     uiox_nfs_err_t  (*net_send)(void*, const uint8_t*, uint32_t);
     uiox_nfs_err_t  (*net_recv)(void*, uint8_t*, uint32_t,
                                   uint32_t*, uint32_t);
     void             *net_ctx;
     /* Options */
     bool              read_only;
     uint32_t          timeout_ms;
 } uiox_nfs_mount_params_t;
 
 /* =========================================================================
  * VFS API — called by 32_FileSystem namei hooks
  * ====================================================================== */
 
 uiox_nfs_err_t uiox_nfs_vfs_init    (void);
 void           uiox_nfs_vfs_deinit  (void);
 
 /** Mount a remote filesystem at @params->mount_point. */
 uiox_nfs_err_t uiox_nfs_vfs_mount   (const uiox_nfs_mount_params_t *params,
                                        uint8_t *mount_idx);
 
 /** Unmount. */
 uiox_nfs_err_t uiox_nfs_vfs_umount  (const char *mount_point);
 
 /** Resolve @path (may cross mount point). Returns mount + relative path. */
 uiox_nfs_mount_t *uiox_nfs_vfs_resolve(const char *path,
                                          const char **rel_path);
 
 /* ── File operations (called by VFS layer) ──────────────────────── */
 
 int  uiox_nfs_vfs_open    (const char *path, uint32_t flags, uint32_t mode);
 int  uiox_nfs_vfs_close   (int fd);
 int  uiox_nfs_vfs_read    (int fd, uint8_t *buf, uint32_t count);
 int  uiox_nfs_vfs_write   (int fd, const uint8_t *buf, uint32_t count);
 int  uiox_nfs_vfs_lseek   (int fd, int64_t offset, int whence);
 int  uiox_nfs_vfs_stat    (const char *path, uiox_nfs_attr_t *attr);
 int  uiox_nfs_vfs_fstat   (int fd, uiox_nfs_attr_t *attr);
 int  uiox_nfs_vfs_mkdir   (const char *path, uint32_t mode);
 int  uiox_nfs_vfs_unlink  (const char *path);
 int  uiox_nfs_vfs_rename  (const char *from, const char *to);
 int  uiox_nfs_vfs_readdir (const char *path,
                              uiox_nfs3_readdir_cb_t cb, void *priv);
 int  uiox_nfs_vfs_statfs  (const char *path, uiox_nfs_statfs_t *stat);
 int  uiox_nfs_vfs_fsync   (int fd);
 
 void uiox_nfs_vfs_print   (void);
 
 /* =========================================================================
  * Syscall numbers (extension to 40_SystemCallInterface/uix_sys.h)
  * ====================================================================== */
 
 #define SYS_MOUNT      21u
 #define SYS_UMOUNT     52u
 #define SYS_STATFS     99u
 
 /* Syscall handlers */
 long sys_mount  (long src, long tgt, long fstype, long flags);
 long sys_umount (long tgt, long flags, long a2, long a3);
 long sys_statfs (long path, long buf,  long a2,   long a3);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_NFS_VFS_H */
 