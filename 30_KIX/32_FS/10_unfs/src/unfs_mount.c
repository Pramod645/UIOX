/*
 * 30_KIX/32_FS/10_unfs/src/unfs_mount.c
 *
 * UNFS — mount.  Reads a volume's superblock and group descriptors and
 * fills an unfs_fs_t.  Returns it; it does NOT register in a mount table.
 *
 * ── WHY THERE IS NO MOUNT TABLE HERE ──────────────────────────────────
 * The previous revision called mount_alloc(), mount_free() and indexed
 * mount_table[], and took a super_block_t *sb.  None of those exist:
 *
 *     grep -rn 'mount_t|NMOUNT|mount_table' 01_fsa/include/   → nothing
 *
 * That is the VFS-era API, and fs_types.h states the position plainly:
 * a compile error naming one of those types is the correct outcome, not
 * something to paper over.  Registration can come when something needs to
 * look a volume up by name; nothing does yet.
 *
 * ── WHY unfs_fs_t OWNS THE GROUPS, AND SuperBlock DOES NOT ────────────
 * SuperBlock is Bach's Ch.4 free-LIST model: free_blocks[50] with
 * free_block_idx as a cursor onto a linked list of block-number arrays.
 * UNFS stores no free list on disk — it stores per-group BITMAPS, which
 * unfs_alloc.c scans.  Had SuperBlock grown a groups[] array it would
 * also have kept free_blocks[], a field no UNFS volume can ever populate,
 * and every mount would have carried a second allocator that cannot work.
 *
 * So: unfs_sb_t is the on-disk superblock, unfs_fs_t is the in-memory
 * mount state including groups[], and unfs_alloc.c takes (dev, want).
 * Three things, no duplication, and no field that cannot be filled.
 *
 * ── NO FEATURE-WORD CHECK ─────────────────────────────────────────────
 * An earlier revision of this file called check_incompat() to test
 * s_feature_incompat against UNFS_SUPPORTED_INCOMPAT.  unfs_sb_t has no
 * feature field — the struct is 22 fields and none of them is one — so
 * that test had nothing to read.  UNFS_SUPPORTED_INCOMPAT still exists as
 * 0u in unfs_format.h, but with no field to mask it there is no check to
 * make.  When a feature word is added to the format, its check comes back
 * here.
 *
 * ── UNITS ─────────────────────────────────────────────────────────────
 * s_blocks_per_group and the descriptors are in UNFS blocks (4096).
 * unfs_io.c converts to bcache sectors (UNFS_SECTORS_PER_BLOCK = 8).
 * Nothing here touches a sector number.
 *
 * @version 3.0.0  @date 2026-09-27
 */

 #include "unfs_fs.h"        /* unfs_fs_t, UNFS_MAX_GROUPS            */
 #include "unfs_io.h"        /* unfs_bdev_read / _write, unfs_crc32   */
 #include "unfs_errno.h"
 
 /* ── errors the mount path adds ─────────────────────────────────────────
  * unfs_errno.h carries the numbered set; these three are this file's own
  * refusal reasons and are grouped with them so a caller can switch on the
  * cause rather than just "mount failed". */
 #ifndef UNFS_EBADMAGIC
 #define UNFS_EBADMAGIC  -110   /* not a UNFS volume                   */
 #endif
 #ifndef UNFS_EBADCRC
 #define UNFS_EBADCRC    -111   /* superblock checksum mismatch        */
 #endif
 #ifndef UNFS_EBADSB
 #define UNFS_EBADSB     -112   /* magic fine, geometry unusable       */
 #endif
 
 /* ─────────────────────────────────────────────────────────────────────
  * read_sb — block 0 into a unfs_sb_t
  *
  * UNFS_SB_OFFSET is 0: the superblock occupies block 0 IN FULL.  The
  * removed unfs_disk.h put it at byte 1024 and read the wrong 4 KB on
  * every mount, which is one of the three facts that got that header
  * deleted.
  * ───────────────────────────────────────────────────────────────────── */
 static int read_sb(uiox_uint32_t dev, unfs_sb_t *out)
 {
     uiox_uint8_t  blk0[UNFS_BLOCK_SIZE];
     uiox_uint8_t *p = (uiox_uint8_t *)out;
     uiox_uint32_t k;
 
     if (unfs_bdev_read(dev, 0u, blk0) != UNFS_OK) return UNFS_EIO;
 
     for (k = 0u; k < (uiox_uint32_t)sizeof(*out); k++)
         p[k] = blk0[UNFS_SB_OFFSET + k];
 
     return UNFS_OK;
 }
 
 /* ─────────────────────────────────────────────────────────────────────
  * verify_sb — magic, checksum, geometry
  *
  * The checksum is computed the way unfs_format.c writes it: over
  * UNFS_SB_SIZE bytes with s_sb_checksum held at zero.  A zero stored
  * value means "not checksummed" and is accepted, because a volume written
  * before checksums existed must still mount — that is the same rule
  * unfs_mount.c always had, kept deliberately.
  * ───────────────────────────────────────────────────────────────────── */
 static int verify_sb(unfs_sb_t *d)
 {
     uiox_uint8_t  image[UNFS_SB_SIZE];
     uiox_uint8_t *p = (uiox_uint8_t *)d;
     uiox_uint32_t stored;
     uiox_uint32_t calc;
     uiox_uint32_t k;
 
     if (d->s_magic != UNFS_MAGIC) return UNFS_EBADMAGIC;
 
     if (d->s_block_size != UNFS_BLOCK_SIZE) return UNFS_EBADSB;
     if (d->s_inode_size != UNFS_INODE_SIZE) return UNFS_EBADSB;
     if (d->s_block_count == 0u)             return UNFS_EBADSB;
     if (d->s_group_count == 0u)             return UNFS_EBADSB;
     if (d->s_group_count > UNFS_MAX_GROUPS) return UNFS_EBADSB;
 
     if (d->s_sb_checksum != 0u) {
         stored = d->s_sb_checksum;
         d->s_sb_checksum = 0u;
 
         for (k = 0u; k < UNFS_SB_SIZE; k++)
             image[k] = (k < (uiox_uint32_t)sizeof(*d)) ? p[k] : 0u;
 
         calc = unfs_crc32(image, UNFS_SB_SIZE);
         d->s_sb_checksum = stored;
 
         if (calc != stored) return UNFS_EBADCRC;
     }
 
     return UNFS_OK;
 }
 
 /* ─────────────────────────────────────────────────────────────────────
  * load_groups — read the descriptor table into unfs_fs_t.groups[]
  *
  * unfs_group_desc_t is 12 bytes, so 341 fit in a 4096-byte block.  mkfs
  * writes them from UNFS_GROUP0_DESC upward, one per group, packed — so
  * the block a descriptor lives in is (i / per_blk) and its offset within
  * that block is (i % per_blk) * 12.
  *
  * The descriptor carries THREE fields, the three the bootloader reads.
  * First block, block count, first inode and node count are DERIVED from
  * the layout constants — the same choice unfs_format.c made writing them,
  * so the two agree by construction rather than by convention.
  * ───────────────────────────────────────────────────────────────────── */
 static int load_groups(uiox_uint32_t dev, const unfs_sb_t *d,
                        unfs_fs_t *fs)
 {
     uiox_uint32_t per_blk = UNFS_BLOCK_SIZE / (uiox_uint32_t)sizeof(unfs_group_desc_t);
     uiox_uint32_t i;
 
     if (per_blk == 0u) return UNFS_EBADSB;
 
     for (i = 0u; i < d->s_group_count; i++) {
         uiox_uint8_t  gblock[UNFS_BLOCK_SIZE];
         uiox_uint32_t blk = UNFS_GROUP0_DESC + (i / per_blk);
         uiox_uint32_t off = (i % per_blk) * (uiox_uint32_t)sizeof(unfs_group_desc_t);
         uiox_uint8_t *src = gblock + off;
         uiox_uint8_t *dst = (uiox_uint8_t *)&fs->groups[i];
         uiox_uint32_t k;
 
         if (unfs_bdev_read(dev, blk, gblock) != UNFS_OK) return UNFS_EIO;
 
         for (k = 0u; k < (uiox_uint32_t)sizeof(fs->groups[i]); k++)
             dst[k] = src[k];
 
         /* Sanity: the three fields must be inside the volume.  A volume
          * whose descriptors point past the end would otherwise mount and
          * fail later, inside an allocation. */
         if (fs->groups[i].bg_block_bitmap == 0u) return UNFS_EBADSB;
         if (fs->groups[i].bg_block_bitmap >= (uiox_uint32_t)d->s_block_count)
             return UNFS_EBADSB;
         if (fs->groups[i].bg_inode_bitmap >= (uiox_uint32_t)d->s_block_count)
             return UNFS_EBADSB;
         if (fs->groups[i].bg_inode_table >= (uiox_uint32_t)d->s_block_count)
             return UNFS_EBADSB;
     }
 
     fs->n_groups = d->s_group_count;
     return UNFS_OK;
 }
 
 /* ═════════════════════════════════════════════════════════════════════
  * unfs_kern_mount — read a volume into an unfs_fs_t
  *
  * The caller owns the unfs_fs_t it passes in; this fills it.  On failure
  * nothing is left half-populated that a caller might trust: fs->n_groups
  * is set only after every descriptor has been validated, and fs->mounted
  * only at the end.
  * ═════════════════════════════════════════════════════════════════════ */
 int unfs_kern_mount(unfs_fs_t *fs, uiox_uint32_t dev)
 {
     int rc;
 
     if (!fs) return UNFS_EINVAL;
 
     fs->n_groups  = 0u;
     fs->mounted   = 0u;
     fs->cow_active = 0u;
     fs->n_snapshots = 0u;
     fs->dev_id    = dev;
 
     rc = read_sb(dev, &fs->disk);
     if (rc != UNFS_OK) return rc;
 
     rc = verify_sb(&fs->disk);
     if (rc != UNFS_OK) return rc;
 
     rc = load_groups(dev, &fs->disk, fs);
     if (rc != UNFS_OK) return rc;
 
     /* SuperBlock is 01_fsa's own and this file does not build one.
      * It stays null until something needs it — and bmap() does not,
      * because it walks extents rather than calling fs_alloc. */
     fs->sb = (SuperBlock *)0;
 
     fs->mounted = 1u;
     return UNFS_OK;
 }
 
 /* ═════════════════════════════════════════════════════════════════════
  * unfs_kern_unmount — mark the handle dead
  *
  * No free-list to flush and no buffer to release: every write this layer
  * makes goes through unfs_io.c, which issues it synchronously.  So
  * unmounting is clearing the handle, not draining state.  If a dirty
  * inode cache is ever added, the flush belongs here.
  * ═════════════════════════════════════════════════════════════════════ */
 int unfs_kern_unmount(unfs_fs_t *fs)
 {
     if (!fs) return UNFS_EINVAL;
     if (!fs->mounted) return UNFS_EINVAL;
 
     fs->mounted     = 0u;
     fs->n_groups    = 0u;
     fs->cow_active  = 0u;
     fs->n_snapshots = 0u;
 
     return UNFS_OK;
 }
 
 /* ═════════════════════════════════════════════════════════════════════
  * unfs_kern_sync — write the superblock back
  *
  * The only mutable on-disk state this layer owns is the superblock: the
  * block and inode bitmaps are written where they are changed, by
  * unfs_alloc.c, before any counter moves.  So sync has exactly one job.
  *
  * s_clean is set on mount in a journalled filesystem; here it is written
  * by mkfs and left, because without a journal there is nothing that a
  * crash-recovery pass would read it for.
  * ═════════════════════════════════════════════════════════════════════ */
 int unfs_kern_sync(unfs_fs_t *fs)
 {
     uiox_uint8_t  blk0[UNFS_BLOCK_SIZE];
     unfs_sb_t     tmp;
     uiox_uint8_t *p;
     uiox_uint32_t k;
 
     if (!fs) return UNFS_EINVAL;
     if (!fs->mounted) return UNFS_EINVAL;
 
     if (unfs_bdev_read(fs->dev_id, 0u, blk0) != UNFS_OK) return UNFS_EIO;
 
     /* checksum with the field zeroed, as mkfs and unfs_alloc.c do */
     tmp = fs->disk;
     tmp.s_sb_checksum = 0u;
     p = (uiox_uint8_t *)&tmp;
     for (k = 0u; k < (uiox_uint32_t)sizeof(tmp); k++)
         blk0[UNFS_SB_OFFSET + k] = p[k];
 
     {
         uiox_uint8_t image[UNFS_SB_SIZE];
         uiox_uint32_t j;
         for (j = 0u; j < UNFS_SB_SIZE; j++)
             image[j] = (j < UNFS_BLOCK_SIZE) ? blk0[j] : 0u;
         tmp.s_sb_checksum = unfs_crc32(image, UNFS_SB_SIZE);
     }
 
     p = (uiox_uint8_t *)&tmp;
     for (k = 0u; k < (uiox_uint32_t)sizeof(tmp); k++)
         blk0[UNFS_SB_OFFSET + k] = p[k];
 
     return unfs_bdev_write(fs->dev_id, 0u, blk0);
 }
 