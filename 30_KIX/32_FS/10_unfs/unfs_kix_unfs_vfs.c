/*
 * 30_KIX/32_FS/10_unfs/unfs_vfs.c
 *
 * UNFS VFS integration — connects UNFS to the UIOX VFS layer.
 *
 * Registers unfs_fs_ops so vfs_register_fs() + vfs_mount_root()
 * can mount a UNFS volume as the root filesystem.
 *
 * Data flow (read path):
 *   sys_read(fd, ubuf, n)
 *     → vfs_read(file, kbuf, n)
 *         → unfs_file_ops.read(file, kbuf, n, pos)
 *             → uiox_pc_read(ino, pos, kbuf, n)   [page cache]
 *                 on miss: unfs_readpage(ino, offset)
 *                     → unfs_extent_phys(ino, blk)
 *                     → bread(dev, blkno) × 8     [block cache]
 *             → returns kbuf to sys_read
 *     → uiox_copy_to_user(ubuf, kbuf, n)          [privilege crossing]
 *
 * @version 1.0.0  @date 2026-07-29
 */

 #include "unfs_fs.h"
 #include "uiox_vfs.h"
 #include "uiox_page_cache.h"
 #include "uiox_journal.h"
 #include "uiox_soc_string.h"
 #include "uiox_soc_stdio.h"
 
 /* ── Global UNFS state (single mounted volume) ──────────────────────── */
 static unfs_fs_t s_unfs;
 
 /* ── Block device stub ──────────────────────────────────────────────── */
 #define UNFS_DEV_ID  0u
 
 /* Forward declarations */
 static int unfs_map_block(uint32_t ino, uint64_t file_off,
                            uint8_t *dev_out, uint32_t *blkno_out);
 
 /* ── CRC32C (shared impl) ───────────────────────────────────────────── */
 uint32_t unfs_crc32c(const void *data, uint32_t len)
 {
     /* Same impl as 01_uBoot/src/unfs.c */
     const uint8_t *p = (const uint8_t *)data;
     uint32_t crc = 0xFFFFFFFFu;
     /* Castagnoli polynomial — abbreviated; real impl uses 256-entry table */
     for (uint32_t i = 0u; i < len; i++) {
         crc ^= p[i];
         for (int b = 0; b < 8; b++)
             crc = (crc >> 1) ^ (0x82F63B78u & -(crc & 1u));
     }
     return crc ^ 0xFFFFFFFFu;
 }
 
 /* ─────────────────────────────────────────────────────────────────────
  * Block I/O helpers
  * ───────────────────────────────────────────────────────────────────── */
 #include "bcache.h"   /* bread, bwrite, brelse, bdwrite — 31_BufferCache */
 
 static void blk_read(uint32_t blkno, void *buf)
 {
     /* One UNFS block = 8 × 512-byte sectors */
     uint8_t *dst = (uint8_t *)buf;
     for (uint32_t i = 0u; i < 8u; i++) {
         BufHdr *b = bread(UNFS_DEV_ID, blkno * 8u + i);
         memcpy(dst + i * 512u, b->data, 512u);
         brelse(b);
     }
 }
 
 static void blk_write(uint32_t blkno, const void *buf)
 {
     const uint8_t *src = (const uint8_t *)buf;
     for (uint32_t i = 0u; i < 8u; i++) {
         BufHdr *b = getblk(UNFS_DEV_ID, blkno * 8u + i);
         memcpy(b->data, src + i * 512u, 512u);
         b->status |= BUF_VALID;
         bdwrite(b);   /* delayed write — journal commits later */
     }
 }
 
 /* ─────────────────────────────────────────────────────────────────────
  * Inode I/O
  * ───────────────────────────────────────────────────────────────────── */
 static int read_disk_inode(uint32_t ino, unfs_inode_t *out)
 {
     if (ino < 1u || ino > s_unfs.disk.s_inode_count) return -22;
 
     uint32_t ino0      = ino - 1u;
     uint32_t grp       = ino0 / s_unfs.disk.s_inodes_per_group;
     uint32_t local_idx = ino0 % s_unfs.disk.s_inodes_per_group;
     uint32_t blk_off   = local_idx / UNFS_INODES_PER_BLOCK;
     uint32_t blk_local = local_idx % UNFS_INODES_PER_BLOCK;
 
     if (grp >= s_unfs.n_groups) return -22;
 
     static uint8_t ibuf[UNFS_BLOCK_SIZE];
     blk_read(s_unfs.groups[grp].bg_inode_table + blk_off, ibuf);
 
     const unfs_inode_t *src =
         (const unfs_inode_t *)(ibuf + blk_local * UNFS_INODE_SIZE);
 
     uint32_t expected = unfs_crc32c(src, UNFS_INODE_SIZE - 4u);
     if (expected != src->i_checksum) return -117; /* ECORRUPT */
 
     memcpy(out, src, UNFS_INODE_SIZE);
     return 0;
 }
 
 static int write_disk_inode(uint32_t ino, const unfs_inode_t *inode)
 {
     uint32_t ino0      = ino - 1u;
     uint32_t grp       = ino0 / s_unfs.disk.s_inodes_per_group;
     uint32_t local_idx = ino0 % s_unfs.disk.s_inodes_per_group;
     uint32_t blk_off   = local_idx / UNFS_INODES_PER_BLOCK;
     uint32_t blk_local = local_idx % UNFS_INODES_PER_BLOCK;
 
     if (grp >= s_unfs.n_groups) return -22;
 
     static uint8_t ibuf[UNFS_BLOCK_SIZE];
     blk_read(s_unfs.groups[grp].bg_inode_table + blk_off, ibuf);
 
     unfs_inode_t *dst =
         (unfs_inode_t *)(ibuf + blk_local * UNFS_INODE_SIZE);
     memcpy(dst, inode, UNFS_INODE_SIZE - 4u);
     dst->i_checksum = unfs_crc32c(dst, UNFS_INODE_SIZE - 4u);
     memcpy(ibuf + blk_local * UNFS_INODE_SIZE, dst, UNFS_INODE_SIZE);
 
     blk_write(s_unfs.groups[grp].bg_inode_table + blk_off, ibuf);
     return 0;
 }
 
 /* ─────────────────────────────────────────────────────────────────────
  * Extent lookup — logical block → physical block
  * ───────────────────────────────────────────────────────────────────── */
 static uint32_t extent_phys(const unfs_inode_t *inode, uint32_t logical)
 {
     for (int i = 0; i < 4; i++) {
         const unfs_extent_t *e = &inode->i_extents[i];
         if (e->e_len == 0u) continue;
         if (logical >= e->e_logical &&
             logical <  e->e_logical + e->e_len) {
             if (e->e_flags & UNFS_EXT_HOLE) return 0u;
             return e->e_physical + (logical - e->e_logical);
         }
     }
     if (inode->i_extent_tree == 0u) return 0u;
 
     static uint8_t etbuf[UNFS_BLOCK_SIZE];
     blk_read(inode->i_extent_tree, etbuf);
     uint32_t max = UNFS_BLOCK_SIZE / sizeof(unfs_extent_t);
     const unfs_extent_t *et = (const unfs_extent_t *)etbuf;
     for (uint32_t i = 0u; i < max; i++) {
         if (et[i].e_len == 0u) break;
         if (logical >= et[i].e_logical &&
             logical <  et[i].e_logical + et[i].e_len) {
             if (et[i].e_flags & UNFS_EXT_HOLE) return 0u;
             return et[i].e_physical + (logical - et[i].e_logical);
         }
     }
     return 0u;
 }
 
 /* ─────────────────────────────────────────────────────────────────────
  * Page cache block-mapping callback
  * Called by uiox_pc_read() on cache miss to fill a page.
  * Translates (ino, file_offset) → (dev, sector_blkno).
  * ───────────────────────────────────────────────────────────────────── */
 static int unfs_map_block(uint32_t  ino,
                            uint64_t  file_off,
                            uint8_t  *dev_out,
                            uint32_t *blkno_out)
 {
     unfs_inode_t inode;
     if (read_disk_inode(ino, &inode) != 0) return -5;
 
     uint32_t logical = (uint32_t)(file_off / UNFS_BLOCK_SIZE);
     uint32_t phys    = extent_phys(&inode, logical);
     if (phys == 0u) return -2;  /* hole / ENOENT */
 
     uint32_t sector_idx = (uint32_t)
         ((file_off % UNFS_BLOCK_SIZE) / 512u);
 
     *dev_out   = UNFS_DEV_ID;
     *blkno_out = phys * 8u + sector_idx;
     return 0;
 }
 
 /* ─────────────────────────────────────────────────────────────────────
  * Block allocator
  * ───────────────────────────────────────────────────────────────────── */
 uint32_t unfs_alloc_block(unfs_fs_t *fs)
 {
     static uint8_t bmap[UNFS_BLOCK_SIZE];
 
     for (uint32_t g = 0u; g < fs->n_groups; g++) {
         if (fs->groups[g].bg_free_blocks == 0u) continue;
 
         blk_read(fs->groups[g].bg_block_bitmap, bmap);
 
         for (uint32_t byte = 0u; byte < UNFS_BLOCK_SIZE; byte++) {
             if (bmap[byte] == 0xFFu) continue;
             for (uint8_t bit = 0u; bit < 8u; bit++) {
                 if (!(bmap[byte] & (1u << bit))) {
                     bmap[byte] |= (uint8_t)(1u << bit);
                     blk_write(fs->groups[g].bg_block_bitmap, bmap);
                     fs->groups[g].bg_free_blocks--;
                     fs->disk.s_free_blocks--;
                     return g * fs->disk.s_blocks_per_group
                          + byte * 8u + bit;
                 }
             }
         }
     }
     return 0u;   /* 0 = allocation failure (block 0 is superblock) */
 }
 
 void unfs_free_block(unfs_fs_t *fs, uint32_t blkno)
 {
     uint32_t grp  = blkno / fs->disk.s_blocks_per_group;
     uint32_t local= blkno % fs->disk.s_blocks_per_group;
     if (grp >= fs->n_groups) return;
 
     static uint8_t bmap[UNFS_BLOCK_SIZE];
     blk_read(fs->groups[grp].bg_block_bitmap, bmap);
     bmap[local / 8u] &= (uint8_t)~(1u << (local % 8u));
     blk_write(fs->groups[grp].bg_block_bitmap, bmap);
     fs->groups[grp].bg_free_blocks++;
     fs->disk.s_free_blocks++;
 }
 
 /* ─────────────────────────────────────────────────────────────────────
  * Inode allocator
  * ───────────────────────────────────────────────────────────────────── */
 uint32_t unfs_alloc_inode(unfs_fs_t *fs, uint16_t mode)
 {
     static uint8_t imap[UNFS_BLOCK_SIZE];
 
     for (uint32_t g = 0u; g < fs->n_groups; g++) {
         if (fs->groups[g].bg_free_inodes == 0u) continue;
 
         blk_read(fs->groups[g].bg_inode_bitmap, imap);
 
         for (uint32_t byte = 0u; byte < UNFS_BLOCK_SIZE; byte++) {
             if (imap[byte] == 0xFFu) continue;
             for (uint8_t bit = 0u; bit < 8u; bit++) {
                 if (!(imap[byte] & (1u << bit))) {
                     imap[byte] |= (uint8_t)(1u << bit);
                     blk_write(fs->groups[g].bg_inode_bitmap, imap);
                     fs->groups[g].bg_free_inodes--;
                     fs->disk.s_free_inodes--;
 
                     uint32_t ino = g * fs->disk.s_inodes_per_group
                                  + byte * 8u + bit + 1u;
 
                     /* Initialise fresh inode */
                     unfs_inode_t ni;
                     memset(&ni, 0, sizeof(ni));
                     ni.i_mode  = mode;
                     ni.i_nlink = 1u;
                     ni.i_checksum = unfs_crc32c(&ni, UNFS_INODE_SIZE-4u);
                     write_disk_inode(ino, &ni);
                     return ino;
                 }
             }
         }
     }
     return 0u;
 }
 
 void unfs_free_inode(unfs_fs_t *fs, uint32_t ino)
 {
     uint32_t ino0  = ino - 1u;
     uint32_t grp   = ino0 / fs->disk.s_inodes_per_group;
     uint32_t local = ino0 % fs->disk.s_inodes_per_group;
     if (grp >= fs->n_groups) return;
 
     static uint8_t imap[UNFS_BLOCK_SIZE];
     blk_read(fs->groups[grp].bg_inode_bitmap, imap);
     imap[local / 8u] &= (uint8_t)~(1u << (local % 8u));
     blk_write(fs->groups[grp].bg_inode_bitmap, imap);
     fs->groups[grp].bg_free_inodes++;
     fs->disk.s_free_inodes++;
 }
 
 /* ─────────────────────────────────────────────────────────────────────
  * COW block write — allocate new block, copy old content
  * ───────────────────────────────────────────────────────────────────── */
 uint32_t unfs_cow_block(unfs_fs_t *fs, uint32_t old_phys)
 {
     if (!fs->cow_active) return old_phys;
 
     uint32_t new_phys = unfs_alloc_block(fs);
     if (new_phys == 0u) return 0u;
 
     static uint8_t cow_buf[UNFS_BLOCK_SIZE];
     blk_read(old_phys, cow_buf);
     blk_write(new_phys, cow_buf);
     return new_phys;
 }
 
 /* ─────────────────────────────────────────────────────────────────────
  * VFS inode operations
  * ───────────────────────────────────────────────────────────────────── */
 static int unfs_vfs_lookup(uiox_inode_t  *dir,
                              const char    *name,
                              uiox_inode_t **out)
 {
     unfs_inode_priv_t *dpriv = (unfs_inode_priv_t *)dir->i_private;
     if (!dpriv) return -22;
 
     unfs_inode_t *di = &dpriv->disk;
     uint64_t size   = di->i_size;
     uint32_t n_blks = (uint32_t)((size + UNFS_BLOCK_SIZE - 1u)
                                   / UNFS_BLOCK_SIZE);
 
     static uint8_t dbuf[UNFS_BLOCK_SIZE];
     uint8_t name_len = 0u;
     while (name[name_len]) name_len++;
 
     for (uint32_t b = 0u; b < n_blks; b++) {
         uint32_t phys = extent_phys(di, b);
         if (phys == 0u) continue;
         blk_read(phys, dbuf);
 
         uint32_t off = 0u;
         while (off + sizeof(unfs_dirent_t) <= UNFS_BLOCK_SIZE) {
             const unfs_dirent_t *de = (const unfs_dirent_t *)(dbuf + off);
             if (de->d_rec_len == 0u) break;
 
             if (de->d_ino != 0u && de->d_name_len == name_len) {
                 uint8_t match = 1u;
                 for (uint8_t i = 0u; i < name_len; i++)
                     if (de->d_name[i] != name[i]) { match=0u; break; }
                 if (match) {
                     /* Allocate VFS inode */
                     static uiox_inode_t child_vfs;
                     static unfs_inode_priv_t child_priv;
                     memset(&child_vfs,  0, sizeof(child_vfs));
                     memset(&child_priv, 0, sizeof(child_priv));
 
                     if (read_disk_inode(de->d_ino, &child_priv.disk) != 0)
                         return -5;
                     child_priv.ino = de->d_ino;
                     child_priv.fs  = &s_unfs;
 
                     child_vfs.i_ino     = de->d_ino;
                     child_vfs.i_size    = child_priv.disk.i_size;
                     child_vfs.i_mode    = child_priv.disk.i_mode;
                     child_vfs.i_nlink   = child_priv.disk.i_nlink;
                     child_vfs.i_ops     = &unfs_inode_ops;
                     child_vfs.i_fops    = &unfs_file_ops;
                     child_vfs.i_private = &child_priv;
                     *out = &child_vfs;
                     return 0;
                 }
             }
             off += de->d_rec_len;
         }
     }
     return -2;  /* ENOENT */
 }
 
 static int unfs_vfs_create(uiox_inode_t  *dir, const char *name,
                              uint32_t mode, uiox_inode_t **out)
 {
     /* Allocate inode */
     uint32_t new_ino = unfs_alloc_inode(&s_unfs, (uint16_t)mode);
     if (new_ino == 0u) return -12;
 
     /* TODO: add directory entry, update parent mtime */
     (void)dir; (void)name; (void)out;
     return 0;
 }
 
 static int unfs_vfs_stat(uiox_inode_t *inode, uiox_stat_t *out)
 {
     unfs_inode_priv_t *priv = (unfs_inode_priv_t *)inode->i_private;
     if (!priv) return -22;
 
     memset(out, 0, sizeof(*out));
     out->st_ino     = priv->ino;
     out->st_mode    = priv->disk.i_mode;
     out->st_nlink   = priv->disk.i_nlink;
     out->st_uid     = priv->disk.i_uid;
     out->st_gid     = priv->disk.i_gid;
     out->st_size    = priv->disk.i_size;
     out->st_atime   = priv->disk.i_atime_ns / 1000000000ULL;
     out->st_mtime   = priv->disk.i_mtime_ns / 1000000000ULL;
     out->st_ctime   = priv->disk.i_ctime_ns / 1000000000ULL;
     out->st_blksize = UNFS_BLOCK_SIZE;
     out->st_blocks  = priv->disk.i_blocks;
     return 0;
 }
 
 const uiox_inode_ops_t unfs_inode_ops = {
     .lookup   = unfs_vfs_lookup,
     .create   = unfs_vfs_create,
     .stat     = unfs_vfs_stat,
     .mkdir    = (void *)0,
     .unlink   = (void *)0,
     .rmdir    = (void *)0,
     .rename   = (void *)0,
     .truncate = (void *)0,
 };
 
 /* ─────────────────────────────────────────────────────────────────────
  * VFS file operations
  * ───────────────────────────────────────────────────────────────────── */
 static ssize_t unfs_vfs_read(uiox_file_t *file,
                                void        *kbuf,
                                size_t       count,
                                uint64_t    *pos)
 {
     unfs_inode_priv_t *priv =
         (unfs_inode_priv_t *)file->f_inode->i_private;
     if (!priv) return -9;
 
     if (*pos >= priv->disk.i_size) return 0;
     if (*pos + count > priv->disk.i_size)
         count = (size_t)(priv->disk.i_size - *pos);
 
     /*
      * Read via page cache.
      * uiox_pc_read calls unfs_map_block on cache miss
      * → bread(dev, blkno) × 8 → fills page → memcpy to kbuf.
      */
     ssize_t n = uiox_pc_read(priv->ino, *pos, kbuf, count);
     if (n > 0) *pos += (uint64_t)n;
     return n;
 }
 
 static ssize_t unfs_vfs_write(uiox_file_t   *file,
                                 const void    *kbuf,
                                 size_t         count,
                                 uint64_t      *pos)
 {
     unfs_inode_priv_t *priv =
         (unfs_inode_priv_t *)file->f_inode->i_private;
     if (!priv) return -9;
 
     /* Begin journal transaction */
     uiox_jr_handle_t *h = uiox_jr_start(2u);
     if (!h) return -12;
 
     /* COW: if write crosses existing block, copy first */
     uint32_t logical = (uint32_t)(*pos / UNFS_BLOCK_SIZE);
     uint32_t old_phys = extent_phys(&priv->disk, logical);
     if (old_phys != 0u && s_unfs.cow_active) {
         uint32_t new_phys = unfs_cow_block(&s_unfs, old_phys);
         /* Update extent to point to new block */
         for (int i = 0; i < 4; i++) {
             unfs_extent_t *e = &priv->disk.i_extents[i];
             if (e->e_physical == old_phys) {
                 uiox_jr_get_write_access(h, e->e_physical);
                 e->e_physical = new_phys;
                 break;
             }
         }
     }
 
     /* Write via page cache */
     ssize_t n = uiox_pc_write(priv->ino, *pos, kbuf, count);
 
     if (n > 0) {
         *pos += (uint64_t)n;
         if (*pos > priv->disk.i_size) priv->disk.i_size = *pos;
         priv->dirty = 1u;
 
         /* Dirty metadata in journal */
         uiox_jr_dirty_metadata(h,
             s_unfs.groups[0].bg_inode_table);
     }
 
     uiox_jr_stop(h);
     return n;
 }
 
 static uintptr_t unfs_vfs_mmap_page(uiox_file_t *file, uint64_t off)
 {
     unfs_inode_priv_t *priv =
         (unfs_inode_priv_t *)file->f_inode->i_private;
     if (!priv) return 0u;
 
     /* Zero-copy: return physical address of page from cache */
     return uiox_pc_get_page_pa(priv->ino, off);
 }
 
 static int unfs_vfs_fsync(uiox_file_t *file)
 {
     unfs_inode_priv_t *priv =
         (unfs_inode_priv_t *)file->f_inode->i_private;
     if (!priv) return -9;
 
     if (priv->dirty) {
         priv->disk.i_checksum =
             unfs_crc32c(&priv->disk, UNFS_INODE_SIZE - 4u);
         write_disk_inode(priv->ino, &priv->disk);
         priv->dirty = 0u;
     }
     return uiox_pc_sync_inode(priv->ino);
 }
 
 const uiox_file_ops_t unfs_file_ops = {
     .read      = unfs_vfs_read,
     .write     = unfs_vfs_write,
     .open      = (void *)0,
     .close     = (void *)0,
     .mmap_page = unfs_vfs_mmap_page,
     .readdir   = (void *)0,
     .fsync     = unfs_vfs_fsync,
     .ioctl     = (void *)0,
     .seek      = (void *)0,
 };
 
 /* ─────────────────────────────────────────────────────────────────────
  * VFS superblock operations — mount / unmount / sync
  * ───────────────────────────────────────────────────────────────────── */
 int unfs_kern_mount(uiox_superblock_t *sb, uint32_t dev_id)
 {
     (void)dev_id;
     memset(&s_unfs, 0, sizeof(s_unfs));
 
     /* Read and validate superblock */
     static uint8_t sbuf[UNFS_BLOCK_SIZE];
     blk_read(UNFS_SB_BLOCK, sbuf);
     const unfs_sb_t *disk_sb = (const unfs_sb_t *)sbuf;
 
     if (disk_sb->s_magic != UNFS_MAGIC) {
         early_puts("[unfs] bad magic\n");
         return -22;
     }
     uint32_t crc = unfs_crc32c(sbuf, UNFS_BLOCK_SIZE - 4u);
     if (crc != disk_sb->s_sb_checksum) {
         early_puts("[unfs] superblock checksum failed\n");
         return -117;
     }
     memcpy(&s_unfs.disk, disk_sb, sizeof(unfs_sb_t));
 
     /* Read group descriptor table */
     static uint8_t gdtbuf[UNFS_BLOCK_SIZE];
     blk_read(UNFS_GDT_BLOCK, gdtbuf);
     s_unfs.n_groups = s_unfs.disk.s_group_count;
     if (s_unfs.n_groups > UNFS_MAX_GROUPS)
         s_unfs.n_groups = UNFS_MAX_GROUPS;
 
     const unfs_group_desc_t *gdt = (const unfs_group_desc_t *)gdtbuf;
     for (uint32_t g = 0u; g < s_unfs.n_groups; g++)
         memcpy(&s_unfs.groups[g], &gdt[g], sizeof(unfs_group_desc_t));
 
     /* Register page cache block-mapping callback */
     uiox_pc_register_map(unfs_map_block);
 
     /* Mount root inode */
     static uiox_inode_t root_vfs;
     static unfs_inode_priv_t root_priv;
     memset(&root_vfs,  0, sizeof(root_vfs));
     memset(&root_priv, 0, sizeof(root_priv));
 
     if (read_disk_inode(UNFS_ROOT_INO, &root_priv.disk) != 0) {
         early_puts("[unfs] root inode read failed\n");
         return -5;
     }
     root_priv.ino = UNFS_ROOT_INO;
     root_priv.fs  = &s_unfs;
 
     root_vfs.i_ino     = UNFS_ROOT_INO;
     root_vfs.i_mode    = root_priv.disk.i_mode;
     root_vfs.i_size    = root_priv.disk.i_size;
     root_vfs.i_ops     = &unfs_inode_ops;
     root_vfs.i_fops    = &unfs_file_ops;
     root_vfs.i_private = &root_priv;
 
     sb->s_root      = &root_vfs;
     sb->s_blocksize = UNFS_BLOCK_SIZE;
     sb->s_private   = &s_unfs;
     s_unfs.vfs_sb   = sb;
     s_unfs.mounted  = 1u;
     s_unfs.cow_active = s_unfs.disk.s_cow_enabled;
 
     early_puts("[unfs] mounted\n");
     return 0;
 }
 
 int unfs_kern_unmount(uiox_superblock_t *sb)
 {
     (void)sb;
     uiox_pc_sync_all();
     s_unfs.mounted = 0u;
     return 0;
 }
 
 int unfs_kern_sync(uiox_superblock_t *sb)
 {
     (void)sb;
     return uiox_pc_sync_all();
 }
 
 const uiox_fs_ops_t unfs_fs_ops = {
     .name    = "unfs",
     .mount   = unfs_kern_mount,
     .unmount = unfs_kern_unmount,
     .sync    = unfs_kern_sync,
     .statfs  = (void *)0,
 };
 
 /* ─────────────────────────────────────────────────────────────────────
  * Snapshot support
  * ───────────────────────────────────────────────────────────────────── */
 int unfs_snap_create(unfs_fs_t *fs, const char *name)
 {
     if (!fs || !name) return -22;
     if (fs->n_snapshots >= UNFS_SNAP_MAX) return -28;
 
     unfs_snap_t *snap = &fs->snapshots[fs->n_snapshots];
     snap->snap_id    = fs->n_snapshots + 1u;
     snap->root_ino   = UNFS_ROOT_INO;
     snap->inuse      = 1u;
     uint8_t i = 0u;
     while (name[i] && i < 31u) { snap->name[i] = name[i]; i++; }
     snap->name[i] = '\0';
     fs->n_snapshots++;
 
     /* Enable COW from this point forward */
     fs->cow_active = 1u;
     early_puts("[unfs] snapshot created\n");
     return 0;
 }
 
 int unfs_snap_delete(unfs_fs_t *fs, uint64_t snap_id)
 {
     if (!fs) return -22;
     for (uint8_t i = 0u; i < fs->n_snapshots; i++) {
         if (fs->snapshots[i].snap_id == snap_id) {
             fs->snapshots[i].inuse = 0u;
             return 0;
         }
     }
     return -2;
 }
 
 int unfs_snap_list(unfs_fs_t *fs, unfs_snap_t *out, uint32_t max)
 {
     if (!fs || !out) return -22;
     uint32_t n = 0u;
     for (uint8_t i = 0u; i < fs->n_snapshots && n < max; i++) {
         if (fs->snapshots[i].inuse) {
             memcpy(&out[n], &fs->snapshots[i], sizeof(unfs_snap_t));
             n++;
         }
     }
     return (int)n;
 }
 
 /* ─────────────────────────────────────────────────────────────────────
  * Registration — called from uiox_fs_init()
  * ───────────────────────────────────────────────────────────────────── */
 void unfs_register(void)
 {
     vfs_register_fs(&unfs_fs_ops);
     early_puts("[unfs] registered\n");
 }
 