/**
 * @file  uiox_nfs_virtfs.h
 * @brief UIOX NFS — VirtIO-FS client (FUSE over VirtIO MMIO).
 *
 * VirtIO-FS uses the FUSE protocol over a VirtIO device.
 * The host (QEMU) acts as the FUSE server; the guest sends
 * FUSE requests via VirtIO queues.
 *
 * @version 1.0.0
 */

 #ifndef UIOX_NFS_VIRTFS_H
 #define UIOX_NFS_VIRTFS_H
 
 #include "uiox_nfs_types.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * VirtIO-FS device registers (VirtIO MMIO spec §4.2)
  * ====================================================================== */
 
 #define VIRTIO_FS_MMIO_MAGIC         0x000u  /* 0x74726976 = "virt"  */
 #define VIRTIO_FS_MMIO_VERSION       0x004u
 #define VIRTIO_FS_MMIO_DEVICE_ID     0x008u  /* 26 = Filesystem       */
 #define VIRTIO_FS_MMIO_STATUS        0x070u
 #define VIRTIO_FS_MMIO_QUEUE_SEL     0x030u
 #define VIRTIO_FS_MMIO_QUEUE_NUM     0x038u
 #define VIRTIO_FS_MMIO_QUEUE_NOTIFY  0x050u
 #define VIRTIO_FS_MMIO_CONFIG        0x100u  /* tag (36 bytes)        */
 
 #define VIRTIO_FS_DEVICE_ID          26u
 #define VIRTIO_FS_STATUS_ACK         0x01u
 #define VIRTIO_FS_STATUS_DRIVER      0x02u
 #define VIRTIO_FS_STATUS_DRIVER_OK   0x04u
 #define VIRTIO_FS_STATUS_FEATURES_OK 0x08u
 
 /* =========================================================================
  * FUSE kernel protocol constants (FUSE_KERNEL_VERSION 7)
  * ====================================================================== */
 
 #define FUSE_KERNEL_VERSION   7u
 #define FUSE_KERNEL_MINOR_VERSION 31u
 
 /* FUSE opcodes */
 #define FUSE_LOOKUP      1u
 #define FUSE_FORGET      2u
 #define FUSE_GETATTR     3u
 #define FUSE_SETATTR     4u
 #define FUSE_READLINK    5u
 #define FUSE_SYMLINK     6u
 #define FUSE_MKNOD       8u
 #define FUSE_MKDIR       9u
 #define FUSE_UNLINK     10u
 #define FUSE_RMDIR      11u
 #define FUSE_RENAME     12u
 #define FUSE_OPEN       14u
 #define FUSE_READ       15u
 #define FUSE_WRITE      16u
 #define FUSE_STATFS     17u
 #define FUSE_RELEASE    18u
 #define FUSE_FSYNC      20u
 #define FUSE_READDIR    28u
 #define FUSE_INIT       26u
 #define FUSE_OPENDIR    27u
 #define FUSE_RELEASEDIR 29u
 #define FUSE_CREATE     35u
 
 /* FUSE_INIT flags */
 #define FUSE_ASYNC_READ          (1u << 0)
 #define FUSE_BIG_WRITES          (1u << 5)
 #define FUSE_WRITEBACK_CACHE     (1u << 16)
 
 /* FUSE in-header */
 typedef struct __attribute__((packed)) {
     uint32_t len;
     uint32_t opcode;
     uint64_t unique;
     uint64_t nodeid;
     uint32_t uid;
     uint32_t gid;
     uint32_t pid;
     uint32_t padding;
 } uiox_fuse_in_hdr_t;
 
 /* FUSE out-header */
 typedef struct __attribute__((packed)) {
     uint32_t len;
     int32_t  error;
     uint64_t unique;
 } uiox_fuse_out_hdr_t;
 
 /* =========================================================================
  * VirtIO-FS client context
  * ====================================================================== */
 
 #define VIRTFS_BUF_SIZE  (256u * 1024u)   /* 256 KB request/reply buf */
 
 typedef struct {
     uintptr_t  mmio_base;
     uint64_t   unique_counter;
     uint8_t    tx_buf[VIRTFS_BUF_SIZE];
     uint8_t    rx_buf[VIRTFS_BUF_SIZE];
     bool       initialized;
     char       tag[36];        /**< Filesystem tag from device config  */
     /* FUSE root nodeid = 1 always */
     uint64_t   root_nodeid;
     /* Stats */
     uint64_t   bytes_read;
     uint64_t   bytes_written;
     uint32_t   requests;
     uint32_t   errors;
 } uiox_virtfs_ctx_t;
 
 /* =========================================================================
  * VirtIO-FS API
  * ====================================================================== */
 
 uiox_nfs_err_t uiox_virtfs_init       (uiox_virtfs_ctx_t *ctx,
                                          uintptr_t mmio_base);
 void           uiox_virtfs_deinit     (uiox_virtfs_ctx_t *ctx);
 
 uiox_nfs_err_t uiox_virtfs_lookup     (uiox_virtfs_ctx_t *ctx,
                                          uint64_t parent, const char *name,
                                          uint64_t *nodeid,
                                          uiox_nfs_attr_t *attr);
 uiox_nfs_err_t uiox_virtfs_getattr    (uiox_virtfs_ctx_t *ctx,
                                          uint64_t nodeid,
                                          uiox_nfs_attr_t *attr);
 uiox_nfs_err_t uiox_virtfs_open       (uiox_virtfs_ctx_t *ctx,
                                          uint64_t nodeid, uint32_t flags,
                                          uint64_t *fh);
 uiox_nfs_err_t uiox_virtfs_read       (uiox_virtfs_ctx_t *ctx,
                                          uint64_t fh, uint64_t nodeid,
                                          uint64_t offset,
                                          uint8_t *buf, uint32_t size,
                                          uint32_t *bytes_read);
 uiox_nfs_err_t uiox_virtfs_write      (uiox_virtfs_ctx_t *ctx,
                                          uint64_t fh, uint64_t nodeid,
                                          uint64_t offset,
                                          const uint8_t *buf, uint32_t size,
                                          uint32_t *bytes_written);
 uiox_nfs_err_t uiox_virtfs_release    (uiox_virtfs_ctx_t *ctx,
                                          uint64_t fh, uint64_t nodeid);
 uiox_nfs_err_t uiox_virtfs_mkdir      (uiox_virtfs_ctx_t *ctx,
                                          uint64_t parent, const char *name,
                                          uint32_t mode, uint64_t *nodeid);
 uiox_nfs_err_t uiox_virtfs_unlink     (uiox_virtfs_ctx_t *ctx,
                                          uint64_t parent, const char *name);
 uiox_nfs_err_t uiox_virtfs_readdir    (uiox_virtfs_ctx_t *ctx,
                                          uint64_t nodeid, uint64_t fh,
                                          uint64_t offset,
                                          uiox_nfs3_readdir_cb_t cb,
                                          void *priv);
 uiox_nfs_err_t uiox_virtfs_statfs     (uiox_virtfs_ctx_t *ctx,
                                          uint64_t nodeid,
                                          uiox_nfs_statfs_t *stat);
 uiox_nfs_err_t uiox_virtfs_path_lookup(uiox_virtfs_ctx_t *ctx,
                                          const char *path,
                                          uint64_t *nodeid,
                                          uiox_nfs_attr_t *attr);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_NFS_VIRTFS_H */
 