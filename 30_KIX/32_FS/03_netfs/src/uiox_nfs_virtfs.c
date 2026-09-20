/**
 * @file  uiox_nfs_virtfs.c
 * @brief UIOX NFS — VirtIO-FS (FUSE over VirtIO MMIO). No libc.
 * @date  2026-07-07
 */

 #include "../include/uiox_nfs_virtfs.h"

 static void vf_memset(void *d,int v,size_t n)
 { uint8_t *p=(uint8_t*)d;while(n--)*p++=(uint8_t)v; }
 static void vf_memcpy(void *d,const void *s,size_t n)
 { uint8_t *dp=(uint8_t*)d;const uint8_t *sp=(const uint8_t*)s;while(n--)*dp++=*sp++; }
 static size_t vf_strlen(const char *s){size_t n=0;while(*s++)n++;return n;}
 
 static inline void vw(uintptr_t b,uint32_t o,uint32_t v){*((volatile uint32_t*)(b+o))=v;}
 static inline uint32_t vr(uintptr_t b,uint32_t o){return *((volatile uint32_t*)(b+o));}
 
 /* ── Build a FUSE in_header ────────────────────────────────── */
 static uint32_t fuse_hdr(uint8_t *buf, uint32_t opcode,
                            uint64_t unique, uint64_t nodeid,
                            uint32_t extra_len)
 {
     uiox_fuse_in_hdr_t *h = (uiox_fuse_in_hdr_t *)buf;
     h->len     = (uint32_t)sizeof(*h) + extra_len;
     h->opcode  = opcode;
     h->unique  = unique;
     h->nodeid  = nodeid;
     h->uid     = 0u;
     h->gid     = 0u;
     h->pid     = 1u;
     h->padding = 0u;
     return (uint32_t)sizeof(*h);
 }
 
 /* ── Submit FUSE request via VirtIO notification (simplified) ─ */
 static uiox_nfs_err_t fuse_submit(uiox_virtfs_ctx_t *ctx,
                                     uint32_t req_len)
 {
     /* Ring TX doorbell queue 0 */
     vw(ctx->mmio_base, VIRTIO_FS_MMIO_QUEUE_NOTIFY, 0u);
     /* Busy-wait for reply (real driver uses virtqueue completion) */
     volatile uint32_t n = 1000000u; while (n--) ;
     ctx->requests++;
     return UIOX_NFS_OK;
 }
 
 /* ── Parse FUSE out_header ─────────────────────────────────── */
 static uiox_nfs_err_t fuse_check_reply(uiox_virtfs_ctx_t *ctx,
                                          uint32_t *payload_off)
 {
     uiox_fuse_out_hdr_t *oh = (uiox_fuse_out_hdr_t *)ctx->rx_buf;
     if (oh->error != 0) { ctx->errors++; return UIOX_NFS_ERR_IO; }
     *payload_off = (uint32_t)sizeof(*oh);
     return UIOX_NFS_OK;
 }
 
 uiox_nfs_err_t uiox_virtfs_init(uiox_virtfs_ctx_t *ctx,
                                    uintptr_t mmio_base)
 {
     vf_memset(ctx, 0, sizeof(*ctx));
     ctx->mmio_base  = mmio_base;
     ctx->root_nodeid = 1u;
 
     if (!mmio_base) {
         /* Simulation mode — no real device */
         ctx->initialized = true;
         return UIOX_NFS_OK;
     }
 
     /* Verify VirtIO magic */
     if (vr(mmio_base, VIRTIO_FS_MMIO_MAGIC) != 0x74726976u)
         return UIOX_NFS_ERR_NODEV;
     if (vr(mmio_base, VIRTIO_FS_MMIO_DEVICE_ID) != VIRTIO_FS_DEVICE_ID)
         return UIOX_NFS_ERR_NODEV;
 
     /* Device negotiation */
     vw(mmio_base, VIRTIO_FS_MMIO_STATUS, VIRTIO_FS_STATUS_ACK);
     vw(mmio_base, VIRTIO_FS_MMIO_STATUS,
        VIRTIO_FS_STATUS_ACK | VIRTIO_FS_STATUS_DRIVER);
     vw(mmio_base, VIRTIO_FS_MMIO_STATUS,
        VIRTIO_FS_STATUS_ACK | VIRTIO_FS_STATUS_DRIVER |
        VIRTIO_FS_STATUS_FEATURES_OK | VIRTIO_FS_STATUS_DRIVER_OK);
 
     /* Read tag from config space */
     const uint8_t *cfg = (const uint8_t *)(mmio_base + VIRTIO_FS_MMIO_CONFIG);
     for (int i=0;i<35;i++) ctx->tag[i] = (char)cfg[i];
     ctx->tag[35] = '\0';
 
     /* FUSE INIT */
     uint32_t pos = fuse_hdr(ctx->tx_buf, FUSE_INIT,
                               ++ctx->unique_counter, 0u, 24u);
     uint32_t *init = (uint32_t *)(ctx->tx_buf + pos);
     init[0] = FUSE_KERNEL_VERSION;
     init[1] = FUSE_KERNEL_MINOR_VERSION;
     init[2] = 0u;   /* max_readahead */
     init[3] = FUSE_ASYNC_READ | FUSE_BIG_WRITES | FUSE_WRITEBACK_CACHE;
     init[4] = 0u;
     init[5] = 0u;
     fuse_submit(ctx, pos + 24u);
 
     ctx->initialized = true;
     return UIOX_NFS_OK;
 }
 
 void uiox_virtfs_deinit(uiox_virtfs_ctx_t *ctx)
 {
     if (!ctx) return;
     if (ctx->mmio_base)
         vw(ctx->mmio_base, VIRTIO_FS_MMIO_STATUS, 0u);  /* reset */
     ctx->initialized = false;
 }
 
 /* ── FUSE_LOOKUP ────────────────────────────────────────────── */
 
 uiox_nfs_err_t uiox_virtfs_lookup(uiox_virtfs_ctx_t *ctx,
                                      uint64_t parent, const char *name,
                                      uint64_t *nodeid,
                                      uiox_nfs_attr_t *attr)
 {
     if (!ctx || !name) return UIOX_NFS_ERR_INVAL;
     size_t nlen = vf_strlen(name) + 1u;
     uint32_t pos = fuse_hdr(ctx->tx_buf, FUSE_LOOKUP,
                               ++ctx->unique_counter, parent,
                               (uint32_t)nlen);
     vf_memcpy(ctx->tx_buf + pos, name, nlen);
     uiox_nfs_err_t rc = fuse_submit(ctx, pos + (uint32_t)nlen);
     if (rc != UIOX_NFS_OK) return rc;
     uint32_t poff = 0u;
     rc = fuse_check_reply(ctx, &poff);
     if (rc != UIOX_NFS_OK) return rc;
     /* entry_out: nodeid[8] generation[8] attr_valid[8] ... attr[88] */
     const uint8_t *e = ctx->rx_buf + poff;
     if (nodeid) *nodeid = *(uint64_t *)e;
     if (attr) {
         /* Parse fuse_attr at offset 24 of entry_out */
         const uint32_t *a = (const uint32_t *)(e + 24u);
         attr->fileid = *(uint64_t *)(e + 24u);
         attr->size   = *(uint64_t *)(e + 32u);
         attr->mode   = a[10];
         attr->nlink  = a[9];
         attr->valid  = true;
     }
     return UIOX_NFS_OK;
 }
 
 uiox_nfs_err_t uiox_virtfs_getattr(uiox_virtfs_ctx_t *ctx,
                                       uint64_t nodeid,
                                       uiox_nfs_attr_t *attr)
 {
     if (!ctx||!attr) return UIOX_NFS_ERR_INVAL;
     uint32_t pos = fuse_hdr(ctx->tx_buf, FUSE_GETATTR,
                               ++ctx->unique_counter, nodeid, 16u);
     vf_memset(ctx->tx_buf + pos, 0, 16u);  /* getattr_in: flags, fh */
     uiox_nfs_err_t rc = fuse_submit(ctx, pos + 16u);
     if (rc != UIOX_NFS_OK) return rc;
     uint32_t poff = 0u;
     rc = fuse_check_reply(ctx, &poff);
     if (rc != UIOX_NFS_OK) return rc;
     /* attr_out: attr_valid[8] attr[88] */
     const uint8_t *a = ctx->rx_buf + poff + 8u;
     attr->fileid = *(uint64_t *)a;
     attr->size   = *(uint64_t *)(a + 8u);
     attr->mode   = *(uint32_t *)(a + 40u);
     attr->valid  = true;
     return UIOX_NFS_OK;
 }
 
 uiox_nfs_err_t uiox_virtfs_open(uiox_virtfs_ctx_t *ctx,
                                    uint64_t nodeid, uint32_t flags,
                                    uint64_t *fh)
 {
     if (!ctx) return UIOX_NFS_ERR_INVAL;
     uint32_t pos = fuse_hdr(ctx->tx_buf, FUSE_OPEN,
                               ++ctx->unique_counter, nodeid, 8u);
     *(uint32_t *)(ctx->tx_buf + pos)     = flags;
     *(uint32_t *)(ctx->tx_buf + pos + 4u)= 0u;
     uiox_nfs_err_t rc = fuse_submit(ctx, pos + 8u);
     if (rc != UIOX_NFS_OK) return rc;
     uint32_t poff = 0u;
     rc = fuse_check_reply(ctx, &poff);
     if (rc != UIOX_NFS_OK) return rc;
     if (fh) *fh = *(uint64_t *)(ctx->rx_buf + poff);
     return UIOX_NFS_OK;
 }
 
 uiox_nfs_err_t uiox_virtfs_read(uiox_virtfs_ctx_t *ctx,
                                    uint64_t fh, uint64_t nodeid,
                                    uint64_t offset,
                                    uint8_t *buf, uint32_t size,
                                    uint32_t *bytes_read)
 {
     if (!ctx||!buf) return UIOX_NFS_ERR_INVAL;
     uint32_t pos = fuse_hdr(ctx->tx_buf, FUSE_READ,
                               ++ctx->unique_counter, nodeid, 40u);
     uint8_t *ri = ctx->tx_buf + pos;
     *(uint64_t *)ri        = fh;
     *(uint64_t *)(ri + 8u) = offset;
     *(uint32_t *)(ri + 16u)= size;
     vf_memset(ri + 20u, 0, 20u);
     uiox_nfs_err_t rc = fuse_submit(ctx, pos + 40u);
     if (rc != UIOX_NFS_OK) return rc;
     uint32_t poff = 0u;
     rc = fuse_check_reply(ctx, &poff);
     if (rc != UIOX_NFS_OK) return rc;
     uint32_t got = ((uiox_fuse_out_hdr_t *)ctx->rx_buf)->len -
                    (uint32_t)sizeof(uiox_fuse_out_hdr_t);
     vf_memcpy(buf, ctx->rx_buf + poff, got < size ? got : size);
     if (bytes_read) *bytes_read = got;
     ctx->bytes_read += got;
     return UIOX_NFS_OK;
 }
 
 uiox_nfs_err_t uiox_virtfs_write(uiox_virtfs_ctx_t *ctx,
                                     uint64_t fh, uint64_t nodeid,
                                     uint64_t offset,
                                     const uint8_t *buf, uint32_t size,
                                     uint32_t *bytes_written)
 {
     if (!ctx||!buf) return UIOX_NFS_ERR_INVAL;
     uint32_t pos = fuse_hdr(ctx->tx_buf, FUSE_WRITE,
                               ++ctx->unique_counter, nodeid, 64u);
     uint8_t *wi = ctx->tx_buf + pos;
     *(uint64_t *)wi        = fh;
     *(uint64_t *)(wi + 8u) = offset;
     *(uint32_t *)(wi + 16u)= size;
     vf_memset(wi + 20u, 0, 44u);
     vf_memcpy(ctx->tx_buf + pos + 64u, buf, size);
     uiox_nfs_err_t rc = fuse_submit(ctx, pos + 64u + size);
     if (rc != UIOX_NFS_OK) return rc;
     uint32_t poff = 0u;
     rc = fuse_check_reply(ctx, &poff);
     if (rc != UIOX_NFS_OK) return rc;
     uint32_t written = *(uint32_t *)(ctx->rx_buf + poff);
     if (bytes_written) *bytes_written = written;
     ctx->bytes_written += written;
     return UIOX_NFS_OK;
 }
 
 uiox_nfs_err_t uiox_virtfs_release(uiox_virtfs_ctx_t *ctx,
                                       uint64_t fh, uint64_t nodeid)
 {
     uint32_t pos = fuse_hdr(ctx->tx_buf, FUSE_RELEASE,
                               ++ctx->unique_counter, nodeid, 24u);
     *(uint64_t *)(ctx->tx_buf + pos) = fh;
     return fuse_submit(ctx, pos + 24u);
 }
 
 uiox_nfs_err_t uiox_virtfs_mkdir(uiox_virtfs_ctx_t *ctx,
                                     uint64_t parent, const char *name,
                                     uint32_t mode, uint64_t *nodeid)
 {
     if (!ctx||!name) return UIOX_NFS_ERR_INVAL;
     size_t nlen = vf_strlen(name) + 1u;
     uint32_t pos = fuse_hdr(ctx->tx_buf, FUSE_MKDIR,
                               ++ctx->unique_counter, parent,
                               8u + (uint32_t)nlen);
     *(uint32_t *)(ctx->tx_buf + pos)     = mode;
     *(uint32_t *)(ctx->tx_buf + pos + 4u)= 0u;
     vf_memcpy(ctx->tx_buf + pos + 8u, name, nlen);
     uiox_nfs_err_t rc = fuse_submit(ctx, pos + 8u + (uint32_t)nlen);
     if (rc != UIOX_NFS_OK) return rc;
     uint32_t poff = 0u;
     rc = fuse_check_reply(ctx, &poff);
     if (rc != UIOX_NFS_OK) return rc;
     if (nodeid) *nodeid = *(uint64_t *)(ctx->rx_buf + poff);
     return UIOX_NFS_OK;
 }
 
 uiox_nfs_err_t uiox_virtfs_unlink(uiox_virtfs_ctx_t *ctx,
                                      uint64_t parent, const char *name)
 {
     if (!ctx||!name) return UIOX_NFS_ERR_INVAL;
     size_t nlen = vf_strlen(name) + 1u;
     uint32_t pos = fuse_hdr(ctx->tx_buf, FUSE_UNLINK,
                               ++ctx->unique_counter, parent,
                               (uint32_t)nlen);
     vf_memcpy(ctx->tx_buf + pos, name, nlen);
     return fuse_submit(ctx, pos + (uint32_t)nlen);
 }
 
 uiox_nfs_err_t uiox_virtfs_readdir(uiox_virtfs_ctx_t *ctx,
                                       uint64_t nodeid, uint64_t fh,
                                       uint64_t offset,
                                       uiox_nfs3_readdir_cb_t cb, void *priv)
 {
     if (!ctx||!cb) return UIOX_NFS_ERR_INVAL;
     uint32_t pos = fuse_hdr(ctx->tx_buf, FUSE_READDIR,
                               ++ctx->unique_counter, nodeid, 40u);
     uint8_t *ri = ctx->tx_buf + pos;
     *(uint64_t *)ri         = fh;
     *(uint64_t *)(ri + 8u)  = offset;
     *(uint32_t *)(ri + 16u) = UIOX_NFS_READDIR_BUF;
     vf_memset(ri + 20u, 0, 20u);
     uiox_nfs_err_t rc = fuse_submit(ctx, pos + 40u);
     if (rc != UIOX_NFS_OK) return rc;
     uint32_t poff = 0u;
     rc = fuse_check_reply(ctx, &poff);
     if (rc != UIOX_NFS_OK) return rc;
     /* Parse dirent stream */
     uint32_t data_len = ((uiox_fuse_out_hdr_t *)ctx->rx_buf)->len -
                         (uint32_t)sizeof(uiox_fuse_out_hdr_t);
     const uint8_t *p = ctx->rx_buf + poff;
     const uint8_t *end = p + data_len;
     while (p < end) {
         uiox_nfs_dirent_t de;
         de.fileid = *(uint64_t *)p;
         de.cookie = *(uint64_t *)(p + 8u);
         uint32_t namelen = *(uint32_t *)(p + 20u);
         uint32_t copy = namelen < UIOX_NFS_NAME_MAX ? namelen : UIOX_NFS_NAME_MAX;
         vf_memcpy(de.name, p + 24u, copy);
         de.name[copy] = '\0';
         if (!cb(&de, priv)) break;
         /* Advance: dirent header=24 + namelen, aligned to 8 */
         uint32_t step = 24u + ((namelen + 7u) & ~7u);
         p += step;
     }
     return UIOX_NFS_OK;
 }
 
 uiox_nfs_err_t uiox_virtfs_statfs(uiox_virtfs_ctx_t *ctx,
                                      uint64_t nodeid,
                                      uiox_nfs_statfs_t *stat)
 {
     if (!ctx||!stat) return UIOX_NFS_ERR_INVAL;
     uint32_t pos = fuse_hdr(ctx->tx_buf, FUSE_STATFS,
                               ++ctx->unique_counter, nodeid, 0u);
     uiox_nfs_err_t rc = fuse_submit(ctx, pos);
     if (rc != UIOX_NFS_OK) return rc;
     uint32_t poff = 0u;
     rc = fuse_check_reply(ctx, &poff);
     if (rc != UIOX_NFS_OK) return rc;
     const uint64_t *s = (const uint64_t *)(ctx->rx_buf + poff);
     stat->tbytes = s[0] * 512u;
     stat->fbytes = s[1] * 512u;
     stat->abytes = s[2] * 512u;
     stat->tfiles = s[3];
     stat->ffiles = s[4];
     return UIOX_NFS_OK;
 }
 
 uiox_nfs_err_t uiox_virtfs_path_lookup(uiox_virtfs_ctx_t *ctx,
                                           const char *path,
                                           uint64_t *nodeid,
                                           uiox_nfs_attr_t *attr)
 {
     if (!ctx||!path||!nodeid) return UIOX_NFS_ERR_INVAL;
     uint64_t curr = ctx->root_nodeid;
     const char *p = path; if (*p=='/') p++;
     if (*p == '\0') {
         *nodeid = curr;
         if (attr) return uiox_virtfs_getattr(ctx, curr, attr);
         return UIOX_NFS_OK;
     }
     while (*p) {
         char comp[UIOX_NFS_NAME_MAX+1u]; int ci=0;
         while (*p && *p!='/' && ci<(int)UIOX_NFS_NAME_MAX)
             comp[ci++]=*p++;
         comp[ci]='\0'; if (*p=='/') p++;
         uint64_t next = 0u;
         uiox_nfs_attr_t a;
         uiox_nfs_err_t rc = uiox_virtfs_lookup(ctx, curr, comp, &next, &a);
         if (rc != UIOX_NFS_OK) return rc;
         curr = next;
         if (attr) *attr = a;
     }
     *nodeid = curr;
     return UIOX_NFS_OK;
 }
 