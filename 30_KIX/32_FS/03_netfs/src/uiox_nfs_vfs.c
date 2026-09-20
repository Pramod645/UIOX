/**
 * @file  uiox_nfs_vfs.c
 * @brief UIOX NFS — VFS mount layer and file operations. No libc.
 * @date  2026-07-07
 */

 #include "../include/uiox_nfs_vfs.h"

 extern void uiox_fw_printf(const char *fmt, ...);
 
 static void v_memset(void *d,int v,size_t n)
 { uint8_t *p=(uint8_t*)d;while(n--)*p++=(uint8_t)v; }
 static void v_memcpy(void *d,const void *s,size_t n)
 { uint8_t *dp=(uint8_t*)d;const uint8_t *sp=(const uint8_t*)s;while(n--)*dp++=*sp++; }
 static size_t v_strlen(const char *s){size_t n=0;while(*s++)n++;return n;}
 static void v_strncpy(char *d,const char *s,size_t n)
 { size_t i=0;while(i<n-1&&s[i]){d[i]=s[i];i++;}d[i]='\0'; }
 static int v_strncmp(const char *a,const char *b,size_t n)
 { while(n--&&*a&&*b){if(*a!=*b)return (int)(unsigned char)*a-(int)(unsigned char)*b;a++;b++;}return 0; }
 
 /* =========================================================================
  * Global mount table
  * ====================================================================== */
 
 static uiox_nfs_mount_t s_mounts[UIOX_NFS_MAX_MOUNTS];
 static bool              s_vfs_init = false;
 
 uiox_nfs_err_t uiox_nfs_vfs_init(void)
 {
     v_memset(s_mounts, 0, sizeof(s_mounts));
     s_vfs_init = true;
     uiox_fw_printf("[netfs] VFS layer init OK  max_mounts=%u\n",
                     UIOX_NFS_MAX_MOUNTS);
     return UIOX_NFS_OK;
 }
 
 void uiox_nfs_vfs_deinit(void)
 {
     for (uint8_t i=0u;i<UIOX_NFS_MAX_MOUNTS;i++) {
         if (!s_mounts[i].mounted) continue;
         uiox_nfs_vfs_umount(s_mounts[i].mount_point);
     }
     s_vfs_init = false;
 }
 
 /* =========================================================================
  * Mount
  * ====================================================================== */
 
 uiox_nfs_err_t uiox_nfs_vfs_mount(const uiox_nfs_mount_params_t *p,
                                      uint8_t *mount_idx)
 {
     if (!s_vfs_init || !p) return UIOX_NFS_ERR_INVAL;
 
     /* Find free slot */
     uint8_t idx = 0xFFu;
     for (uint8_t i=0u;i<UIOX_NFS_MAX_MOUNTS;i++) {
         if (!s_mounts[i].mounted) { idx=i; break; }
     }
     if (idx == 0xFFu) return UIOX_NFS_ERR_BUSY;
 
     uiox_nfs_mount_t *m = &s_mounts[idx];
     v_memset(m, 0, sizeof(*m));
     v_strncpy(m->mount_point, p->mount_point, UIOX_NFS_PATH_MAX);
     m->type      = p->type;
     m->read_only = p->read_only;
 
     uiox_nfs_err_t rc = UIOX_NFS_ERR_UNSUP;
 
     switch (p->type) {
     case UIOX_NETFS_NFS3:
         /* Wire network callbacks */
         m->nfs3.rpc.send     = p->net_send;
         m->nfs3.rpc.recv     = p->net_recv;
         m->nfs3.rpc.net_ctx  = p->net_ctx;
         m->nfs3.mount_rpc.send    = p->net_send;
         m->nfs3.mount_rpc.recv    = p->net_recv;
         m->nfs3.mount_rpc.net_ctx = p->net_ctx;
         if (p->timeout_ms) m->nfs3.rpc.timeout_ms = p->timeout_ms;
         rc = uiox_nfs3_mount(&m->nfs3, p->server_ip, p->export_path);
         break;
 
     case UIOX_NETFS_9P2000:
         m->p9.send    = p->net_send;
         m->p9.recv    = p->net_recv;
         m->p9.net_ctx = p->net_ctx;
         rc = uiox_9p_connect(&m->p9, p->p9_aname);
         break;
 
     case UIOX_NETFS_VIRTFS:
         rc = uiox_virtfs_init(&m->virtfs, p->virtio_mmio_base);
         break;
 
     default:
         return UIOX_NFS_ERR_UNSUP;
     }
 
     if (rc != UIOX_NFS_OK) return rc;
 
     /* Initialise cache (stub uptime — real kernel wires timer here) */
     uiox_nfs_cache_init(&m->cache, NULL);
 
     m->mounted = true;
     if (mount_idx) *mount_idx = idx;
 
     uiox_fw_printf("[netfs] mounted '%s'  type=%u  slot=%u\n",
                     m->mount_point, (uint32_t)m->type, idx);
     return UIOX_NFS_OK;
 }
 
 uiox_nfs_err_t uiox_nfs_vfs_umount(const char *mount_point)
 {
     if (!mount_point) return UIOX_NFS_ERR_INVAL;
     for (uint8_t i=0u;i<UIOX_NFS_MAX_MOUNTS;i++) {
         uiox_nfs_mount_t *m = &s_mounts[i];
         if (!m->mounted) continue;
         if (v_strncmp(m->mount_point, mount_point, UIOX_NFS_PATH_MAX) != 0)
             continue;
         /* Flush dirty pages before unmounting */
         /* (writeback_fn is NULL in stub — production wires it to protocol) */
         switch (m->type) {
         case UIOX_NETFS_NFS3:
             uiox_nfs3_umount(&m->nfs3);
             break;
         case UIOX_NETFS_9P2000:
             uiox_9p_disconnect(&m->p9);
             break;
         case UIOX_NETFS_VIRTFS:
             uiox_virtfs_deinit(&m->virtfs);
             break;
         default: break;
         }
         m->mounted = false;
         uiox_fw_printf("[netfs] umounted '%s'\n", mount_point);
         return UIOX_NFS_OK;
     }
     return UIOX_NFS_ERR_NOENT;
 }
 
 /* =========================================================================
  * Path resolution
  * ====================================================================== */
 
 uiox_nfs_mount_t *uiox_nfs_vfs_resolve(const char *path,
                                           const char **rel_path)
 {
     if (!path) return NULL;
     uiox_nfs_mount_t *best = NULL;
     size_t            best_len = 0u;
 
     for (uint8_t i=0u;i<UIOX_NFS_MAX_MOUNTS;i++) {
         uiox_nfs_mount_t *m = &s_mounts[i];
         if (!m->mounted) continue;
         size_t mlen = v_strlen(m->mount_point);
         if (v_strncmp(path, m->mount_point, mlen) == 0) {
             if (mlen > best_len) {
                 best     = m;
                 best_len = mlen;
             }
         }
     }
     if (best && rel_path) {
         *rel_path = path + best_len;
         if (**rel_path == '/') (*rel_path)++;
     }
     return best;
 }
 
 /* =========================================================================
  * File descriptor table
  * ====================================================================== */
 
 static uiox_nfs_fd_t *fd_alloc(uiox_nfs_mount_t *m)
 {
     for (uint32_t i=0u;i<UIOX_NFS_MAX_OPEN;i++) {
         if (!m->fds[i].in_use) {
             v_memset(&m->fds[i], 0, sizeof(m->fds[i]));
             m->fds[i].in_use = true;
             m->open_files++;
             return &m->fds[i];
         }
     }
     return NULL;
 }
 
 static int fd_to_global(uiox_nfs_mount_t *m, uiox_nfs_fd_t *fd)
 {
     /* Encode: (mount_idx << 8) | local_fd_idx */
     uint8_t midx = (uint8_t)(m - s_mounts);
     uint32_t lidx = (uint32_t)(fd - m->fds);
     return (int)((midx << 8u) | (lidx & 0xFFu)) + 1000;
 }
 
 static uiox_nfs_fd_t *global_to_fd(int gfd, uiox_nfs_mount_t **mount_out)
 {
     if (gfd < 1000) return NULL;
     gfd -= 1000;
     uint8_t  midx = (uint8_t)((uint32_t)gfd >> 8u);
     uint8_t  lidx = (uint8_t)((uint32_t)gfd & 0xFFu);
     if (midx >= UIOX_NFS_MAX_MOUNTS) return NULL;
     uiox_nfs_mount_t *m = &s_mounts[midx];
     if (!m->mounted || lidx >= UIOX_NFS_MAX_OPEN) return NULL;
     if (!m->fds[lidx].in_use) return NULL;
     if (mount_out) *mount_out = m;
     return &m->fds[lidx];
 }
 
 /* =========================================================================
  * open / close / read / write / lseek
  * ====================================================================== */
 
 int uiox_nfs_vfs_open(const char *path, uint32_t flags, uint32_t mode)
 {
     const char *rel = NULL;
     uiox_nfs_mount_t *m = uiox_nfs_vfs_resolve(path, &rel);
     if (!m) return (int)UIOX_NFS_ERR_NOENT;
 
     uiox_nfs_fh_t   fh;
     uiox_nfs_attr_t attr;
     v_memset(&fh,   0, sizeof(fh));
     v_memset(&attr, 0, sizeof(attr));
     uiox_nfs_err_t rc = UIOX_NFS_ERR_UNSUP;
 
     switch (m->type) {
     case UIOX_NETFS_NFS3:
         /* Check attribute cache first */
         {
             uiox_nfs_attr_t *ca = uiox_nfs_acache_get(
                 &m->cache,
                 uiox_nfs_fh_hash(&m->nfs3.root_fh));
             (void)ca;
         }
         rc = uiox_nfs3_path_lookup(&m->nfs3, rel, &fh, &attr);
         if (rc == UIOX_NFS_ERR_NOENT && (flags & UIOX_NFS_O_CREAT)) {
             /* Find parent dir and create */
             rc = uiox_nfs3_create(&m->nfs3, &m->nfs3.root_fh,
                                     rel, mode ? mode : 0644u, &fh);
         }
         break;
     case UIOX_NETFS_VIRTFS: {
         uint64_t nodeid = 0u;
         rc = uiox_virtfs_path_lookup(&m->virtfs, rel, &nodeid, &attr);
         if (rc == UIOX_NFS_OK) {
             uiox_nfs_fd_t *fd = fd_alloc(m);
             if (!fd) return (int)UIOX_NFS_ERR_BUSY;
             uint64_t vfh = 0u;
             rc = uiox_virtfs_open(&m->virtfs, nodeid, flags, &vfh);
             if (rc != UIOX_NFS_OK) { fd->in_use=false; return (int)rc; }
             fd->nodeid   = nodeid;
             fd->virt_fh  = vfh;
             fd->flags    = flags;
             fd->offset   = 0u;
             fd->mount_idx= (uint8_t)(m - s_mounts);
             return fd_to_global(m, fd);
         }
         break;
     }
     case UIOX_NETFS_9P2000: {
         uint32_t fid = uiox_9p_alloc_fid(&m->p9);
         uint32_t new_fid = 0u;
         rc = uiox_9p_walk(&m->p9, m->p9.root_fid, rel, &new_fid);
         if (rc == UIOX_NFS_OK) {
             rc = uiox_9p_open(&m->p9, new_fid, (uint8_t)(flags & 0x3u));
             if (rc != UIOX_NFS_OK) { uiox_9p_free_fid(&m->p9,new_fid); return (int)rc; }
             uiox_nfs_fd_t *fd = fd_alloc(m);
             if (!fd) { uiox_9p_clunk(&m->p9,new_fid); return (int)UIOX_NFS_ERR_BUSY; }
             fd->p9_fid  = new_fid;
             fd->flags   = flags;
             fd->offset  = 0u;
             fd->mount_idx=(uint8_t)(m - s_mounts);
             return fd_to_global(m, fd);
         }
         (void)fid;
         break;
     }
     default: return (int)UIOX_NFS_ERR_UNSUP;
     }
 
     if (rc != UIOX_NFS_OK) return (int)rc;
 
     uiox_nfs_fd_t *fd = fd_alloc(m);
     if (!fd) return (int)UIOX_NFS_ERR_BUSY;
     v_memcpy(&fd->fh, &fh, sizeof(fh));
     fd->flags    = flags;
     fd->offset   = (flags & UIOX_NFS_O_APPEND) ? attr.size : 0u;
     fd->mount_idx= (uint8_t)(m - s_mounts);
     return fd_to_global(m, fd);
 }
 
 int uiox_nfs_vfs_close(int gfd)
 {
     uiox_nfs_mount_t *m = NULL;
     uiox_nfs_fd_t    *fd = global_to_fd(gfd, &m);
     if (!fd) return (int)UIOX_NFS_ERR_INVAL;
 
     if (m->type == UIOX_NETFS_VIRTFS && fd->virt_fh)
         uiox_virtfs_release(&m->virtfs, fd->virt_fh, fd->nodeid);
     else if (m->type == UIOX_NETFS_9P2000 && fd->p9_fid != P9_NOFID)
         uiox_9p_clunk(&m->p9, fd->p9_fid);
 
     fd->in_use = false;
     m->open_files--;
     return 0;
 }
 
 int uiox_nfs_vfs_read(int gfd, uint8_t *buf, uint32_t count)
 {
     uiox_nfs_mount_t *m = NULL;
     uiox_nfs_fd_t    *fd = global_to_fd(gfd, &m);
     if (!fd || !buf) return (int)UIOX_NFS_ERR_INVAL;
 
     uint32_t got = 0u;
     uiox_nfs_err_t rc = UIOX_NFS_ERR_UNSUP;
 
     switch (m->type) {
     case UIOX_NETFS_NFS3: {
         bool eof = false;
         rc = uiox_nfs3_read(&m->nfs3, &fd->fh, fd->offset,
                               buf, count, &got, &eof);
         break;
     }
     case UIOX_NETFS_VIRTFS:
         rc = uiox_virtfs_read(&m->virtfs, fd->virt_fh, fd->nodeid,
                                fd->offset, buf, count, &got);
         break;
     case UIOX_NETFS_9P2000:
         rc = uiox_9p_read(&m->p9, fd->p9_fid, fd->offset,
                            buf, count, &got);
         break;
     default: return (int)UIOX_NFS_ERR_UNSUP;
     }
 
     if (rc != UIOX_NFS_OK) return (int)rc;
     fd->offset   += got;
     m->bytes_read += got;
     return (int)got;
 }
 
 int uiox_nfs_vfs_write(int gfd, const uint8_t *buf, uint32_t count)
 {
     uiox_nfs_mount_t *m = NULL;
     uiox_nfs_fd_t    *fd = global_to_fd(gfd, &m);
     if (!fd || !buf) return (int)UIOX_NFS_ERR_INVAL;
     if (m->read_only)  return (int)UIOX_NFS_ERR_PERM;
 
     uint32_t written = 0u;
     uiox_nfs_err_t rc = UIOX_NFS_ERR_UNSUP;
 
     switch (m->type) {
     case UIOX_NETFS_NFS3:
         rc = uiox_nfs3_write(&m->nfs3, &fd->fh, fd->offset,
                                buf, count, &written);
         break;
     case UIOX_NETFS_VIRTFS:
         rc = uiox_virtfs_write(&m->virtfs, fd->virt_fh, fd->nodeid,
                                 fd->offset, buf, count, &written);
         break;
     case UIOX_NETFS_9P2000:
         rc = uiox_9p_write(&m->p9, fd->p9_fid, fd->offset,
                             buf, count, &written);
         break;
     default: return (int)UIOX_NFS_ERR_UNSUP;
     }
 
     if (rc != UIOX_NFS_OK) return (int)rc;
     fd->offset        += written;
     m->bytes_written  += written;
     return (int)written;
 }
 
 int uiox_nfs_vfs_lseek(int gfd, int64_t offset, int whence)
 {
     uiox_nfs_mount_t *m  = NULL;
     uiox_nfs_fd_t    *fd = global_to_fd(gfd, &m);
     if (!fd) return (int)UIOX_NFS_ERR_INVAL;
     switch (whence) {
     case 0: fd->offset = (uint64_t)offset; break;           /* SEEK_SET */
     case 1: fd->offset = (uint64_t)((int64_t)fd->offset + offset); break; /* SEEK_CUR */
     case 2: {
         uiox_nfs_attr_t attr;
         v_memset(&attr, 0, sizeof(attr));
         if (m->type == UIOX_NETFS_NFS3)
             uiox_nfs3_getattr(&m->nfs3, &fd->fh, &attr);
         fd->offset = (uint64_t)((int64_t)attr.size + offset);
         break;
     }
     default: return (int)UIOX_NFS_ERR_INVAL;
     }
     return (int)fd->offset;
 }
 
 int uiox_nfs_vfs_stat(const char *path, uiox_nfs_attr_t *attr)
 {
     const char *rel = NULL;
     uiox_nfs_mount_t *m = uiox_nfs_vfs_resolve(path, &rel);
     if (!m) return (int)UIOX_NFS_ERR_NOENT;
     switch (m->type) {
     case UIOX_NETFS_NFS3: {
         uiox_nfs_fh_t fh;
         return (int)uiox_nfs3_path_lookup(&m->nfs3, rel, &fh, attr);
     }
     case UIOX_NETFS_VIRTFS: {
         uint64_t nodeid=0u;
         return (int)uiox_virtfs_path_lookup(&m->virtfs, rel,
                                               &nodeid, attr);
     }
     default: return (int)UIOX_NFS_ERR_UNSUP;
     }
 }
 
 int uiox_nfs_vfs_fstat(int gfd, uiox_nfs_attr_t *attr)
 {
     uiox_nfs_mount_t *m  = NULL;
     uiox_nfs_fd_t    *fd = global_to_fd(gfd, &m);
     if (!fd || !attr) return (int)UIOX_NFS_ERR_INVAL;
     if (m->type == UIOX_NETFS_NFS3)
         return (int)uiox_nfs3_getattr(&m->nfs3, &fd->fh, attr);
     if (m->type == UIOX_NETFS_VIRTFS)
         return (int)uiox_virtfs_getattr(&m->virtfs, fd->nodeid, attr);
     return (int)UIOX_NFS_ERR_UNSUP;
 }
 
 int uiox_nfs_vfs_mkdir(const char *path, uint32_t mode)
 {
     const char *rel = NULL;
     uiox_nfs_mount_t *m = uiox_nfs_vfs_resolve(path, &rel);
     if (!m) return (int)UIOX_NFS_ERR_NOENT;
     if (m->read_only) return (int)UIOX_NFS_ERR_PERM;
     if (m->type == UIOX_NETFS_NFS3) {
         uiox_nfs_fh_t fh;
         return (int)uiox_nfs3_mkdir(&m->nfs3, &m->nfs3.root_fh,
                                       rel, mode, &fh);
     }
     if (m->type == UIOX_NETFS_VIRTFS) {
         uint64_t nodeid=0u;
         return (int)uiox_virtfs_mkdir(&m->virtfs,
                                         m->virtfs.root_nodeid,
                                         rel, mode, &nodeid);
     }
     return (int)UIOX_NFS_ERR_UNSUP;
 }
 
 int uiox_nfs_vfs_unlink(const char *path)
 {
     const char *rel = NULL;
     uiox_nfs_mount_t *m = uiox_nfs_vfs_resolve(path, &rel);
     if (!m) return (int)UIOX_NFS_ERR_NOENT;
     if (m->read_only) return (int)UIOX_NFS_ERR_PERM;
     if (m->type == UIOX_NETFS_NFS3)
         return (int)uiox_nfs3_remove(&m->nfs3,
                                        &m->nfs3.root_fh, rel);
     if (m->type == UIOX_NETFS_VIRTFS)
         return (int)uiox_virtfs_unlink(&m->virtfs,
                                          m->virtfs.root_nodeid, rel);
     return (int)UIOX_NFS_ERR_UNSUP;
 }
 
 int uiox_nfs_vfs_rename(const char *from, const char *to)
 {
     const char *rel_from=NULL, *rel_to=NULL;
     uiox_nfs_mount_t *mf = uiox_nfs_vfs_resolve(from, &rel_from);
     uiox_nfs_mount_t *mt = uiox_nfs_vfs_resolve(to,   &rel_to);
     if (!mf || !mt || mf != mt) return (int)UIOX_NFS_ERR_INVAL;
     if (mf->read_only) return (int)UIOX_NFS_ERR_PERM;
     if (mf->type == UIOX_NETFS_NFS3)
         return (int)uiox_nfs3_rename(&mf->nfs3,
                                        &mf->nfs3.root_fh, rel_from,
                                        &mf->nfs3.root_fh, rel_to);
     return (int)UIOX_NFS_ERR_UNSUP;
 }
 
 int uiox_nfs_vfs_readdir(const char *path,
                            uiox_nfs3_readdir_cb_t cb, void *priv)
 {
     const char *rel = NULL;
     uiox_nfs_mount_t *m = uiox_nfs_vfs_resolve(path, &rel);
     if (!m) return (int)UIOX_NFS_ERR_NOENT;
     if (m->type == UIOX_NETFS_NFS3) {
         uiox_nfs_fh_t fh; uiox_nfs_attr_t attr;
         uiox_nfs_err_t rc = uiox_nfs3_path_lookup(&m->nfs3, rel,
                                                      &fh, &attr);
         if (rc != UIOX_NFS_OK) return (int)rc;
         return (int)uiox_nfs3_readdir(&m->nfs3, &fh, cb, priv);
     }
     if (m->type == UIOX_NETFS_VIRTFS) {
         uint64_t nodeid=0u; uiox_nfs_attr_t attr;
         uiox_nfs_err_t rc = uiox_virtfs_path_lookup(&m->virtfs, rel,
                                                        &nodeid, &attr);
         if (rc != UIOX_NFS_OK) return (int)rc;
         return (int)uiox_virtfs_readdir(&m->virtfs, nodeid, 0u,
                                           0u, cb, priv);
     }
     return (int)UIOX_NFS_ERR_UNSUP;
 }
 
 int uiox_nfs_vfs_statfs(const char *path, uiox_nfs_statfs_t *stat)
 {
     const char *rel = NULL;
     uiox_nfs_mount_t *m = uiox_nfs_vfs_resolve(path, &rel);
     if (!m) return (int)UIOX_NFS_ERR_NOENT;
     if (m->type == UIOX_NETFS_NFS3)
         return (int)uiox_nfs3_fsstat(&m->nfs3, &m->nfs3.root_fh, stat);
     if (m->type == UIOX_NETFS_VIRTFS)
         return (int)uiox_virtfs_statfs(&m->virtfs,
                                          m->virtfs.root_nodeid, stat);
     return (int)UIOX_NFS_ERR_UNSUP;
 }
 
 int uiox_nfs_vfs_fsync(int gfd)
 {
     uiox_nfs_mount_t *m  = NULL;
     uiox_nfs_fd_t    *fd = global_to_fd(gfd, &m);
     if (!fd) return (int)UIOX_NFS_ERR_INVAL;
     if (m->type == UIOX_NETFS_NFS3)
         return (int)uiox_nfs3_commit(&m->nfs3, &fd->fh, 0u, 0u);
     return 0;
 }
 
 void uiox_nfs_vfs_print(void)
 {
     uiox_fw_printf("[netfs] Mount table:\n");
     for (uint8_t i=0u;i<UIOX_NFS_MAX_MOUNTS;i++) {
         const uiox_nfs_mount_t *m = &s_mounts[i];
         if (!m->mounted) continue;
         static const char *type_names[]={"NFS3","9P2000","VirtFS"};
         uiox_fw_printf("  [%u] %-20s  type=%-8s  ro=%d  open=%u\n",
                         i, m->mount_point,
                         (uint32_t)m->type < 3u ? type_names[m->type] : "?",
                         (int)m->read_only,
                         m->open_files);
         uiox_fw_printf("       read=%llu B  write=%llu B\n",
                         (unsigned long long)m->bytes_read,
                         (unsigned long long)m->bytes_written);
     }
 }
 
 /* =========================================================================
  * Syscall handlers
  * ====================================================================== */
 
 long sys_mount(long src, long tgt, long fstype, long flags)
 {
     uiox_nfs_mount_params_t p;
     v_memset(&p, 0, sizeof(p));
     const char *target   = (const char *)tgt;
     const char *fs_type  = (const char *)fstype;
     v_strncpy(p.mount_point, target, UIOX_NFS_PATH_MAX);
     /* Determine type from fstype string */
     if (v_strncmp(fs_type, "nfs",     3) == 0) p.type = UIOX_NETFS_NFS3;
     else if (v_strncmp(fs_type, "9p", 2) == 0) p.type = UIOX_NETFS_9P2000;
     else                                         p.type = UIOX_NETFS_VIRTFS;
     p.read_only   = !!(flags & 1u);
     p.timeout_ms  = 5000u;
     /* source: "server:/export" for NFS */
     const char *source = (const char *)src;
     if (p.type == UIOX_NETFS_NFS3) {
         /* Parse "W.X.Y.Z:/path" */
         uint8_t a=0,b=0,c=0,d=0; int i=0;
         while (source[i] && source[i] != '.') a=(uint8_t)(a*10+source[i++]-'0');
         if (source[i]=='.'){i++;while(source[i]&&source[i]!='.')b=(uint8_t)(b*10+source[i++]-'0');}
         if (source[i]=='.'){i++;while(source[i]&&source[i]!='.')c=(uint8_t)(c*10+source[i++]-'0');}
         if (source[i]=='.'){i++;while(source[i]&&source[i]!=':')d=(uint8_t)(d*10+source[i++]-'0');}
         p.server_ip[0]=a; p.server_ip[1]=b; p.server_ip[2]=c; p.server_ip[3]=d;
         if (source[i]==':') v_strncpy(p.export_path, source+i+1, UIOX_NFS_PATH_MAX);
     }
     uint8_t idx = 0u;
     return (long)uiox_nfs_vfs_mount(&p, &idx);
 }
 
 long sys_umount(long tgt, long flags, long a2, long a3)
 {
     UIOX_NFS_UNUSED(flags); UIOX_NFS_UNUSED(a2); UIOX_NFS_UNUSED(a3);
     return (long)uiox_nfs_vfs_umount((const char *)tgt);
 }
 
 long sys_statfs(long path, long buf, long a2, long a3)
 {
     UIOX_NFS_UNUSED(a2); UIOX_NFS_UNUSED(a3);
     return (long)uiox_nfs_vfs_statfs((const char *)path,
                                        (uiox_nfs_statfs_t *)buf);
 }
 