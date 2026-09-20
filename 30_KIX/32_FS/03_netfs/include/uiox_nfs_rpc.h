/**
 * @file  uiox_nfs_rpc.h
 * @brief UIOX NFS — RPC/XDR layer (UDP/TCP framing, no libc).
 *
 * Implements:
 *   - ONC RPC (RFC 5531) call/reply framing
 *   - XDR encode/decode for primitive types
 *   - UDP send/recv via uiox_fw_eth
 *   - TCP Record Marking (RFC 5531 §11)
 *
 * @version 1.0.0
 */

 #ifndef UIOX_NFS_RPC_H
 #define UIOX_NFS_RPC_H
 
 #include "uiox_nfs_types.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * RPC constants (ONC RPC / RFC 5531)
  * ====================================================================== */
 
 #define RPC_CALL            0u
 #define RPC_REPLY           1u
 #define RPC_MSG_ACCEPTED    0u
 #define RPC_MSG_DENIED      1u
 #define RPC_SUCCESS         0u
 #define RPC_PROG_UNAVAIL    1u
 #define RPC_PROG_MISMATCH   2u
 #define RPC_PROC_UNAVAIL    3u
 #define RPC_GARBAGE_ARGS    4u
 
 /* NFS program numbers */
 #define NFS_PORTMAPPER_PROG 100000u
 #define NFS_MOUNT_PROG      100005u
 #define NFS_PROG            100003u
 #define NFS_VERS3           3u
 #define NFS_MOUNT_VERS3     3u
 
 /* NFS v3 procedure numbers */
 #define NFSPROC3_NULL        0u
 #define NFSPROC3_GETATTR     1u
 #define NFSPROC3_SETATTR     2u
 #define NFSPROC3_LOOKUP      3u
 #define NFSPROC3_ACCESS      4u
 #define NFSPROC3_READLINK    5u
 #define NFSPROC3_READ        6u
 #define NFSPROC3_WRITE       7u
 #define NFSPROC3_CREATE      8u
 #define NFSPROC3_MKDIR       9u
 #define NFSPROC3_REMOVE     12u
 #define NFSPROC3_RMDIR      13u
 #define NFSPROC3_RENAME     14u
 #define NFSPROC3_READDIR    16u
 #define NFSPROC3_READDIRPLUS 17u
 #define NFSPROC3_FSSTAT     18u
 #define NFSPROC3_PATHCONF   20u
 #define NFSPROC3_COMMIT     21u
 
 /* MOUNT v3 procedure numbers */
 #define MOUNTPROC3_MNT       1u
 #define MOUNTPROC3_UMNT      3u
 
 /* NFS v3 status codes (on-wire) */
 #define NFS3_OK              0u
 #define NFS3ERR_PERM         1u
 #define NFS3ERR_NOENT        2u
 #define NFS3ERR_IO           5u
 #define NFS3ERR_ACCES       13u
 #define NFS3ERR_EXIST       17u
 #define NFS3ERR_NOTDIR      20u
 #define NFS3ERR_ISDIR       21u
 #define NFS3ERR_INVAL       22u
 #define NFS3ERR_NAMETOOLONG 63u
 #define NFS3ERR_STALE       70u
 
 /* =========================================================================
  * XDR buffer
  * ====================================================================== */
 
 #define UIOX_RPC_BUF_SIZE   (64u * 1024u)  /* 64 KB per RPC call        */
 
 typedef struct {
     uint8_t  *data;
     uint32_t  size;     /**< Total capacity                             */
     uint32_t  pos;      /**< Current encode/decode position             */
     bool      error;    /**< Encode/decode error flag                   */
 } uiox_xdr_t;
 
 /* =========================================================================
  * RPC connection context
  * ====================================================================== */
 
 typedef enum { UIOX_RPC_UDP=0, UIOX_RPC_TCP=1 } uiox_rpc_transport_t;
 
 typedef struct {
     uiox_rpc_transport_t transport;
     uint8_t   server_ip[4];   /**< IPv4 server address                  */
     uint16_t  port;           /**< NFS port (default 2049)              */
     uint32_t  xid;            /**< Transaction ID counter               */
     uint32_t  timeout_ms;     /**< RPC call timeout                     */
     uint32_t  retries;        /**< Retry count                          */
     /* TX/RX buffers */
     uint8_t   tx_buf[UIOX_RPC_BUF_SIZE];
     uint8_t   rx_buf[UIOX_RPC_BUF_SIZE];
     uint32_t  rx_len;
     /* Network send/receive callbacks (wired to uiox_fw_eth) */
     uiox_nfs_err_t (*send)(void *ctx,
                              const uint8_t *buf, uint32_t len);
     uiox_nfs_err_t (*recv)(void *ctx,
                              uint8_t *buf, uint32_t max_len,
                              uint32_t *rx_len, uint32_t timeout_ms);
     void *net_ctx;
     /* Stats */
     uint32_t calls;
     uint32_t retransmits;
     uint32_t errors;
 } uiox_rpc_ctx_t;
 
 /* =========================================================================
  * XDR encode/decode — no libc, big-endian wire format
  * ====================================================================== */
 
 void     uiox_xdr_init_enc (uiox_xdr_t *x, uint8_t *buf, uint32_t size);
 void     uiox_xdr_init_dec (uiox_xdr_t *x, uint8_t *buf, uint32_t size);
 
 void     uiox_xdr_enc_u32  (uiox_xdr_t *x, uint32_t v);
 void     uiox_xdr_enc_u64  (uiox_xdr_t *x, uint64_t v);
 void     uiox_xdr_enc_bytes(uiox_xdr_t *x, const uint8_t *b, uint32_t len);
 void     uiox_xdr_enc_str  (uiox_xdr_t *x, const char *s);
 void     uiox_xdr_enc_fh   (uiox_xdr_t *x, const uiox_nfs_fh_t *fh);
 
 uint32_t uiox_xdr_dec_u32  (uiox_xdr_t *x);
 uint64_t uiox_xdr_dec_u64  (uiox_xdr_t *x);
 uint32_t uiox_xdr_dec_bytes(uiox_xdr_t *x, uint8_t *buf, uint32_t max);
 uint32_t uiox_xdr_dec_str  (uiox_xdr_t *x, char *buf, uint32_t max);
 void     uiox_xdr_dec_fh   (uiox_xdr_t *x, uiox_nfs_fh_t *fh);
 void     uiox_xdr_dec_attr  (uiox_xdr_t *x, uiox_nfs_attr_t *attr);
 void     uiox_xdr_skip     (uiox_xdr_t *x, uint32_t bytes);
 
 /* =========================================================================
  * RPC call/reply
  * ====================================================================== */
 
 uiox_nfs_err_t uiox_rpc_init   (uiox_rpc_ctx_t *ctx,
                                   const uint8_t server_ip[4],
                                   uint16_t port,
                                   uiox_rpc_transport_t transport);
 
 /**
  * Build RPC call header, call send/recv, parse reply header.
  * @param prog    RPC program number.
  * @param vers    RPC version.
  * @param proc    Procedure number.
  * @param args    XDR-encoded argument buffer.
  * @param args_len Argument byte count.
  * @param reply   Output: parsed reply payload in ctx->rx_buf.
  * @param reply_len Output: reply payload byte count.
  */
 uiox_nfs_err_t uiox_rpc_call   (uiox_rpc_ctx_t *ctx,
                                   uint32_t prog, uint32_t vers, uint32_t proc,
                                   const uint8_t *args, uint32_t args_len,
                                   uint8_t **reply, uint32_t *reply_len);
 
 void           uiox_rpc_print  (const uiox_rpc_ctx_t *ctx);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_NFS_RPC_H */
 