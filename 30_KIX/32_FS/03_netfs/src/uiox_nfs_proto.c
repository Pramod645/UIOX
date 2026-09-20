/**
 * @file  uiox_nfs_proto.c
 * @brief UIOX NFS v3 client procedure implementations. No libc.
 * @date  2026-07-07
 */

 #include "../include/uiox_nfs_proto.h"

 extern void uiox_fw_printf(const char *fmt, ...);
 
 static void nfs_memset(void *d,int v,size_t n)
 { uint8_t *p=(uint8_t*)d;while(n--)*p++=(uint8_t)v; }
 static void nfs_memcpy(void *d,const void *s,size_t n)
 { uint8_t *dp=(uint8_t*)d;const uint8_t *sp=(const uint8_t*)s;while(n--)*dp++=*sp++; }
 static size_t nfs_strlen(const char *s){size_t n=0;while(*s++)n++;return n;}
 static void nfs_strncpy(char *d,const char *s,size_t n)
 { size_t i=0;while(i<n-1&&s[i]){d[i]=s[i];i++;}d[i]='\0'; }
 
 /* Map NFS3 on-wire status to uiox_nfs_err_t */
 static uiox_nfs_err_t nfs3_status(uint32_t st)
 {
     switch (st) {
     case 0:  return UIOX_NFS_OK;
     case NFS3ERR_PERM:  return UIOX_NFS_ERR_PERM;
     case NFS3ERR_NOENT: return UIOX_NFS_ERR_NOENT;
     case NFS3ERR_IO:    return UIOX_NFS_ERR_IO;
     case NFS3ERR_ACCES: return UIOX_NFS_ERR_ACCES;
     case NFS3ERR_STALE: return UIOX_NFS_ERR_STALE;
     default:            return UIOX_NFS_ERR_IO;
     }
 }
 
 /* =========================================================================
  * MOUNT
  * ====================================================================== */
 
 uiox_nfs_err_t uiox_nfs3_mount(uiox_nfs3_ctx_t *ctx,
                                   const uint8_t server_ip[4],
                                   const char *export_path)
 {
     if (!ctx || !export_path) return UIOX_NFS_ERR_INVAL;
     nfs_memset(ctx, 0, sizeof(*ctx));
 
     /* Initialise NFS RPC (port 2049) */
     uiox_rpc_init(&ctx->rpc, server_ip, 2049u, UIOX_RPC_UDP);
     /* Mount RPC (port 635 standard, but QEMU uses 2049 for both) */
     uiox_rpc_init(&ctx->mount_rpc, server_ip, 635u, UIOX_RPC_UDP);
 
     nfs_strncpy(ctx->export_path, export_path, UIOX_NFS_PATH_MAX);
 
     /* Build MOUNTPROC3_MNT args */
     uint8_t  args[512];
     uiox_xdr_t enc;
     uiox_xdr_init_enc(&enc, args, sizeof(args));
     uiox_xdr_enc_str(&enc, export_path);
 
     uint8_t *reply = NULL; uint32_t reply_len = 0u;
     uiox_nfs_err_t rc = uiox_rpc_call(&ctx->mount_rpc,
                                          NFS_MOUNT_PROG, NFS_MOUNT_VERS3,
                                          MOUNTPROC3_MNT,
                                          args, enc.pos,
                                          &reply, &reply_len);
     if (rc != UIOX_NFS_OK) return rc;
 
     uiox_xdr_t dec;
     uiox_xdr_init_dec(&dec, reply, reply_len);
     uint32_t status = uiox_xdr_dec_u32(&dec);
     rc = nfs3_status(status);
     if (rc != UIOX_NFS_OK) return rc;
 
     uiox_xdr_dec_fh(&dec, &ctx->root_fh);
     ctx->mounted = true;
 
     uiox_fw_printf("[NFS3] mounted %u.%u.%u.%u:%s  root_fh_len=%u\n",
                     server_ip[0], server_ip[1], server_ip[2], server_ip[3],
                     export_path, ctx->root_fh.len);
     return UIOX_NFS_OK;
 }
 
 uiox_nfs_err_t uiox_nfs3_umount(uiox_nfs3_ctx_t *ctx)
 {
     if (!ctx || !ctx->mounted) return UIOX_NFS_ERR_INVAL;
     uint8_t args[512]; uiox_xdr_t enc;
     uiox_xdr_init_enc(&enc, args, sizeof(args));
     uiox_xdr_enc_str(&enc, ctx->export_path);
     uint8_t *reply=NULL; uint32_t reply_len=0u;
     uiox_rpc_call(&ctx->mount_rpc,
                    NFS_MOUNT_PROG, NFS_MOUNT_VERS3, MOUNTPROC3_UMNT,
                    args, enc.pos, &reply, &reply_len);
     ctx->mounted = false;
     return UIOX_NFS_OK;
 }
 
 /* =========================================================================
  * GETATTR
  * ====================================================================== */
 
 uiox_nfs_err_t uiox_nfs3_getattr(uiox_nfs3_ctx_t *ctx,
                                     const uiox_nfs_fh_t *fh,
                                     uiox_nfs_attr_t *attr)
 {
     if (!ctx || !fh || !attr) return UIOX_NFS_ERR_INVAL;
     uint8_t args[256]; uiox_xdr_t enc;
     uiox_xdr_init_enc(&enc, args, sizeof(args));
     uiox_xdr_enc_fh(&enc, fh);
     uint8_t *reply=NULL; uint32_t rl=0u;
     uiox_nfs_err_t rc = uiox_rpc_call(&ctx->rpc,
         NFS_PROG, NFS_VERS3, NFSPROC3_GETATTR,
         args, enc.pos, &reply, &rl);
     if (rc != UIOX_NFS_OK) return rc;
     uiox_xdr_t dec; uiox_xdr_init_dec(&dec, reply, rl);
     rc = nfs3_status(uiox_xdr_dec_u32(&dec));
     if (rc != UIOX_NFS_OK) return rc;
     uiox_xdr_dec_attr(&dec, attr);
     ctx->getattr_calls++;
     return UIOX_NFS_OK;
 }
 
 /* =========================================================================
  * LOOKUP
  * ====================================================================== */
 
 uiox_nfs_err_t uiox_nfs3_lookup(uiox_nfs3_ctx_t *ctx,
                                    const uiox_nfs_fh_t *dir_fh,
                                    const char *name,
                                    uiox_nfs_fh_t *out_fh,
                                    uiox_nfs_attr_t *out_attr)
 {
     if (!ctx || !dir_fh || !name || !out_fh) return UIOX_NFS_ERR_INVAL;
     uint8_t args[512]; uiox_xdr_t enc;
     uiox_xdr_init_enc(&enc, args, sizeof(args));
     uiox_xdr_enc_fh(&enc, dir_fh);
     uiox_xdr_enc_str(&enc, name);
     uint8_t *reply=NULL; uint32_t rl=0u;
     uiox_nfs_err_t rc = uiox_rpc_call(&ctx->rpc,
         NFS_PROG, NFS_VERS3, NFSPROC3_LOOKUP,
         args, enc.pos, &reply, &rl);
     if (rc != UIOX_NFS_OK) return rc;
     uiox_xdr_t dec; uiox_xdr_init_dec(&dec, reply, rl);
     rc = nfs3_status(uiox_xdr_dec_u32(&dec));
     if (rc != UIOX_NFS_OK) return rc;
     uiox_xdr_dec_fh(&dec, out_fh);
     if (out_attr) {
         if (uiox_xdr_dec_u32(&dec))  /* post_op_attr present? */
             uiox_xdr_dec_attr(&dec, out_attr);
         else
             nfs_memset(out_attr, 0, sizeof(*out_attr));
     }
     ctx->lookup_calls++;
     return UIOX_NFS_OK;
 }
 
 /* =========================================================================
  * READ
  * ====================================================================== */
 
 uiox_nfs_err_t uiox_nfs3_read(uiox_nfs3_ctx_t *ctx,
                                  const uiox_nfs_fh_t *fh,
                                  uint64_t offset,
                                  uint8_t *buf, uint32_t count,
                                  uint32_t *bytes_read, bool *eof)
 {
     if (!ctx || !fh || !buf) return UIOX_NFS_ERR_INVAL;
     uint8_t args[512]; uiox_xdr_t enc;
     uiox_xdr_init_enc(&enc, args, sizeof(args));
     uiox_xdr_enc_fh(&enc, fh);
     uiox_xdr_enc_u64(&enc, offset);
     uiox_xdr_enc_u32(&enc, count);
     uint8_t *reply=NULL; uint32_t rl=0u;
     uiox_nfs_err_t rc = uiox_rpc_call(&ctx->rpc,
         NFS_PROG, NFS_VERS3, NFSPROC3_READ,
         args, enc.pos, &reply, &rl);
     if (rc != UIOX_NFS_OK) return rc;
     uiox_xdr_t dec; uiox_xdr_init_dec(&dec, reply, rl);
     rc = nfs3_status(uiox_xdr_dec_u32(&dec));
     if (rc != UIOX_NFS_OK) return rc;
     /* post_op_attr */
     if (uiox_xdr_dec_u32(&dec)) uiox_xdr_skip(&dec, 84u);
     uint32_t cnt  = uiox_xdr_dec_u32(&dec);
     uint32_t eof_ = uiox_xdr_dec_u32(&dec);
     /* data length + bytes */
     uint32_t data_len = uiox_xdr_dec_u32(&dec);
     uint32_t copy     = UIOX_NFS_MIN(data_len, count);
     if (dec.pos + copy <= rl)
         nfs_memcpy(buf, reply + dec.pos, copy);
     if (bytes_read) *bytes_read = cnt;
     if (eof)        *eof        = !!eof_;
     ctx->bytes_read += cnt;
     ctx->read_calls++;
     return UIOX_NFS_OK;
 }
 
 /* =========================================================================
  * WRITE
  * ====================================================================== */
 
 uiox_nfs_err_t uiox_nfs3_write(uiox_nfs3_ctx_t *ctx,
                                   const uiox_nfs_fh_t *fh,
                                   uint64_t offset,
                                   const uint8_t *buf, uint32_t count,
                                   uint32_t *bytes_written)
 {
     if (!ctx || !fh || !buf) return UIOX_NFS_ERR_INVAL;
     uint8_t args[UIOX_RPC_BUF_SIZE / 2u]; uiox_xdr_t enc;
     uiox_xdr_init_enc(&enc, args, sizeof(args));
     uiox_xdr_enc_fh(&enc, fh);
     uiox_xdr_enc_u64(&enc, offset);
     uiox_xdr_enc_u32(&enc, count);
     /* stable: 0=UNSTABLE, 1=DATA_SYNC, 2=FILE_SYNC */
     uiox_xdr_enc_u32(&enc, ctx->unstable_write ? 0u : 2u);
     uiox_xdr_enc_bytes(&enc, buf, count);
     uint8_t *reply=NULL; uint32_t rl=0u;
     uiox_nfs_err_t rc = uiox_rpc_call(&ctx->rpc,
         NFS_PROG, NFS_VERS3, NFSPROC3_WRITE,
         args, enc.pos, &reply, &rl);
     if (rc != UIOX_NFS_OK) return rc;
     uiox_xdr_t dec; uiox_xdr_init_dec(&dec, reply, rl);
     rc = nfs3_status(uiox_xdr_dec_u32(&dec));
     if (rc != UIOX_NFS_OK) return rc;
     /* wcc_data (skip) */
     if (uiox_xdr_dec_u32(&dec)) uiox_xdr_skip(&dec, 32u);
     if (uiox_xdr_dec_u32(&dec)) uiox_xdr_skip(&dec, 84u);
     uint32_t cnt = uiox_xdr_dec_u32(&dec);
     if (bytes_written) *bytes_written = cnt;
     ctx->bytes_written += cnt;
     ctx->write_calls++;
     return UIOX_NFS_OK;
 }
 
 /* =========================================================================
  * READDIR
  * ====================================================================== */
 
 uiox_nfs_err_t uiox_nfs3_readdir(uiox_nfs3_ctx_t *ctx,
                                     const uiox_nfs_fh_t *dir_fh,
                                     uiox_nfs3_readdir_cb_t cb, void *priv)
 {
     if (!ctx || !dir_fh || !cb) return UIOX_NFS_ERR_INVAL;
     uint64_t cookie = 0u;
     uint8_t  cookieverf[8] = {0};
 
     for (;;) {
         uint8_t args[512]; uiox_xdr_t enc;
         uiox_xdr_init_enc(&enc, args, sizeof(args));
         uiox_xdr_enc_fh(&enc, dir_fh);
         uiox_xdr_enc_u64(&enc, cookie);
         for (int i=0;i<8;i++) uiox_xdr_enc_u32(&enc, cookieverf[i>>2]);
         uiox_xdr_enc_u32(&enc, UIOX_NFS_READDIR_BUF);
         uint8_t *reply=NULL; uint32_t rl=0u;
         uiox_nfs_err_t rc = uiox_rpc_call(&ctx->rpc,
             NFS_PROG, NFS_VERS3, NFSPROC3_READDIR,
             args, enc.pos, &reply, &rl);
         if (rc != UIOX_NFS_OK) return rc;
         uiox_xdr_t dec; uiox_xdr_init_dec(&dec, reply, rl);
         rc = nfs3_status(uiox_xdr_dec_u32(&dec));
         if (rc != UIOX_NFS_OK) return rc;
         /* post_op_attr + cookieverf */
         if (uiox_xdr_dec_u32(&dec)) uiox_xdr_skip(&dec, 84u);
         for (int i=0;i<2;i++) uiox_xdr_dec_u32(&dec); /* cookieverf */
 
         bool eof = false;
         while (!dec.error) {
             uint32_t present = uiox_xdr_dec_u32(&dec);
             if (!present) {
                 eof = !!uiox_xdr_dec_u32(&dec);
                 break;
             }
             uiox_nfs_dirent_t de;
             de.fileid = uiox_xdr_dec_u64(&dec);
             uiox_xdr_dec_str(&dec, de.name, UIOX_NFS_NAME_MAX + 1u);
             de.cookie = uiox_xdr_dec_u64(&dec);
             cookie = de.cookie;
             if (!cb(&de, priv)) return UIOX_NFS_OK;
         }
         if (eof || dec.error) break;
     }
     return UIOX_NFS_OK;
 }
 
 /* =========================================================================
  * Path walk
  * ====================================================================== */
 
 uiox_nfs_err_t uiox_nfs3_path_lookup(uiox_nfs3_ctx_t *ctx,
                                         const char *path,
                                         uiox_nfs_fh_t *out_fh,
                                         uiox_nfs_attr_t *out_attr)
 {
     if (!ctx || !path || !out_fh) return UIOX_NFS_ERR_INVAL;
     /* Start from root */
     nfs_memcpy(out_fh, &ctx->root_fh, sizeof(*out_fh));
 
     /* Skip leading slash */
     const char *p = path;
     if (*p == '/') p++;
     if (*p == '\0') {
         if (out_attr)
             return uiox_nfs3_getattr(ctx, out_fh, out_attr);
         return UIOX_NFS_OK;
     }
 
     char component[UIOX_NFS_NAME_MAX + 1u];
     while (*p) {
         /* Extract next component */
         int i = 0;
         while (*p && *p != '/' && i < (int)UIOX_NFS_NAME_MAX)
             component[i++] = *p++;
         component[i] = '\0';
         if (*p == '/') p++;
 
         uiox_nfs_fh_t   next_fh;
         uiox_nfs_attr_t next_attr;
         uiox_nfs_err_t rc = uiox_nfs3_lookup(ctx, out_fh, component,
                                                &next_fh, &next_attr);
         if (rc != UIOX_NFS_OK) return rc;
         nfs_memcpy(out_fh, &next_fh, sizeof(*out_fh));
         if (out_attr) nfs_memcpy(out_attr, &next_attr, sizeof(*out_attr));
     }
     return UIOX_NFS_OK;
 }
 
 /* ── Remaining procedures (fsstat, create, mkdir, remove, rename, commit)
  * all follow the same XDR encode → rpc_call → XDR decode pattern.  ── */
 
 uiox_nfs_err_t uiox_nfs3_fsstat(uiox_nfs3_ctx_t *ctx,
                                    const uiox_nfs_fh_t *fh,
                                    uiox_nfs_statfs_t *stat)
 {
     if (!ctx||!fh||!stat) return UIOX_NFS_ERR_INVAL;
     uint8_t args[256]; uiox_xdr_t enc;
     uiox_xdr_init_enc(&enc,args,sizeof(args));
     uiox_xdr_enc_fh(&enc,fh);
     uint8_t *reply=NULL; uint32_t rl=0u;
     uiox_nfs_err_t rc=uiox_rpc_call(&ctx->rpc,
         NFS_PROG,NFS_VERS3,NFSPROC3_FSSTAT,args,enc.pos,&reply,&rl);
     if(rc!=UIOX_NFS_OK)return rc;
     uiox_xdr_t dec; uiox_xdr_init_dec(&dec,reply,rl);
     rc=nfs3_status(uiox_xdr_dec_u32(&dec));
     if(rc!=UIOX_NFS_OK)return rc;
     if(uiox_xdr_dec_u32(&dec))uiox_xdr_skip(&dec,84u);
     stat->tbytes=uiox_xdr_dec_u64(&dec);
     stat->fbytes=uiox_xdr_dec_u64(&dec);
     stat->abytes=uiox_xdr_dec_u64(&dec);
     stat->tfiles=uiox_xdr_dec_u64(&dec);
     stat->ffiles=uiox_xdr_dec_u64(&dec);
     stat->invarsec=uiox_xdr_dec_u32(&dec);
     return UIOX_NFS_OK;
 }
 
 uiox_nfs_err_t uiox_nfs3_create(uiox_nfs3_ctx_t *ctx,
                                    const uiox_nfs_fh_t *dir_fh,
                                    const char *name, uint32_t mode,
                                    uiox_nfs_fh_t *out_fh)
 {
     if(!ctx||!dir_fh||!name||!out_fh)return UIOX_NFS_ERR_INVAL;
     uint8_t args[512]; uiox_xdr_t enc;
     uiox_xdr_init_enc(&enc,args,sizeof(args));
     uiox_xdr_enc_fh(&enc,dir_fh);
     uiox_xdr_enc_str(&enc,name);
     uiox_xdr_enc_u32(&enc,0u);  /* UNCHECKED create */
     uiox_xdr_enc_u32(&enc,1u);  /* mode present    */
     uiox_xdr_enc_u32(&enc,mode);
     uint8_t *reply=NULL; uint32_t rl=0u;
     uiox_nfs_err_t rc=uiox_rpc_call(&ctx->rpc,
         NFS_PROG,NFS_VERS3,NFSPROC3_CREATE,args,enc.pos,&reply,&rl);
     if(rc!=UIOX_NFS_OK)return rc;
     uiox_xdr_t dec; uiox_xdr_init_dec(&dec,reply,rl);
     rc=nfs3_status(uiox_xdr_dec_u32(&dec));
     if(rc!=UIOX_NFS_OK)return rc;
     if(uiox_xdr_dec_u32(&dec))uiox_xdr_dec_fh(&dec,out_fh);
     return UIOX_NFS_OK;
 }
 
 uiox_nfs_err_t uiox_nfs3_remove(uiox_nfs3_ctx_t *ctx,
                                    const uiox_nfs_fh_t *dir_fh,
                                    const char *name)
 {
     if(!ctx||!dir_fh||!name)return UIOX_NFS_ERR_INVAL;
     uint8_t args[512]; uiox_xdr_t enc;
     uiox_xdr_init_enc(&enc,args,sizeof(args));
     uiox_xdr_enc_fh(&enc,dir_fh);
     uiox_xdr_enc_str(&enc,name);
     uint8_t *reply=NULL; uint32_t rl=0u;
     uiox_nfs_err_t rc=uiox_rpc_call(&ctx->rpc,
         NFS_PROG,NFS_VERS3,NFSPROC3_REMOVE,args,enc.pos,&reply,&rl);
     if(rc!=UIOX_NFS_OK)return rc;
     uiox_xdr_t dec; uiox_xdr_init_dec(&dec,reply,rl);
     return nfs3_status(uiox_xdr_dec_u32(&dec));
 }
 
 uiox_nfs_err_t uiox_nfs3_commit(uiox_nfs3_ctx_t *ctx,
                                    const uiox_nfs_fh_t *fh,
                                    uint64_t offset, uint32_t count)
 {
     if(!ctx||!fh)return UIOX_NFS_ERR_INVAL;
     uint8_t args[256]; uiox_xdr_t enc;
     uiox_xdr_init_enc(&enc,args,sizeof(args));
     uiox_xdr_enc_fh(&enc,fh);
     uiox_xdr_enc_u64(&enc,offset);
     uiox_xdr_enc_u32(&enc,count);
     uint8_t *reply=NULL; uint32_t rl=0u;
     uiox_nfs_err_t rc=uiox_rpc_call(&ctx->rpc,
         NFS_PROG,NFS_VERS3,NFSPROC3_COMMIT,args,enc.pos,&reply,&rl);
     if(rc!=UIOX_NFS_OK)return rc;
     uiox_xdr_t dec; uiox_xdr_init_dec(&dec,reply,rl);
     return nfs3_status(uiox_xdr_dec_u32(&dec));
 }
 