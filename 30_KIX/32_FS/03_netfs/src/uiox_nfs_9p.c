/**
 * @file  uiox_nfs_9p.c
 * @brief UIOX NFS — 9P2000 client. No libc.
 * @date  2026-07-07
 */

 #include "../include/uiox_nfs_9p.h"

 static void p9_memset(void *d,int v,size_t n)
 { uint8_t *p=(uint8_t*)d;while(n--)*p++=(uint8_t)v; }
 static void p9_memcpy(void *d,const void *s,size_t n)
 { uint8_t *dp=(uint8_t*)d;const uint8_t *sp=(const uint8_t*)s;while(n--)*dp++=*sp++; }
 
 /* ── LE 16/32 helpers ───────────────────────────────────────── */
 static void le16(uint8_t *p, uint16_t v){ p[0]=(uint8_t)v; p[1]=(uint8_t)(v>>8); }
 static void le32(uint8_t *p, uint32_t v){ p[0]=(uint8_t)v; p[1]=(uint8_t)(v>>8); p[2]=(uint8_t)(v>>16); p[3]=(uint8_t)(v>>24); }
 static void le64(uint8_t *p, uint64_t v){ le32(p,(uint32_t)v); le32(p+4,(uint32_t)(v>>32)); }
 static uint16_t rl16(const uint8_t *p){ return (uint16_t)(p[0]|(uint16_t)p[1]<<8); }
 static uint32_t rl32(const uint8_t *p){ return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24); }
 static uint64_t rl64(const uint8_t *p){ return (uint64_t)rl32(p)|(((uint64_t)rl32(p+4))<<32); }
 
 /* ── String helpers ─────────────────────────────────────────── */
 static size_t p9_strlen(const char *s){size_t n=0;while(*s++)n++;return n;}
 
 static uint32_t p9_enc_str(uint8_t *buf, uint32_t pos, const char *s)
 {
     uint16_t len = (uint16_t)p9_strlen(s);
     le16(buf+pos, len); pos += 2u;
     p9_memcpy(buf+pos, s, len); pos += len;
     return pos;
 }
 
 static uint32_t p9_dec_str(const uint8_t *buf, uint32_t pos, char *out, uint32_t max)
 {
     uint16_t len = rl16(buf+pos); pos += 2u;
     uint32_t copy = (len < max-1u) ? len : max-1u;
     p9_memcpy(out, buf+pos, copy); out[copy]='\0';
     return pos + len;
 }
 
 /* =========================================================================
  * send_recv wrapper
  * ====================================================================== */
 
 static uiox_nfs_err_t p9_transact(uiox_9p_ctx_t *ctx,
                                      uint8_t msg_type,
                                      uint32_t payload_len,
                                      uint16_t tag)
 {
     /* Build 9P header: size[4] type[1] tag[2] */
     uint32_t total = 7u + payload_len;
     le32(ctx->tx_buf, total);
     ctx->tx_buf[4] = msg_type;
     le16(ctx->tx_buf + 5u, tag);
 
     uiox_nfs_err_t rc = ctx->send(ctx->net_ctx, ctx->tx_buf, total);
     if (rc != UIOX_NFS_OK) { ctx->errors++; return rc; }
 
     uint32_t rx_len = 0u;
     rc = ctx->recv(ctx->net_ctx, ctx->rx_buf, P9_MSIZE, &rx_len, 5000u);
     if (rc != UIOX_NFS_OK) { ctx->errors++; return rc; }
 
     /* Check for RERROR */
     if (ctx->rx_buf[4] == P9_RERROR) return UIOX_NFS_ERR_IO;
     return UIOX_NFS_OK;
 }
 
 /* =========================================================================
  * Connection
  * ====================================================================== */
 
 uiox_nfs_err_t uiox_9p_connect(uiox_9p_ctx_t *ctx, const char *aname)
 {
     if (!ctx || !ctx->send || !ctx->recv) return UIOX_NFS_ERR_NODEV;
 
     /* TVERSION */
     uint32_t pos = 7u;
     le32(ctx->tx_buf + pos, P9_MSIZE); pos += 4u;
     pos = p9_enc_str(ctx->tx_buf, pos, "9P2000");
     uiox_nfs_err_t rc = p9_transact(ctx, P9_TVERSION, pos - 7u, P9_NOTAG);
     if (rc != UIOX_NFS_OK) return rc;
     ctx->msize = rl32(ctx->rx_buf + 7u);
 
     /* TATTACH: fid=1, afid=NOFID, uname="root", aname=export */
     ctx->next_fid = 1u;
     ctx->root_fid = uiox_9p_alloc_fid(ctx);
     pos = 7u;
     le32(ctx->tx_buf+pos, ctx->root_fid); pos+=4u;
     le32(ctx->tx_buf+pos, P9_NOFID);      pos+=4u;
     pos = p9_enc_str(ctx->tx_buf, pos, "root");
     pos = p9_enc_str(ctx->tx_buf, pos, aname ? aname : "/");
     le32(ctx->tx_buf+pos, 0u);             pos+=4u; /* n_uname */
     rc = p9_transact(ctx, P9_TATTACH, pos - 7u, ctx->next_tag++);
     if (rc != UIOX_NFS_OK) return rc;
 
     /* aname stored */
     size_t al = p9_strlen(aname ? aname : "/");
     p9_memcpy(ctx->aname, aname ? aname : "/", al < 127u ? al : 127u);
 
     ctx->connected = true;
     return UIOX_NFS_OK;
 }
 
 uiox_nfs_err_t uiox_9p_disconnect(uiox_9p_ctx_t *ctx)
 {
     if (!ctx) return UIOX_NFS_ERR_INVAL;
     uiox_9p_clunk(ctx, ctx->root_fid);
     ctx->connected = false;
     return UIOX_NFS_OK;
 }
 
 /* =========================================================================
  * Walk / open / read / write / clunk / stat / create / remove
  * ====================================================================== */
 
 uiox_nfs_err_t uiox_9p_walk(uiox_9p_ctx_t *ctx,
                                uint32_t fid, const char *path,
                                uint32_t *new_fid)
 {
     uint32_t nfid = uiox_9p_alloc_fid(ctx);
     uint32_t pos = 7u;
     le32(ctx->tx_buf+pos, fid);  pos+=4u;
     le32(ctx->tx_buf+pos, nfid); pos+=4u;
     /* Count components */
     uint16_t nwname = 0u;
     const char *p = path; if (*p=='/') p++;
     const char *tmp = p;
     while (*tmp) { if (*tmp=='/') nwname++; tmp++; }
     if (*p) nwname++;
     le16(ctx->tx_buf+pos, nwname); pos+=2u;
     /* Encode each component */
     p = path; if (*p=='/') p++;
     while (*p) {
         char comp[UIOX_NFS_NAME_MAX+1u]; int ci=0;
         while (*p && *p!='/' && ci<(int)UIOX_NFS_NAME_MAX)
             comp[ci++]=*p++;
         comp[ci]='\0'; if (*p=='/') p++;
         pos = p9_enc_str(ctx->tx_buf, pos, comp);
     }
     uiox_nfs_err_t rc = p9_transact(ctx, P9_TWALK, pos-7u, ctx->next_tag++);
     if (rc != UIOX_NFS_OK) { uiox_9p_free_fid(ctx,nfid); return rc; }
     *new_fid = nfid;
     return UIOX_NFS_OK;
 }
 
 uiox_nfs_err_t uiox_9p_open(uiox_9p_ctx_t *ctx, uint32_t fid, uint8_t mode)
 {
     uint32_t pos = 7u;
     le32(ctx->tx_buf+pos, fid); pos+=4u;
     ctx->tx_buf[pos++] = mode;
     return p9_transact(ctx, P9_TOPEN, pos-7u, ctx->next_tag++);
 }
 
 uiox_nfs_err_t uiox_9p_read(uiox_9p_ctx_t *ctx,
                                uint32_t fid, uint64_t offset,
                                uint8_t *buf, uint32_t count,
                                uint32_t *bytes_read)
 {
     uint32_t pos = 7u;
     le32(ctx->tx_buf+pos, fid);    pos+=4u;
     le64(ctx->tx_buf+pos, offset); pos+=8u;
     le32(ctx->tx_buf+pos, count);  pos+=4u;
     uiox_nfs_err_t rc = p9_transact(ctx, P9_TREAD, pos-7u, ctx->next_tag++);
     if (rc != UIOX_NFS_OK) return rc;
     uint32_t got = rl32(ctx->rx_buf + 7u);
     p9_memcpy(buf, ctx->rx_buf + 11u, got < count ? got : count);
     if (bytes_read) *bytes_read = got;
     ctx->reads++;
     return UIOX_NFS_OK;
 }
 
 uiox_nfs_err_t uiox_9p_write(uiox_9p_ctx_t *ctx,
                                 uint32_t fid, uint64_t offset,
                                 const uint8_t *buf, uint32_t count,
                                 uint32_t *bytes_written)
 {
     uint32_t pos = 7u;
     le32(ctx->tx_buf+pos, fid);    pos+=4u;
     le64(ctx->tx_buf+pos, offset); pos+=8u;
     le32(ctx->tx_buf+pos, count);  pos+=4u;
     p9_memcpy(ctx->tx_buf+pos, buf, count); pos+=count;
     uiox_nfs_err_t rc = p9_transact(ctx, P9_TWRITE, pos-7u, ctx->next_tag++);
     if (rc != UIOX_NFS_OK) return rc;
     uint32_t written = rl32(ctx->rx_buf + 7u);
     if (bytes_written) *bytes_written = written;
     ctx->writes++;
     return UIOX_NFS_OK;
 }
 
 uiox_nfs_err_t uiox_9p_clunk(uiox_9p_ctx_t *ctx, uint32_t fid)
 {
     uint32_t pos = 7u;
     le32(ctx->tx_buf+pos, fid); pos+=4u;
     uiox_9p_free_fid(ctx, fid);
     return p9_transact(ctx, P9_TCLUNK, pos-7u, ctx->next_tag++);
 }
 
 uiox_nfs_err_t uiox_9p_stat(uiox_9p_ctx_t *ctx, uint32_t fid,
                                uiox_nfs_attr_t *attr)
 {
     uint32_t pos = 7u;
     le32(ctx->tx_buf+pos, fid); pos+=4u;
     uiox_nfs_err_t rc = p9_transact(ctx, P9_TSTAT, pos-7u, ctx->next_tag++);
     if (rc != UIOX_NFS_OK) return rc;
     /* Parse minimal stat: size[2] type[2] dev[4] qid[13] mode[4] ... size8[8] */
     const uint8_t *s = ctx->rx_buf + 9u;  /* skip size[2] */
     attr->mode   = rl32(s + 17u);
     attr->size   = rl64(s + 37u);
     attr->ftype  = (attr->mode & P9_DMDIR) ? UIOX_NFS_FTYPE_DIR
                                             : UIOX_NFS_FTYPE_REG;
     attr->valid  = true;
     return UIOX_NFS_OK;
 }
 
 uiox_nfs_err_t uiox_9p_create(uiox_9p_ctx_t *ctx,
                                  uint32_t dir_fid, const char *name,
                                  uint32_t perm, uint8_t mode,
                                  uint32_t *new_fid)
 {
     /* Walk to dir, then TCREATE on the cloned fid */
     uint32_t cfid = uiox_9p_alloc_fid(ctx);
     uint32_t pos = 7u;
     le32(ctx->tx_buf+pos, cfid); pos+=4u;
     pos = p9_enc_str(ctx->tx_buf, pos, name);
     le32(ctx->tx_buf+pos, perm); pos+=4u;
     ctx->tx_buf[pos++] = mode;
     uiox_nfs_err_t rc = p9_transact(ctx, P9_TCREATE, pos-7u, ctx->next_tag++);
     if (rc != UIOX_NFS_OK) { uiox_9p_free_fid(ctx,cfid); return rc; }
     *new_fid = cfid;
     return UIOX_NFS_OK;
 }
 
 uiox_nfs_err_t uiox_9p_remove(uiox_9p_ctx_t *ctx, uint32_t fid)
 {
     uint32_t pos = 7u;
     le32(ctx->tx_buf+pos, fid); pos+=4u;
     uiox_9p_free_fid(ctx, fid);
     return p9_transact(ctx, P9_TREMOVE, pos-7u, ctx->next_tag++);
 }
 
 uint32_t uiox_9p_alloc_fid(uiox_9p_ctx_t *ctx)
 {
     for (uint32_t i=0u;i<P9_MAX_FIDS;i++) {
         if (!ctx->fids[i].open) {
             ctx->fids[i].fid  = ctx->next_fid++;
             ctx->fids[i].open = true;
             return ctx->fids[i].fid;
         }
     }
     return P9_NOFID;
 }
 
 void uiox_9p_free_fid(uiox_9p_ctx_t *ctx, uint32_t fid)
 {
     for (uint32_t i=0u;i<P9_MAX_FIDS;i++) {
         if (ctx->fids[i].open && ctx->fids[i].fid == fid) {
             ctx->fids[i].open = false;
             return;
         }
     }
 }
 