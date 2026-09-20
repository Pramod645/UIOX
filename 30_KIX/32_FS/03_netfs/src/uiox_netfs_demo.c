/**
 * @file  uiox_netfs_demo.c
 * @brief UIOX Network Filesystem demo — exercises NFS3, 9P, VirtFS.
 * @date  2026-07-07
 */

 #include "../include/uiox_netfs.h"

 extern void uiox_fw_printf(const char *fmt, ...);
 
 /* ── Stub network callbacks (QEMU simulation — no real network) ─ */
 
 static uiox_nfs_err_t stub_send(void *ctx,
                                   const uint8_t *buf, uint32_t len)
 {
     UIOX_NFS_UNUSED(ctx); UIOX_NFS_UNUSED(buf); UIOX_NFS_UNUSED(len);
     return UIOX_NFS_OK;
 }
 
 static uiox_nfs_err_t stub_recv(void *ctx,
                                   uint8_t *buf, uint32_t max,
                                   uint32_t *rx_len, uint32_t timeout_ms)
 {
     UIOX_NFS_UNUSED(ctx); UIOX_NFS_UNUSED(timeout_ms);
     /* Build a minimal stub RPC reply: XID + REPLY + ACCEPTED + SUCCESS */
     static const uint8_t stub_reply[] = {
         0x12,0x34,0x56,0x78,  /* XID */
         0x00,0x00,0x00,0x01,  /* msg_type = REPLY */
         0x00,0x00,0x00,0x00,  /* reply_stat = MSG_ACCEPTED */
         0x00,0x00,0x00,0x00,  /* verifier flavor = AUTH_NULL */
         0x00,0x00,0x00,0x00,  /* verifier length = 0 */
         0x00,0x00,0x00,0x00,  /* accept_stat = SUCCESS */
         /* NFS3 GETATTR reply: status=0 + minimal attr */
         0x00,0x00,0x00,0x00,  /* NFS3_OK */
         0x00,0x00,0x00,0x01,  /* ftype = REG */
         0x00,0x00,0x01,0xA4,  /* mode = 0644 */
         0x00,0x00,0x00,0x01,  /* nlink */
         0x00,0x00,0x00,0x00,  /* uid */
         0x00,0x00,0x00,0x00,  /* gid */
         0x00,0x00,0x00,0x00,0x00,0x00,0x10,0x00, /* size = 4096 */
         0x00,0x00,0x00,0x00,0x00,0x00,0x10,0x00, /* used */
         0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /* rdev */
         0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01, /* fsid */
         0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02, /* fileid */
         0x00,0x00,0x00,0x00,0x67,0x7B,0xD3,0x80, /* atime_sec */
         0x00,0x00,0x00,0x00,0x67,0x7B,0xD3,0x80, /* mtime_sec */
         0x00,0x00,0x00,0x00,0x67,0x7B,0xD3,0x80, /* ctime_sec */
     };
     uint32_t n = sizeof(stub_reply);
     if (n > max) n = max;
     uint8_t *p = (uint8_t*)buf;
     const uint8_t *s = stub_reply;
     for (uint32_t i=0u;i<n;i++) p[i]=s[i];
     *rx_len = n;
     return UIOX_NFS_OK;
 }
 
 /* ── Readdir callback ────────────────────────────────────────── */
 static bool demo_readdir_cb(const uiox_nfs_dirent_t *de, void *priv)
 {
     uint32_t *count = (uint32_t *)priv;
     uiox_fw_printf("  dirent[%u]: ino=%llu  name=%s\n",
                     *count,
                     (unsigned long long)de->fileid,
                     de->name);
     (*count)++;
     return true;  /* continue */
 }
 
 /* =========================================================================
  * Demo main
  * ====================================================================== */
 
 void uiox_netfs_demo(void)
 {
     uiox_fw_printf("\n=== UIOX Network Filesystem Demo ===\n\n");
 
     /* ── Step 1: Init VFS layer ─────────────────────────────── */
     uiox_fw_printf("--- Step 1: VFS layer init ---\n");
     uiox_nfs_err_t rc = uiox_nfs_vfs_init();
     uiox_fw_printf("  vfs_init: %s\n", uiox_nfs_err_str(rc));
 
     /* ── Step 2: Mount NFS v3 (simulated) ───────────────────── */
     uiox_fw_printf("\n--- Step 2: Mount NFS v3 ---\n");
     static const uint8_t server_ip[] = { 192u, 168u, 1u, 100u };
     uiox_nfs_mount_params_t nfs_p;
     uint8_t *pp = (uint8_t *)&nfs_p;
     for (size_t i=0u;i<sizeof(nfs_p);i++) pp[i]=0u;
     nfs_p.type           = UIOX_NETFS_NFS3;
     nfs_p.server_ip[0]   = 192u; nfs_p.server_ip[1] = 168u;
     nfs_p.server_ip[2]   =   1u; nfs_p.server_ip[3] = 100u;
     const char *ep = "/export/uiox";
     for (int i=0;ep[i];i++) nfs_p.export_path[i]=ep[i];
     const char *mp = "/nfs";
     for (int i=0;mp[i];i++) nfs_p.mount_point[i]=mp[i];
     nfs_p.net_send       = stub_send;
     nfs_p.net_recv       = stub_recv;
     nfs_p.timeout_ms     = 5000u;
     nfs_p.read_only      = false;
     uint8_t midx = 0u;
     rc = uiox_nfs_vfs_mount(&nfs_p, &midx);
     uiox_fw_printf("  nfs3_mount: %s  slot=%u\n",
                     uiox_nfs_err_str(rc), midx);
 
     /* ── Step 3: Mount VirtIO-FS (stub MMIO=0 = sim mode) ───── */
     uiox_fw_printf("\n--- Step 3: Mount VirtIO-FS ---\n");
     uiox_nfs_mount_params_t vfs_p;
     pp = (uint8_t *)&vfs_p;
     for (size_t i=0u;i<sizeof(vfs_p);i++) pp[i]=0u;
     vfs_p.type            = UIOX_NETFS_VIRTFS;
     const char *vmp = "/virtfs";
     for (int i=0;vmp[i];i++) vfs_p.mount_point[i]=vmp[i];
     vfs_p.virtio_mmio_base = 0u;   /* 0 = simulation mode */
     uint8_t vidx = 0u;
     rc = uiox_nfs_vfs_mount(&vfs_p, &vidx);
     uiox_fw_printf("  virtfs_mount: %s  slot=%u\n",
                     uiox_nfs_err_str(rc), vidx);
 
     /* ── Step 4: Print mount table ──────────────────────────── */
     uiox_fw_printf("\n--- Step 4: Mount table ---\n");
     uiox_nfs_vfs_print();
 
     /* ── Step 5: stat on NFS path ───────────────────────────── */
     uiox_fw_printf("\n--- Step 5: stat /nfs/README.md ---\n");
     uiox_nfs_attr_t attr;
     uint8_t *ap = (uint8_t*)&attr;
     for (size_t i=0u;i<sizeof(attr);i++) ap[i]=0u;
     rc = uiox_nfs_vfs_stat("/nfs/README.md", &attr);
     uiox_fw_printf("  stat: %s  size=%llu  mode=0%o  ftype=%u\n",
                     uiox_nfs_err_str(rc),
                     (unsigned long long)attr.size,
                     attr.mode,
                     (uint32_t)attr.ftype);
 
     /* ── Step 6: open / read / close ────────────────────────── */
     uiox_fw_printf("\n--- Step 6: open/read/close /nfs/test.txt ---\n");
     int fd = uiox_nfs_vfs_open("/nfs/test.txt",
                                 UIOX_NFS_O_RDONLY, 0u);
     uiox_fw_printf("  open fd=%d\n", fd);
     if (fd >= 1000) {
         uint8_t rbuf[64];
         int got = uiox_nfs_vfs_read(fd, rbuf, sizeof(rbuf) - 1u);
         uiox_fw_printf("  read: %d bytes\n", got);
         uiox_nfs_vfs_close(fd);
         uiox_fw_printf("  closed\n");
     }
 
     /* ── Step 7: open / write / fsync / close ───────────────── */
     uiox_fw_printf("\n--- Step 7: write /nfs/output.txt ---\n");
     int wfd = uiox_nfs_vfs_open("/nfs/output.txt",
                                   UIOX_NFS_O_WRONLY | UIOX_NFS_O_CREAT,
                                   0644u);
     if (wfd >= 1000) {
         static const uint8_t data[] = "Hello UIOX netfs!\n";
         int written = uiox_nfs_vfs_write(wfd, data, 18u);
         uiox_fw_printf("  write: %d bytes\n", written);
         uiox_nfs_vfs_fsync(wfd);
         uiox_fw_printf("  fsynced\n");
         uiox_nfs_vfs_close(wfd);
     }
 
     /* ── Step 8: mkdir ──────────────────────────────────────── */
     uiox_fw_printf("\n--- Step 8: mkdir /nfs/newdir ---\n");
     rc = uiox_nfs_vfs_mkdir("/nfs/newdir", 0755u);
     uiox_fw_printf("  mkdir: %s\n", uiox_nfs_err_str(rc));
 
     /* ── Step 9: readdir ────────────────────────────────────── */
     uiox_fw_printf("\n--- Step 9: readdir /nfs ---\n");
     uint32_t de_count = 0u;
     rc = uiox_nfs_vfs_readdir("/nfs", demo_readdir_cb, &de_count);
     uiox_fw_printf("  readdir: %s  %u entries\n",
                     uiox_nfs_err_str(rc), de_count);
 
     /* ── Step 10: statfs ────────────────────────────────────── */
     uiox_fw_printf("\n--- Step 10: statfs /nfs ---\n");
     uiox_nfs_statfs_t sfs;
     uint8_t *sfp = (uint8_t*)&sfs;
     for (size_t i=0u;i<sizeof(sfs);i++) sfp[i]=0u;
     rc = uiox_nfs_vfs_statfs("/nfs", &sfs);
     uiox_fw_printf("  statfs: %s  total=%llu MB  free=%llu MB\n",
                     uiox_nfs_err_str(rc),
                     (unsigned long long)(sfs.tbytes >> 20u),
                     (unsigned long long)(sfs.fbytes >> 20u));
 
     /* ── Step 11: rename ────────────────────────────────────── */
     uiox_fw_printf("\n--- Step 11: rename /nfs/test.txt → /nfs/test2.txt ---\n");
     rc = uiox_nfs_vfs_rename("/nfs/test.txt", "/nfs/test2.txt");
     uiox_fw_printf("  rename: %s\n", uiox_nfs_err_str(rc));
 
     /* ── Step 12: unlink ────────────────────────────────────── */
     uiox_fw_printf("\n--- Step 12: unlink /nfs/test2.txt ---\n");
     rc = uiox_nfs_vfs_unlink("/nfs/test2.txt");
     uiox_fw_printf("  unlink: %s\n", uiox_nfs_err_str(rc));
 
     /* ── Step 13: cache statistics ──────────────────────────── */
     uiox_fw_printf("\n--- Step 13: Cache statistics ---\n");
     /* Access mount 0's cache directly for demo */
     extern uiox_nfs_mount_t s_mounts[];
     if (s_mounts[0].mounted)
         uiox_nfs_cache_print(&s_mounts[0].cache);
 
     /* ── Step 14: sys_mount syscall interface ───────────────── */
     uiox_fw_printf("\n--- Step 14: sys_mount syscall ---\n");
     long sret = sys_mount((long)"192.168.1.200:/data",
                            (long)"/mnt/nfs2",
                            (long)"nfs", 0L);
     uiox_fw_printf("  sys_mount: %s\n",
                     uiox_nfs_err_str((uiox_nfs_err_t)sret));
 
     /* ── Step 15: Umount all ────────────────────────────────── */
     uiox_fw_printf("\n--- Step 15: Umount all ---\n");
     rc = uiox_nfs_vfs_umount("/nfs");
     uiox_fw_printf("  umount /nfs:    %s\n", uiox_nfs_err_str(rc));
     rc = uiox_nfs_vfs_umount("/virtfs");
     uiox_fw_printf("  umount /virtfs: %s\n", uiox_nfs_err_str(rc));
     rc = uiox_nfs_vfs_umount("/mnt/nfs2");
     uiox_fw_printf("  umount /mnt/nfs2: %s\n", uiox_nfs_err_str(rc));
 
     uiox_nfs_vfs_deinit();
     uiox_fw_printf("\n=== UIOX netfs Demo complete ===\n");
 }
 