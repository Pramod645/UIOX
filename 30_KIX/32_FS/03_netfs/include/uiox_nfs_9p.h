/**
 * @file  uiox_nfs_9p.h
 * @brief UIOX NFS — Plan 9 / 9P2000 client.
 *
 * Used by QEMU's -virtfs option (plan9 transport).
 * Simpler than NFS: stateless, single TCP connection, tag-multiplexed.
 *
 * @version 1.0.0
 */

 #ifndef UIOX_NFS_9P_H
 #define UIOX_NFS_9P_H
 
 #include "uiox_nfs_types.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * 9P2000 message types
  * ====================================================================== */
 
 #define P9_TVERSION  100u
 #define P9_RVERSION  101u
 #define P9_TATTACH   104u
 #define P9_RATTACH   105u
 #define P9_TERROR    106u
 #define P9_RERROR    107u
 #define P9_TFLUSH    108u
 #define P9_RFLUSH    109u
 #define P9_TWALK     110u
 #define P9_RWALK     111u
 #define P9_TOPEN     112u
 #define P9_ROPEN     113u
 #define P9_TCREATE   114u
 #define P9_RCREATE   115u
 #define P9_TREAD     116u
 #define P9_RREAD     117u
 #define P9_TWRITE    118u
 #define P9_RWRITE    119u
 #define P9_TCLUNK    120u
 #define P9_RCLUNK    121u
 #define P9_TREMOVE   122u
 #define P9_RREMOVE   123u
 #define P9_TSTAT     124u
 #define P9_RSTAT     125u
 #define P9_TWSTAT    126u
 #define P9_RWSTAT    127u
 
 /* 9P constants */
 #define P9_NOTAG     0xFFFFu   /**< No tag (used in TVERSION)          */
 #define P9_NOFID     0xFFFFFFFFu
 #define P9_MSIZE     65536u    /**< Default maximum message size        */
 #define P9_OREAD     0x00u
 #define P9_OWRITE    0x01u
 #define P9_ORDWR     0x02u
 #define P9_OEXEC     0x03u
 #define P9_OTRUNC    0x10u
 #define P9_DMDIR     0x80000000u
 
 /* =========================================================================
  * 9P client context
  * ====================================================================== */
 
 #define P9_MAX_FIDS  64u
 #define P9_FID_FREE  0xFFFFFFFFu
 
 typedef struct {
     uint32_t  fid;
     uintptr_t inode;   /**< VFS inode associated with this fid        */
     bool      open;
 } uiox_9p_fid_entry_t;
 
 typedef struct {
     /* Network */
     uiox_nfs_err_t (*send)(void *ctx, const uint8_t *buf, uint32_t len);
     uiox_nfs_err_t (*recv)(void *ctx, uint8_t *buf, uint32_t max,
                              uint32_t *rx_len, uint32_t timeout_ms);
     void           *net_ctx;
     /* Session */
     uint32_t         msize;      /**< Negotiated message size          */
     uint16_t         next_tag;
     uint32_t         next_fid;
     uint32_t         root_fid;   /**< FID of mount root                */
     char             aname[128]; /**< Attach name (export path)        */
     uint8_t          tx_buf[P9_MSIZE];
     uint8_t          rx_buf[P9_MSIZE];
     /* FID table */
     uiox_9p_fid_entry_t fids[P9_MAX_FIDS];
     bool             connected;
     /* Stats */
     uint32_t         reads;
     uint32_t         writes;
     uint32_t         errors;
 } uiox_9p_ctx_t;
 
 /* =========================================================================
  * 9P API
  * ====================================================================== */
 
 uiox_nfs_err_t uiox_9p_connect   (uiox_9p_ctx_t *ctx,
                                     const char *aname);
 uiox_nfs_err_t uiox_9p_disconnect (uiox_9p_ctx_t *ctx);
 
 uiox_nfs_err_t uiox_9p_walk      (uiox_9p_ctx_t *ctx,
                                     uint32_t fid, const char *path,
                                     uint32_t *new_fid);
 uiox_nfs_err_t uiox_9p_open      (uiox_9p_ctx_t *ctx,
                                     uint32_t fid, uint8_t mode);
 uiox_nfs_err_t uiox_9p_read      (uiox_9p_ctx_t *ctx,
                                     uint32_t fid, uint64_t offset,
                                     uint8_t *buf, uint32_t count,
                                     uint32_t *bytes_read);
 uiox_nfs_err_t uiox_9p_write     (uiox_9p_ctx_t *ctx,
                                     uint32_t fid, uint64_t offset,
                                     const uint8_t *buf, uint32_t count,
                                     uint32_t *bytes_written);
 uiox_nfs_err_t uiox_9p_clunk     (uiox_9p_ctx_t *ctx, uint32_t fid);
 uiox_nfs_err_t uiox_9p_stat      (uiox_9p_ctx_t *ctx,
                                     uint32_t fid, uiox_nfs_attr_t *attr);
 uiox_nfs_err_t uiox_9p_create    (uiox_9p_ctx_t *ctx,
                                     uint32_t dir_fid, const char *name,
                                     uint32_t perm, uint8_t mode,
                                     uint32_t *new_fid);
 uiox_nfs_err_t uiox_9p_remove    (uiox_9p_ctx_t *ctx, uint32_t fid);
 uint32_t       uiox_9p_alloc_fid  (uiox_9p_ctx_t *ctx);
 void           uiox_9p_free_fid   (uiox_9p_ctx_t *ctx, uint32_t fid);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_NFS_9P_H */
 