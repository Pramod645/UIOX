/*
 * 30_KIX/32_FS/10_scfs/scfs.c (New)
 *
 * UIOX Simple Contiguous File System (SCFS).
 *
 * SCFS is a flat, read-oriented filesystem for the boot device.
 * Layout on the block device:
 *
 *   Block 0:       Superblock
 *   Block 1:       Inode table  (SCFS_INODE_MAX inodes)
 *   Block 2..N:    Data blocks  (contiguous per file)
 *
 * This is intentionally simple — no directories within files,
 * flat namespace, no fragmentation. Suitable for initrd/ramfs.
 *
 * Data path to userspace:
 *   scfs_read() fills kbuf from physical DRAM page cache
 *   → vfs_read() returns kbuf to sys_read() in 33_PCS
 *   → sys_read() calls uiox_copy_to_user(ubuf, kbuf, n)
 *   → data arrives in user process
 *
 * @version 1.0.0  @date 2026-07-29
 */

 #include "uiox_vfs.h"
 #include "uiox_soc_string.h"
 #include "uiox_soc_stdio.h"




/*
 * Replace scfs_get_page() and scfs_mmap_page() in scfs.c.
 * All other scfs.c code stays identical.
 *
 * Add at top of scfs.c:
 */
#include "uiox_page_cache.h"   /* from 31_BufferCache/00_FileBuff/include */

/*
 * SCFS block mapping callback — registered with page cache at mount.
 * Translates (ino, file_offset) → (dev=0, blkno).
 */
static int scfs_map_block(uint32_t  ino,
                           uint64_t  file_offset,
                           uint8_t  *dev_out,
                           uint32_t *blkno_out)
{
    /* Find inode in SCFS inode table */
    for (uint32_t i = 0u; i < s_scfs.disk_sb.inode_count; i++) {
        scfs_inode_disk_t *di = &s_scfs.inodes[i];
        if (di->ino != ino) continue;

        uint32_t sector_idx = (uint32_t)(file_offset / BCACHE_SECTOR_SIZE);
        *dev_out   = 0u;
        *blkno_out = di->start_block * BCACHE_BLOCKS_PER_PAGE + sector_idx;
        return 0;
    }
    return -2;   /* ENOENT */
}

/*
 * Replace scfs_read() — now delegates to page cache:
 */
static ssize_t scfs_read(uiox_file_t *file,
                          void        *kbuf,
                          size_t       count,
                          uint64_t    *pos)
{
    if (!file || !kbuf || count == 0u) return 0;

    scfs_inode_disk_t *di = (scfs_inode_disk_t *)file->f_inode->i_private;
    if (!di) return (ssize_t)UIOX_FS_EBADF;

    /* Clamp to file size */
    if (*pos >= di->size) return 0;
    if (*pos + count > di->size)
        count = (size_t)(di->size - *pos);

    /*
     * uiox_pc_read fills kbuf from the page cache.
     * On a cache miss it calls readpage() → bread() → block device.
     * On a cache hit it copies directly from DRAM page — no device I/O.
     */
    ssize_t n = uiox_pc_read(di->ino, *pos, kbuf, count);
    if (n > 0) *pos += (uint64_t)n;
    return n;
}

/*
 * Replace scfs_mmap_page() — now delegates to page cache:
 */
static uintptr_t scfs_mmap_page(uiox_file_t *file, uint64_t off)
{
    scfs_inode_disk_t *di = (scfs_inode_disk_t *)file->f_inode->i_private;
    if (!di) return 0u;

    /*
     * uiox_pc_get_page_pa ensures the page is loaded and returns
     * its physical address. 33_PCS/uiox_sys_mmap.c then inserts
     * this PA as a PTE in the user page table — zero copy.
     */
    return uiox_pc_get_page_pa(di->ino, off);
}

/*
 * Update scfs_mount() — register block mapping callback:
 */
static int scfs_mount(uiox_superblock_t *sb, uint32_t dev_id)
{
    /* ... existing code ... */

    /* Register block mapping so page cache can call bread() */
    uiox_pc_register_map(scfs_map_block);

    /* ... rest of existing mount code ... */
    return UIOX_FS_OK;
}





/*
 * Update scfs_close() in scfs.c:
 */
static int scfs_close(uiox_inode_t *inode, uiox_file_t *file)
{
    (void)file;
    if (!inode) return UIOX_FS_OK;

    /*
     * On final close (inuse drops to 0), evict cached pages.
     * For open files kept in page cache for performance, remove
     * this call and rely on LRU eviction under memory pressure.
     */
    if (inode->i_inuse == 0u)
        uiox_pc_invalidate(inode->i_ino);

    return UIOX_FS_OK;
}




 
 /* ── SCFS on-disk layout ───────────────────────────────────────────── */
 #define SCFS_MAGIC         0x53434653UL   /* "SCFS"                    */
 #define SCFS_BLOCK_SIZE    4096u
 #define SCFS_INODE_MAX     64u
 #define SCFS_NAME_MAX      64u
 
 /* On-disk superblock (block 0) */
 typedef struct {
     uint32_t  magic;
     uint32_t  block_size;
     uint32_t  inode_count;
     uint32_t  data_start_block;  /* first data block index            */
     uint32_t  total_blocks;
     uint8_t   _pad[SCFS_BLOCK_SIZE - 20u];
 } __attribute__((packed)) scfs_sb_disk_t;
 
 /* On-disk inode */
 typedef struct {
     uint32_t  ino;
     uint8_t   type;              /* UIOX_DT_REG or UIOX_DT_DIR        */
     uint8_t   _pad[3];
     char      name[SCFS_NAME_MAX];
     uint64_t  size;              /* file size in bytes                 */
     uint32_t  start_block;      /* first data block index             */
     uint32_t  block_count;
 } __attribute__((packed)) scfs_inode_disk_t;
 
 /* In-memory page cache entry */
 typedef struct {
     uint32_t  ino;
     uint64_t  offset;            /* page-aligned offset in file        */
     uint8_t   data[SCFS_BLOCK_SIZE];
     uint8_t   valid;
 } scfs_page_t;
 
 #define SCFS_PAGE_CACHE_SIZE  32u
 static scfs_page_t s_page_cache[SCFS_PAGE_CACHE_SIZE];
 
 /* In-memory SCFS superblock data */
 typedef struct {
     scfs_sb_disk_t    disk_sb;
     scfs_inode_disk_t inodes[SCFS_INODE_MAX];
     uint8_t           loaded;
 } scfs_priv_t;
 
 static scfs_priv_t s_scfs;
 
 /* ── Block device read stub ────────────────────────────────────────── */
 /*
  * scfs_read_block — read one block from the block device into buf.
  *
  * For SCFS, "block device" is a flat region of DRAM starting at
  * SCFS_DRAM_BASE. In production this would call into
  * 30_DeviceDrivers/03_NonSensors/emmc or 31_BufferCache.
  *
  * SCFS_DRAM_BASE is provided by a weak platform hook —
  * override from 10_BSP for the real target.
  */
 #define SCFS_DRAM_BASE_DEFAULT  0x44000000UL   /* 64 MB above kernel   */
 
 __attribute__((weak))
 uintptr_t scfs_plat_dram_base(void)
 {
     return SCFS_DRAM_BASE_DEFAULT;
 }
 
 static void scfs_read_block(uint32_t block_idx, void *buf)
 {
     uintptr_t base = scfs_plat_dram_base();
     uintptr_t addr = base + (uintptr_t)block_idx * SCFS_BLOCK_SIZE;
     memcpy(buf, (const void *)addr, SCFS_BLOCK_SIZE);
 }
 
 /* ── Page cache ────────────────────────────────────────────────────── */
 static scfs_page_t *cache_lookup(uint32_t ino, uint64_t offset)
 {
     for (uint32_t i = 0u; i < SCFS_PAGE_CACHE_SIZE; i++) {
         if (s_page_cache[i].valid &&
             s_page_cache[i].ino    == ino &&
             s_page_cache[i].offset == offset)
             return &s_page_cache[i];
     }
     return (scfs_page_t *)0;
 }
 
 static scfs_page_t *cache_alloc(uint32_t ino, uint64_t offset)
 {
     /* Simple FIFO — find empty slot or overwrite slot 0 */
     for (uint32_t i = 0u; i < SCFS_PAGE_CACHE_SIZE; i++) {
         if (!s_page_cache[i].valid) {
             s_page_cache[i].ino    = ino;
             s_page_cache[i].offset = offset;
             s_page_cache[i].valid  = 1u;
             return &s_page_cache[i];
         }
     }
     /* Evict slot 0 (no dirty tracking for read-only SCFS) */
     s_page_cache[0].ino    = ino;
     s_page_cache[0].offset = offset;
     s_page_cache[0].valid  = 1u;
     return &s_page_cache[0];
 }
 
 /*
  * scfs_get_page — returns pointer to DRAM page for (ino, offset).
  * Fills page cache on miss by calling scfs_read_block().
  *
  * This is the core of the DRAM→kernel data path:
  *   physical block on device → page cache DRAM
  */
 static const uint8_t *scfs_get_page(const scfs_inode_disk_t *di,
                                      uint64_t file_offset)
 {
     uint64_t page_off = file_offset & ~((uint64_t)SCFS_BLOCK_SIZE - 1u);
 
     scfs_page_t *pg = cache_lookup(di->ino, page_off);
     if (pg) return pg->data;
 
     /* Cache miss — read from block device */
     uint32_t page_idx   = (uint32_t)(page_off / SCFS_BLOCK_SIZE);
     uint32_t block_idx  = di->start_block + page_idx;
 
     pg = cache_alloc(di->ino, page_off);
     scfs_read_block(block_idx, pg->data);
     return pg->data;
 }
 
 /* ── SCFS file operations ──────────────────────────────────────────── */
 static ssize_t scfs_read(uiox_file_t *file,
                           void        *kbuf,
                           size_t       count,
                           uint64_t    *pos)
 {
     if (!file || !kbuf || count == 0u) return 0;
 
     scfs_inode_disk_t *di = (scfs_inode_disk_t *)file->f_inode->i_private;
     if (!di) return (ssize_t)UIOX_FS_EBADF;
 
     /* Clamp to file size */
     if (*pos >= di->size) return 0;
     if (*pos + count > di->size)
         count = (size_t)(di->size - *pos);
 
     size_t   remaining = count;
     uint8_t *dst       = (uint8_t *)kbuf;
     uint64_t cur_pos   = *pos;
 
     while (remaining > 0u) {
         /*
          * scfs_get_page returns a pointer to the DRAM page
          * that holds file data at cur_pos.
          * We memcpy from it into kbuf — this is the
          * DRAM → kernel buffer copy.
          *
          * Then sys_read() in 33_PCS does:
          *   uiox_copy_to_user(ubuf, kbuf, n)
          * which is the kernel buffer → userspace copy.
          */
         const uint8_t *page = scfs_get_page(di, cur_pos);
         uint32_t page_off   = (uint32_t)(cur_pos % SCFS_BLOCK_SIZE);
         uint32_t can_copy   = SCFS_BLOCK_SIZE - page_off;
         if (can_copy > remaining) can_copy = (uint32_t)remaining;
 
         memcpy(dst, page + page_off, can_copy);
 
         dst       += can_copy;
         cur_pos   += can_copy;
         remaining -= can_copy;
     }
 
     *pos = cur_pos;
     return (ssize_t)count;
 }
 
 static ssize_t scfs_write(uiox_file_t   *file,
                            const void    *kbuf,
                            size_t         count,
                            uint64_t      *pos)
 {
     /*
      * Write path: kbuf → page cache DRAM → block device.
      * kbuf was already filled by copy_from_user in sys_write().
      * SCFS is read-only for now — write goes to journal in 02_journal.
      */
     (void)file; (void)kbuf; (void)count; (void)pos;
     return (ssize_t)UIOX_FS_ENOSYS;
 }
 
 static uintptr_t scfs_mmap_page(uiox_file_t *file, uint64_t off)
 {
     /*
      * Zero-copy mmap: return physical address of page at offset.
      * 33_PCS/uiox_mmap.c inserts this PA as a PTE in user page table.
      * No copy needed — user maps directly to DRAM.
      */
     scfs_inode_disk_t *di = (scfs_inode_disk_t *)file->f_inode->i_private;
     if (!di) return 0u;
 
     uint64_t page_off  = off & ~((uint64_t)SCFS_BLOCK_SIZE - 1u);
     uint32_t page_idx  = (uint32_t)(page_off / SCFS_BLOCK_SIZE);
     uint32_t block_idx = di->start_block + page_idx;
 
     return scfs_plat_dram_base() +
            (uintptr_t)block_idx * SCFS_BLOCK_SIZE;
 }
 
 static int scfs_open(uiox_inode_t *inode, uiox_file_t *file)
 {
     (void)inode; (void)file;
     return UIOX_FS_OK;
 }
 
 static int scfs_close(uiox_inode_t *inode, uiox_file_t *file)
 {
     (void)inode; (void)file;
     return UIOX_FS_OK;
 }
 
 static int scfs_readdir(uiox_file_t   *file,
                          uiox_dirent_t *de,
                          uint32_t       idx)
 {
     if (!file || !de) return UIOX_FS_EINVAL;
 
     uint32_t found = 0u;
     for (uint32_t i = 0u; i < s_scfs.disk_sb.inode_count; i++) {
         scfs_inode_disk_t *di = &s_scfs.inodes[i];
         if (di->ino == 0u) continue;
         if (found == idx) {
             de->d_ino  = di->ino;
             de->d_type = di->type;
             memcpy(de->d_name, di->name, SCFS_NAME_MAX);
             de->d_name[SCFS_NAME_MAX] = '\0';
             return UIOX_FS_OK;
         }
         found++;
     }
     return UIOX_FS_ENOENT;
 }
 
 static const uiox_file_ops_t s_scfs_fops = {
     .read      = scfs_read,
     .write     = scfs_write,
     .open      = scfs_open,
     .close     = scfs_close,
     .mmap_page = scfs_mmap_page,
     .readdir   = scfs_readdir,
     .fsync     = (void *)0,
     .ioctl     = (void *)0,
     .seek      = (void *)0,
 };
 
 /* ── SCFS inode operations ─────────────────────────────────────────── */
 static int scfs_lookup(uiox_inode_t  *dir,
                         const char    *name,
                         uiox_inode_t **out)
 {
     (void)dir;
     for (uint32_t i = 0u; i < s_scfs.disk_sb.inode_count; i++) {
         scfs_inode_disk_t *di = &s_scfs.inodes[i];
         if (di->ino == 0u) continue;
         uint32_t j = 0u;
         while (di->name[j] && name[j] &&
                di->name[j] == name[j]) j++;
         if (di->name[j] == '\0' && name[j] == '\0') {
             /* Found — fill VFS inode */
             uiox_inode_t *inode = (uiox_inode_t *)0;
             /* use static inode table via VFS layer */
             /* For simplicity, return pointer into our priv */
             /* In production: call inode_alloc() */
             static uiox_inode_t s_vfs_inodes[SCFS_INODE_MAX];
             inode = &s_vfs_inodes[i];
             inode->i_ino     = di->ino;
             inode->i_size    = di->size;
             inode->i_mode    = di->type;
             inode->i_fops    = &s_scfs_fops;
             inode->i_private = di;
             *out = inode;
             return UIOX_FS_OK;
         }
     }
     return UIOX_FS_ENOENT;
 }
 
 static int scfs_stat(uiox_inode_t *inode, uiox_stat_t *out)
 {
     memset(out, 0, sizeof(*out));
     out->st_ino  = inode->i_ino;
     out->st_mode = inode->i_mode;
     out->st_size = inode->i_size;
     return UIOX_FS_OK;
 }
 
 static const uiox_inode_ops_t s_scfs_iops = {
     .lookup   = scfs_lookup,
     .stat     = scfs_stat,
     .create   = (void *)0,   /* read-only FS */
     .mkdir    = (void *)0,
     .unlink   = (void *)0,
     .rmdir    = (void *)0,
     .rename   = (void *)0,
     .truncate = (void *)0,
 };
 
 /* ── SCFS superblock operations ────────────────────────────────────── */
 static int scfs_mount(uiox_superblock_t *sb, uint32_t dev_id)
 {
     (void)dev_id;
 
     /* Read superblock from block 0 */
     scfs_read_block(0u, &s_scfs.disk_sb);
 
     if (s_scfs.disk_sb.magic != SCFS_MAGIC) {
         early_puts("[scfs] bad magic — no SCFS on device\n");
         return UIOX_FS_EINVAL;
     }
 
     /* Read inode table from block 1 */
     scfs_read_block(1u, s_scfs.inodes);
     s_scfs.loaded = 1u;
 
     /* Clear page cache */
     memset(s_page_cache, 0, sizeof(s_page_cache));
 
     /* Create root inode */
     static uiox_inode_t s_root_inode;
     memset(&s_root_inode, 0, sizeof(s_root_inode));
     s_root_inode.i_ino   = 1u;
     s_root_inode.i_mode  = UIOX_DT_DIR;
     s_root_inode.i_ops   = &s_scfs_iops;
     s_root_inode.i_fops  = &s_scfs_fops;
 
     sb->s_root      = &s_root_inode;
     sb->s_blocksize = SCFS_BLOCK_SIZE;
     sb->s_private   = &s_scfs;
 
     early_puts("[scfs] mounted — blocks=");
     return UIOX_FS_OK;
 }
 
 static int scfs_unmount(uiox_superblock_t *sb)
 {
     (void)sb;
     s_scfs.loaded = 0u;
     return UIOX_FS_OK;
 }
 
 static int scfs_sync(uiox_superblock_t *sb)
 {
     (void)sb;
     /* Read-only FS — nothing to sync */
     return UIOX_FS_OK;
 }
 
 static const uiox_fs_ops_t s_scfs_ops = {
     .name    = "scfs",
     .mount   = scfs_mount,
     .unmount = scfs_unmount,
     .sync    = scfs_sync,
     .statfs  = (void *)0,
 };
 
 /* ── scfs_init — called from vfs_init sequence ─────────────────────── */
 void scfs_init(void)
 {
     vfs_register_fs(&s_scfs_ops);
     early_puts("[scfs] registered\n");
 }
 