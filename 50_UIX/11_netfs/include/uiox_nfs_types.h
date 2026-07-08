/**
 * @file  uiox_nfs_types.h
 * @brief UIOX Network Filesystem — base types, error codes, wire structs.
 *
 * Covers NFS v3 (RFC 1813), 9P2000 (Plan 9), and VirtIO-FS (FUSE-over-virtio).
 * Zero libc dependency.
 *
 * @version 1.0.0
 * @date    2026-07-07
 */

 #ifndef UIOX_NFS_TYPES_H
 #define UIOX_NFS_TYPES_H
 
 #include <stdint.h>
 #include <stdbool.h>
 #include <stddef.h>
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * Error codes
  * ====================================================================== */
 
 typedef enum {
     UIOX_NFS_OK           =  0,
     UIOX_NFS_ERR_PERM     = -1,   /**< EPERM  — not permitted           */
     UIOX_NFS_ERR_NOENT    = -2,   /**< ENOENT — no such file/directory  */
     UIOX_NFS_ERR_IO       = -3,   /**< EIO    — I/O error               */
     UIOX_NFS_ERR_ACCES    = -4,   /**< EACCES — permission denied       */
     UIOX_NFS_ERR_EXIST    = -5,   /**< EEXIST — file exists             */
     UIOX_NFS_ERR_NOTDIR   = -6,   /**< ENOTDIR                          */
     UIOX_NFS_ERR_ISDIR    = -7,   /**< EISDIR                           */
     UIOX_NFS_ERR_INVAL    = -8,   /**< EINVAL                           */
     UIOX_NFS_ERR_NOSPC    = -9,   /**< ENOSPC — no space                */
     UIOX_NFS_ERR_NAMETOOLONG=-10,
     UIOX_NFS_ERR_NOTEMPTY = -11,
     UIOX_NFS_ERR_STALE    = -12,  /**< ESTALE — stale NFS file handle   */
     UIOX_NFS_ERR_TIMEOUT  = -13,
     UIOX_NFS_ERR_NOMEM    = -14,
     UIOX_NFS_ERR_NODEV    = -15,
     UIOX_NFS_ERR_BUSY     = -16,
     UIOX_NFS_ERR_PROTO    = -17,  /**< Protocol format error            */
     UIOX_NFS_ERR_UNSUP    = -18,
 } uiox_nfs_err_t;
 
 /* =========================================================================
  * NFS v3 file handle (opaque, up to 64 bytes)
  * ====================================================================== */
 
 #define UIOX_NFS_FH_MAX_LEN     64u
 #define UIOX_NFS_NAME_MAX       255u
 #define UIOX_NFS_PATH_MAX       1024u
 #define UIOX_NFS_READDIR_BUF    4096u
 
 typedef struct {
     uint8_t  data[UIOX_NFS_FH_MAX_LEN];
     uint32_t len;
 } uiox_nfs_fh_t;
 
 /* =========================================================================
  * File types
  * ====================================================================== */
 
 typedef enum {
     UIOX_NFS_FTYPE_REG  = 1,
     UIOX_NFS_FTYPE_DIR  = 2,
     UIOX_NFS_FTYPE_LNK  = 5,
     UIOX_NFS_FTYPE_BLK  = 6,
     UIOX_NFS_FTYPE_CHR  = 7,
     UIOX_NFS_FTYPE_FIFO = 3,
     UIOX_NFS_FTYPE_SOCK = 4,
 } uiox_nfs_ftype_t;
 
 /* =========================================================================
  * File attributes (fattr3 in NFS v3)
  * ====================================================================== */
 
 typedef struct {
     uiox_nfs_ftype_t ftype;
     uint32_t         mode;       /**< Unix permission bits             */
     uint32_t         nlink;
     uint32_t         uid;
     uint32_t         gid;
     uint64_t         size;       /**< File size in bytes               */
     uint64_t         used;       /**< Bytes allocated on disk          */
     uint32_t         rdev_major;
     uint32_t         rdev_minor;
     uint64_t         fsid;
     uint64_t         fileid;     /**< inode number                     */
     uint64_t         atime_sec;  /**< Access time                      */
     uint64_t         mtime_sec;  /**< Modification time                */
     uint64_t         ctime_sec;  /**< Change time                      */
     bool             valid;
 } uiox_nfs_attr_t;
 
 /* =========================================================================
  * Directory entry
  * ====================================================================== */
 
 typedef struct {
     uint64_t  fileid;
     char      name[UIOX_NFS_NAME_MAX + 1u];
     uint64_t  cookie;   /**< Opaque readdir cookie for pagination     */
 } uiox_nfs_dirent_t;
 
 /* =========================================================================
  * Filesystem statistics (statfs3)
  * ====================================================================== */
 
 typedef struct {
     uint64_t  tbytes;   /**< Total bytes on filesystem                */
     uint64_t  fbytes;   /**< Free bytes                               */
     uint64_t  abytes;   /**< Available to non-root                    */
     uint64_t  tfiles;   /**< Total file slots                         */
     uint64_t  ffiles;   /**< Free file slots                          */
     uint32_t  invarsec; /**< Seconds until ESTALE                     */
 } uiox_nfs_statfs_t;
 
 /* =========================================================================
  * Open flags (mirrors POSIX)
  * ====================================================================== */
 
 #define UIOX_NFS_O_RDONLY   0x0000u
 #define UIOX_NFS_O_WRONLY   0x0001u
 #define UIOX_NFS_O_RDWR     0x0002u
 #define UIOX_NFS_O_CREAT    0x0040u
 #define UIOX_NFS_O_TRUNC    0x0200u
 #define UIOX_NFS_O_APPEND   0x0400u
 #define UIOX_NFS_O_EXCL     0x0080u
 
 /* =========================================================================
  * Network filesystem client type
  * ====================================================================== */
 
 typedef enum {
     UIOX_NETFS_NFS3    = 0,   /**< NFS version 3 (RFC 1813, UDP/TCP)  */
     UIOX_NETFS_9P2000  = 1,   /**< Plan 9 / 9P2000 protocol           */
     UIOX_NETFS_VIRTFS  = 2,   /**< VirtIO-FS (QEMU native)            */
 } uiox_netfs_type_t;
 
 /* =========================================================================
  * Utility macros
  * ====================================================================== */
 
 #define UIOX_NFS_UNUSED(x)       ((void)(x))
 #define UIOX_NFS_ARRAY_SIZE(a)   (sizeof(a)/sizeof((a)[0]))
 #define UIOX_NFS_MIN(a,b)        ((a)<(b)?(a):(b))
 #define UIOX_NFS_MAX(a,b)        ((a)>(b)?(a):(b))
 
 static inline const char *uiox_nfs_err_str(uiox_nfs_err_t e) {
     switch (e) {
     case UIOX_NFS_OK:            return "OK";
     case UIOX_NFS_ERR_PERM:      return "EPERM";
     case UIOX_NFS_ERR_NOENT:     return "ENOENT";
     case UIOX_NFS_ERR_IO:        return "EIO";
     case UIOX_NFS_ERR_ACCES:     return "EACCES";
     case UIOX_NFS_ERR_STALE:     return "ESTALE";
     case UIOX_NFS_ERR_TIMEOUT:   return "ETIMEOUT";
     case UIOX_NFS_ERR_PROTO:     return "EPROTO";
     case UIOX_NFS_ERR_NODEV:     return "ENODEV";
     default:                      return "EUNKNOWN";
     }
 }
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_NFS_TYPES_H */
 