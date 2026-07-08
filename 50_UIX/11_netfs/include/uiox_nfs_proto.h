/**
 * @file  uiox_nfs_proto.h
 * @brief UIOX NFS v3 client protocol procedures.
 * @version 1.0.0
 */

 #ifndef UIOX_NFS_PROTO_H
 #define UIOX_NFS_PROTO_H
 
 #include "uiox_nfs_rpc.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * NFS v3 client context
  * ====================================================================== */
 
 typedef struct {
     uiox_rpc_ctx_t  rpc;
     uiox_rpc_ctx_t  mount_rpc;     /**< Separate connection for MOUNT  */
     uiox_nfs_fh_t   root_fh;       /**< Root file handle from MOUNT    */
     char            export_path[UIOX_NFS_PATH_MAX];
     bool            mounted;
     /* Write mode */
     bool            unstable_write; /**< Use UNSTABLE writes (faster)  */
     /* Stats */
     uint64_t        bytes_read;
     uint64_t        bytes_written;
     uint32_t        getattr_calls;
     uint32_t        lookup_calls;
     uint32_t        read_calls;
     uint32_t        write_calls;
 } uiox_nfs3_ctx_t;
 
 /* =========================================================================
  * NFS v3 Procedures (RFC 1813)
  * ====================================================================== */
 
 /** Mount: call MOUNTPROC3_MNT, receive root file handle. */
 uiox_nfs_err_t uiox_nfs3_mount      (uiox_nfs3_ctx_t *ctx,
                                        const uint8_t server_ip[4],
                                        const char *export_path);
 
 /** Unmount: call MOUNTPROC3_UMNT. */
 uiox_nfs_err_t uiox_nfs3_umount     (uiox_nfs3_ctx_t *ctx);
 
 /** NFSPROC3_GETATTR: get file attributes for @fh. */
 uiox_nfs_err_t uiox_nfs3_getattr    (uiox_nfs3_ctx_t *ctx,
                                        const uiox_nfs_fh_t *fh,
                                        uiox_nfs_attr_t *attr);
 
 /** NFSPROC3_LOOKUP: look up @name in @dir_fh, return file FH + attrs. */
 uiox_nfs_err_t uiox_nfs3_lookup     (uiox_nfs3_ctx_t *ctx,
                                        const uiox_nfs_fh_t *dir_fh,
                                        const char *name,
                                        uiox_nfs_fh_t *out_fh,
                                        uiox_nfs_attr_t *out_attr);
 
 /** NFSPROC3_READ: read @count bytes at @offset from @fh into @buf. */
 uiox_nfs_err_t uiox_nfs3_read       (uiox_nfs3_ctx_t *ctx,
                                        const uiox_nfs_fh_t *fh,
                                        uint64_t offset,
                                        uint8_t *buf, uint32_t count,
                                        uint32_t *bytes_read,
                                        bool *eof);
 
 /** NFSPROC3_WRITE: write @count bytes at @offset to @fh from @buf. */
 uiox_nfs_err_t uiox_nfs3_write      (uiox_nfs3_ctx_t *ctx,
                                        const uiox_nfs_fh_t *fh,
                                        uint64_t offset,
                                        const uint8_t *buf, uint32_t count,
                                        uint32_t *bytes_written);
 
 /** NFSPROC3_CREATE: create a file in @dir_fh. */
 uiox_nfs_err_t uiox_nfs3_create     (uiox_nfs3_ctx_t *ctx,
                                        const uiox_nfs_fh_t *dir_fh,
                                        const char *name, uint32_t mode,
                                        uiox_nfs_fh_t *out_fh);
 
 /** NFSPROC3_MKDIR: create a directory in @dir_fh. */
 uiox_nfs_err_t uiox_nfs3_mkdir      (uiox_nfs3_ctx_t *ctx,
                                        const uiox_nfs_fh_t *dir_fh,
                                        const char *name, uint32_t mode,
                                        uiox_nfs_fh_t *out_fh);
 
 /** NFSPROC3_REMOVE: remove a file. */
 uiox_nfs_err_t uiox_nfs3_remove     (uiox_nfs3_ctx_t *ctx,
                                        const uiox_nfs_fh_t *dir_fh,
                                        const char *name);
 
 /** NFSPROC3_RENAME: rename a file. */
 uiox_nfs_err_t uiox_nfs3_rename     (uiox_nfs3_ctx_t *ctx,
                                        const uiox_nfs_fh_t *from_dir,
                                        const char *from_name,
                                        const uiox_nfs_fh_t *to_dir,
                                        const char *to_name);
 
 /** NFSPROC3_READDIR: read directory entries.
  *  Calls @cb for each entry; returns when @cb returns false or dir ends. */
 typedef bool (*uiox_nfs3_readdir_cb_t)(const uiox_nfs_dirent_t *de,
                                          void *priv);
 uiox_nfs_err_t uiox_nfs3_readdir    (uiox_nfs3_ctx_t *ctx,
                                        const uiox_nfs_fh_t *dir_fh,
                                        uiox_nfs3_readdir_cb_t cb,
                                        void *priv);
 
 /** NFSPROC3_FSSTAT: filesystem statistics. */
 uiox_nfs_err_t uiox_nfs3_fsstat     (uiox_nfs3_ctx_t *ctx,
                                        const uiox_nfs_fh_t *fh,
                                        uiox_nfs_statfs_t *stat);
 
 /** NFSPROC3_COMMIT: flush writes to stable storage. */
 uiox_nfs_err_t uiox_nfs3_commit     (uiox_nfs3_ctx_t *ctx,
                                        const uiox_nfs_fh_t *fh,
                                        uint64_t offset, uint32_t count);
 
 /** Path walk: repeatedly LOOKUP each component of @path from root. */
 uiox_nfs_err_t uiox_nfs3_path_lookup(uiox_nfs3_ctx_t *ctx,
                                        const char *path,
                                        uiox_nfs_fh_t *out_fh,
                                        uiox_nfs_attr_t *out_attr);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_NFS_PROTO_H */
 