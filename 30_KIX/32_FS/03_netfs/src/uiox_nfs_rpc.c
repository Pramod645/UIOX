/**
 * @file  uiox_nfs_rpc.c
 * @brief UIOX NFS — XDR encode/decode + ONC RPC call/reply. No libc.
 * @date  2026-07-07
 */

 #include "../include/uiox_nfs_rpc.h"

 /* ── No-libc helpers ────────────────────────────────────────── */
 
 static void xdr_memcpy(void *d, const void *s, size_t n)
 { uint8_t *dp=(uint8_t*)d; const uint8_t *sp=(const uint8_t*)s;
   while(n--)*dp++=*sp++; }
 
 static void xdr_memset(void *d, int v, size_t n)
 { uint8_t *dp=(uint8_t*)d; while(n--)*dp++=(uint8_t)v; }
 
 static size_t xdr_strlen(const char *s)
 { size_t n=0; while(*s++)n++; return n; }
 
 /* =========================================================================
  * XDR encoder / decoder
  * ====================================================================== */
 
 void uiox_xdr_init_enc(uiox_xdr_t *x, uint8_t *buf, uint32_t size)
 { x->data=buf; x->size=size; x->pos=0u; x->error=false; }
 
 void uiox_xdr_init_dec(uiox_xdr_t *x, uint8_t *buf, uint32_t size)
 { x->data=buf; x->size=size; x->pos=0u; x->error=false; }
 
 static void xdr_align(uiox_xdr_t *x, uint32_t n)
 {
     uint32_t pad = (4u - (n % 4u)) % 4u;
     if (x->pos + n + pad > x->size) { x->error=true; return; }
     x->pos += n + pad;
 }
 
 void uiox_xdr_enc_u32(uiox_xdr_t *x, uint32_t v)
 {
     if (x->pos + 4u > x->size) { x->error=true; return; }
     x->data[x->pos+0] = (uint8_t)(v >> 24u);
     x->data[x->pos+1] = (uint8_t)(v >> 16u);
     x->data[x->pos+2] = (uint8_t)(v >>  8u);
     x->data[x->pos+3] = (uint8_t)(v       );
     x->pos += 4u;
 }
 
 void uiox_xdr_enc_u64(uiox_xdr_t *x, uint64_t v)
 {
     uiox_xdr_enc_u32(x, (uint32_t)(v >> 32u));
     uiox_xdr_enc_u32(x, (uint32_t)(v       ));
 }
 
 void uiox_xdr_enc_bytes(uiox_xdr_t *x, const uint8_t *b, uint32_t len)
 {
     uiox_xdr_enc_u32(x, len);
     uint32_t pad = (4u - (len % 4u)) % 4u;
     if (x->pos + len + pad > x->size) { x->error=true; return; }
     xdr_memcpy(x->data + x->pos, b, len);
     xdr_memset(x->data + x->pos + len, 0, pad);
     x->pos += len + pad;
 }
 
 void uiox_xdr_enc_str(uiox_xdr_t *x, const char *s)
 {
     uint32_t len = (uint32_t)xdr_strlen(s);
     uiox_xdr_enc_bytes(x, (const uint8_t *)s, len);
 }
 
 void uiox_xdr_enc_fh(uiox_xdr_t *x, const uiox_nfs_fh_t *fh)
 {
     uiox_xdr_enc_bytes(x, fh->data, fh->len);
 }
 
 uint32_t uiox_xdr_dec_u32(uiox_xdr_t *x)
 {
     if (x->pos + 4u > x->size) { x->error=true; return 0u; }
     uint32_t v = ((uint32_t)x->data[x->pos+0] << 24u)
                | ((uint32_t)x->data[x->pos+1] << 16u)
                | ((uint32_t)x->data[x->pos+2] <<  8u)
                |  (uint32_t)x->data[x->pos+3];
     x->pos += 4u;
     return v;
 }
 
 uint64_t uiox_xdr_dec_u64(uiox_xdr_t *x)
 {
     uint64_t hi = uiox_xdr_dec_u32(x);
     uint64_t lo = uiox_xdr_dec_u32(x);
     return (hi << 32u) | lo;
 }
 
 uint32_t uiox_xdr_dec_bytes(uiox_xdr_t *x, uint8_t *buf, uint32_t max)
 {
     uint32_t len = uiox_xdr_dec_u32(x);
     uint32_t pad = (4u - (len % 4u)) % 4u;
     if (x->pos + len + pad > x->size) { x->error=true; return 0u; }
     uint32_t copy = (len < max) ? len : max - 1u;
     if (buf) { xdr_memcpy(buf, x->data + x->pos, copy); buf[copy]='\0'; }
     x->pos += len + pad;
     return len;
 }
 
 uint32_t uiox_xdr_dec_str(uiox_xdr_t *x, char *buf, uint32_t max)
 { return uiox_xdr_dec_bytes(x, (uint8_t *)buf, max); }
 
 void uiox_xdr_dec_fh(uiox_xdr_t *x, uiox_nfs_fh_t *fh)
 {
     fh->len = uiox_xdr_dec_bytes(x, fh->data, UIOX_NFS_FH_MAX_LEN);
 }
 
 void uiox_xdr_dec_attr(uiox_xdr_t *x, uiox_nfs_attr_t *attr)
 {
     attr->ftype     = (uiox_nfs_ftype_t)uiox_xdr_dec_u32(x);
     attr->mode      = uiox_xdr_dec_u32(x);
     attr->nlink     = uiox_xdr_dec_u32(x);
     attr->uid       = uiox_xdr_dec_u32(x);
     attr->gid       = uiox_xdr_dec_u32(x);
     attr->size      = uiox_xdr_dec_u64(x);
     attr->used      = uiox_xdr_dec_u64(x);
     attr->rdev_major= uiox_xdr_dec_u32(x);
     attr->rdev_minor= uiox_xdr_dec_u32(x);
     attr->fsid      = uiox_xdr_dec_u64(x);
     attr->fileid    = uiox_xdr_dec_u64(x);
     attr->atime_sec = uiox_xdr_dec_u64(x);
     attr->mtime_sec = uiox_xdr_dec_u64(x);
     attr->ctime_sec = uiox_xdr_dec_u64(x);
     attr->valid     = !x->error;
 }
 
 void uiox_xdr_skip(uiox_xdr_t *x, uint32_t bytes)
 {
     if (x->pos + bytes > x->size) { x->error=true; return; }
     x->pos += bytes;
 }
 
 /* =========================================================================
  * RPC call / reply
  * ====================================================================== */
 
 uiox_nfs_err_t uiox_rpc_init(uiox_rpc_ctx_t *ctx,
                                const uint8_t server_ip[4],
                                uint16_t port,
                                uiox_rpc_transport_t transport)
 {
     if (!ctx) return UIOX_NFS_ERR_INVAL;
     xdr_memset(ctx, 0, sizeof(*ctx));
     ctx->server_ip[0] = server_ip[0];
     ctx->server_ip[1] = server_ip[1];
     ctx->server_ip[2] = server_ip[2];
     ctx->server_ip[3] = server_ip[3];
     ctx->port         = port;
     ctx->transport    = transport;
     ctx->xid          = 0x12345678u;
     ctx->timeout_ms   = 5000u;
     ctx->retries      = 3u;
     return UIOX_NFS_OK;
 }
 
 uiox_nfs_err_t uiox_rpc_call(uiox_rpc_ctx_t *ctx,
                                uint32_t prog, uint32_t vers, uint32_t proc,
                                const uint8_t *args, uint32_t args_len,
                                uint8_t **reply, uint32_t *reply_len)
 {
     if (!ctx || !ctx->send || !ctx->recv) return UIOX_NFS_ERR_NODEV;
 
     /* Build RPC call header */
     uiox_xdr_t enc;
     uiox_xdr_init_enc(&enc, ctx->tx_buf, UIOX_RPC_BUF_SIZE);
 
     uint32_t xid = ctx->xid++;
     /* For TCP: reserve 4 bytes for Record Mark */
     uint32_t hdr_start = (ctx->transport == UIOX_RPC_TCP) ? 4u : 0u;
     enc.pos = hdr_start;
 
     uiox_xdr_enc_u32(&enc, xid);          /* XID                      */
     uiox_xdr_enc_u32(&enc, RPC_CALL);     /* msg type = CALL          */
     uiox_xdr_enc_u32(&enc, 2u);           /* RPC version 2            */
     uiox_xdr_enc_u32(&enc, prog);         /* program                  */
     uiox_xdr_enc_u32(&enc, vers);         /* version                  */
     uiox_xdr_enc_u32(&enc, proc);         /* procedure                */
     uiox_xdr_enc_u32(&enc, 0u);           /* AUTH_NULL cred flavor    */
     uiox_xdr_enc_u32(&enc, 0u);           /* cred length = 0          */
     uiox_xdr_enc_u32(&enc, 0u);           /* AUTH_NULL verifier       */
     uiox_xdr_enc_u32(&enc, 0u);           /* verifier length = 0      */
 
     /* Append arguments */
     if (args && args_len > 0u) {
         if (enc.pos + args_len > UIOX_RPC_BUF_SIZE) return UIOX_NFS_ERR_NOMEM;
         xdr_memcpy(ctx->tx_buf + enc.pos, args, args_len);
         enc.pos += args_len;
     }
 
     uint32_t pkt_len = enc.pos - hdr_start;
 
     /* TCP Record Mark: last fragment bit set, length in bits [30:0] */
     if (ctx->transport == UIOX_RPC_TCP) {
         uint32_t rm = 0x80000000u | pkt_len;
         ctx->tx_buf[0] = (uint8_t)(rm >> 24u);
         ctx->tx_buf[1] = (uint8_t)(rm >> 16u);
         ctx->tx_buf[2] = (uint8_t)(rm >>  8u);
         ctx->tx_buf[3] = (uint8_t)(rm       );
     }
 
     /* Send with retries */
     for (uint32_t attempt = 0u; attempt <= ctx->retries; attempt++) {
         uiox_nfs_err_t rc = ctx->send(ctx->net_ctx,
                                         ctx->tx_buf, enc.pos);
         if (rc != UIOX_NFS_OK) { ctx->errors++; continue; }
 
         ctx->rx_len = 0u;
         rc = ctx->recv(ctx->net_ctx,
                         ctx->rx_buf, UIOX_RPC_BUF_SIZE,
                         &ctx->rx_len, ctx->timeout_ms);
         if (rc != UIOX_NFS_OK) {
             ctx->retransmits++;
             continue;
         }
 
         /* Parse RPC reply header */
         uiox_xdr_t dec;
         uint32_t rx_start = (ctx->transport == UIOX_RPC_TCP) ? 4u : 0u;
         uiox_xdr_init_dec(&dec, ctx->rx_buf + rx_start,
                             ctx->rx_len - rx_start);
 
         uint32_t r_xid    = uiox_xdr_dec_u32(&dec);
         uint32_t r_type   = uiox_xdr_dec_u32(&dec);
         uint32_t r_status = uiox_xdr_dec_u32(&dec);
 
         if (r_xid != xid || r_type != RPC_REPLY ||
             r_status != RPC_MSG_ACCEPTED) {
             ctx->errors++;
             return UIOX_NFS_ERR_PROTO;
         }
 
         /* Skip verifier */
         uiox_xdr_skip(&dec, 8u);
 
         uint32_t accept_stat = uiox_xdr_dec_u32(&dec);
         if (accept_stat != RPC_SUCCESS) {
             ctx->errors++;
             return UIOX_NFS_ERR_PROTO;
         }
 
         *reply     = ctx->rx_buf + rx_start + dec.pos;
         *reply_len = ctx->rx_len - rx_start - dec.pos;
         ctx->calls++;
         return UIOX_NFS_OK;
     }
 
     ctx->errors++;
     return UIOX_NFS_ERR_TIMEOUT;
 }
 
 /* Forward */
 extern void uiox_fw_printf(const char *fmt, ...);
 
 void uiox_rpc_print(const uiox_rpc_ctx_t *ctx)
 {
     if (!ctx) return;
     uiox_fw_printf("[RPC] server=%u.%u.%u.%u:%u  transport=%s\n",
                     ctx->server_ip[0], ctx->server_ip[1],
                     ctx->server_ip[2], ctx->server_ip[3],
                     ctx->port,
                     ctx->transport == UIOX_RPC_TCP ? "TCP" : "UDP");
     uiox_fw_printf("[RPC] calls=%u  retransmits=%u  errors=%u\n",
                     ctx->calls, ctx->retransmits, ctx->errors);
 }
 