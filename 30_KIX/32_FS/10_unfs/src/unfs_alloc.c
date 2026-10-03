/*
 * 30_KIX/32_FS/10_unfs/src/unfs_alloc.c
 *
 * UNFS block and inode allocation over the group bitmaps.
 * See unfs_alloc.h for the bit convention and the ordering rules.
 *
 * ── the layout this reads ─────────────────────────────────────────────
 * unfs_format.c writes, for group 0:
 *
 *     UNFS_GROUP0_DESC   258   one unfs_group_desc_t per group
 *     UNFS_GROUP0_BBMAP  259   block bitmap, 1 bit per block, 1 = FREE
 *     UNFS_GROUP0_IBMAP  260   inode bitmap, same convention
 *     UNFS_GROUP0_ITABLE 261   inode table
 *     UNFS_GROUP0_DATA   269   first data block
 *
 * The superblock carries s_blocks_per_group, s_inodes_per_group and the
 * counts.  Everything below derives its bounds from those, so a volume
 * laid out by a future mkfs with different group sizes still works.
 *
 * ── bits per bitmap block ─────────────────────────────────────────────
 * 4096 bytes = 32768 bits, so one bitmap block describes 32768 blocks.
 * UNFS_BLOCKS_PER_GROUP is 8192, well inside one block, which is why
 * mkfs writes exactly one bitmap block per group.  The scan below still
 * bounds itself by s_blocks_per_group rather than assuming, so a larger
 * group size would be caught rather than silently truncated.
 *
 * @version 1.0.0  @date 2026-09-27
 */

 #include "unfs_alloc.h"
 #include "unfs_io.h"
 #include "bcache.h"
 #include "uiox_kix_jrnl.h"
 
 /* One bitmap block covers this many blocks. */
 #define UNFS_BITS_PER_BITMAP_BLOCK  (UNFS_BLOCK_SIZE * 8u)
 
 /* ─────────────────────────────────────────────────────────────────────
  * bitmap helpers — the convention from unfs_format.c, repeated with the
  * same polarity so a reader comparing the two files sees identical code.
  * ───────────────────────────────────────────────────────────────────── */
 static int bit_is_free(const uiox_uint8_t *bm, uiox_uint32_t bit)
 {
     return (bm[bit >> 3] & (uiox_uint8_t)(1u << (bit & 7u))) ? 1 : 0;
 }
 
 static void bit_set_free(uiox_uint8_t *bm, uiox_uint32_t bit)
 {
     bm[bit >> 3] |= (uiox_uint8_t)(1u << (bit & 7u));
 }
 
 static void bit_set_used(uiox_uint8_t *bm, uiox_uint32_t bit)
 {
     bm[bit >> 3] &= (uiox_uint8_t)~(1u << (bit & 7u));
 }
 
 /* ─────────────────────────────────────────────────────────────────────
  * The superblock, re-read per call.
  *
  * No cache here on purpose.  A cached copy would have to be invalidated
  * from unfs_io.c's write path, or two mounts would disagree about
  * s_free_blocks; reading it per allocation costs one buffer-cache hit
  * (already resident) and removes a whole class of staleness bug.  If
  * profiling ever says otherwise, the cache belongs in unfs_io.c next to
  * the writes, not here.
  * ───────────────────────────────────────────────────────────────────── */
 static int read_sb(uiox_uint32_t dev, unfs_sb_t *sb)
 {
     uiox_uint8_t blk0[UNFS_BLOCK_SIZE];
     uiox_uint32_t k;
     uiox_uint8_t *p = (uiox_uint8_t *)sb;
 
     if (unfs_bdev_read(dev, 0u, blk0) != UNFS_OK) return UNFS_EIO;
 
     for (k = 0u; k < (uiox_uint32_t)sizeof(*sb); k++)
         p[k] = blk0[UNFS_SB_OFFSET + k];
 
     if (sb->s_magic != UNFS_MAGIC) return UNFS_EIO;
     return UNFS_OK;
 }
 
 static int write_sb(uiox_uint32_t dev, const unfs_sb_t *sb)
 {
     unfs_sb_t tmp;
     uiox_uint8_t blk0[UNFS_BLOCK_SIZE];
     uiox_uint8_t *p;
     uiox_uint32_t k;
 
     if (unfs_bdev_read(dev, 0u, blk0) != UNFS_OK) return UNFS_EIO;
 
     /* The checksum covers UNFS_SB_SIZE bytes with s_sb_checksum
      * temporarily zero — the same procedure unfs_format.c uses, so a
      * volume written by mkfs and one updated here verify identically. */
     tmp = *sb;
     tmp.s_sb_checksum = 0u;
     p = (uiox_uint8_t *)&tmp;
     for (k = 0u; k < (uiox_uint32_t)sizeof(tmp); k++)
         blk0[UNFS_SB_OFFSET + k] = p[k];
 
     /* recompute over the block-sized image, as mkfs does */
     {
         uiox_uint8_t image[UNFS_SB_SIZE];
         uiox_uint32_t j;
         for (j = 0u; j < UNFS_SB_SIZE && j < UNFS_BLOCK_SIZE; j++) image[j] = blk0[j];
         for (; j < UNFS_SB_SIZE; j++) image[j] = 0u;
         tmp.s_sb_checksum = unfs_crc32(image, UNFS_SB_SIZE);
     }
 
     p = (uiox_uint8_t *)&tmp;
     for (k = 0u; k < (uiox_uint32_t)sizeof(tmp); k++)
         blk0[UNFS_SB_OFFSET + k] = p[k];
 
     return unfs_bdev_write(dev, 0u, blk0);
 }
 
 /* ─────────────────────────────────────────────────────────────────────
  * Where one group's bitmap lives.
  *
  * Group 0's is the constant mkfs wrote.  Later groups are not laid out
  * yet — mkfs currently writes bitmaps only for group 0 and leaves the
  * rest zeroed — so the function derives the pattern mkfs would follow
  * rather than inventing an offset: each group owns
  * (1 descriptor + 2 bitmap blocks + inode-table blocks) after the last.
  *
  * Until mkfs writes more than one group, this returns group 0's block for
  * group > 0 as well, and the caller's s_group_count bound is what keeps
  * the arithmetic honest.  A single-group volume — which is what mkfs
  * produces today and what a 32 MiB target gets — never reaches the
  * multi-group branch.
  * ───────────────────────────────────────────────────────────────────── */
 static uiox_uint32_t group_bmap_blk(const unfs_sb_t *sb, uiox_uint32_t group)
 {
     if (group == 0u) return UNFS_GROUP0_BBMAP;
 
     /* per-group stride: bmap + ibmap + itable blocks, after the data area
      * of the previous group.  Derived from the superblock, not a literal. */
     {
         uiox_uint32_t stride = 2u + UNFS_ITABLE_BLOCKS;
         (void)sb;
         return UNFS_GROUP0_DATA
              + (group - 1u) * (stride + UNFS_ITABLE_BLOCKS)
              + stride;
     }
 }
 
 static uiox_uint32_t group_first_data_blk(const unfs_sb_t *sb, uiox_uint32_t group)
 {
     if (group == 0u) return UNFS_GROUP0_DATA;
     return group_bmap_blk(sb, group) + 2u + UNFS_ITABLE_BLOCKS;
 }
 
 /* ─────────────────────────────────────────────────────────────────────
  * The run scan — FIRST-FIT over set bits.
  *
  * Walks the bitmap from bit 0, counting consecutive set bits, and stops
  * at the first run of 'want'.  bits used beyond s_blocks_per_group are
  * not part of this group's address space and end the scan.
  * ───────────────────────────────────────────────────────────────────── */
 static int find_run(const uiox_uint8_t *bm, uiox_uint32_t limit,
                     uiox_uint32_t want, uiox_uint32_t *start_out)
 {
     uiox_uint32_t b = 0u;
     uiox_uint32_t run = 0u;
     uiox_uint32_t run_start = 0u;
 
     while (b < limit) {
         if (bit_is_free(bm, b)) {
             if (run == 0u) run_start = b;
             run++;
             if (run >= want) { *start_out = run_start; return 1; }
         } else {
             run = 0u;
         }
         b++;
     }
     return 0;
 }
 
 /* ═════════════════════════════════════════════════════════════════════
  * unfs_alloc_run
  * ═════════════════════════════════════════════════════════════════════ */
 int unfs_alloc_run(uiox_uint32_t dev, uiox_uint32_t want, unfs_run_t *out)
 {
     unfs_sb_t     sb;
     uiox_uint8_t  bmbuf[UNFS_BLOCK_SIZE];
     uiox_uint8_t  prebuf[UNFS_BLOCK_SIZE];//for journal
     uiox_uint32_t group;
     uiox_uint32_t start;
     uiox_uint32_t limit;
     uiox_uint32_t k;
     uiox_jr_ctx_t *jr = uiox_jr_ctx_for((uiox_uint8_t)dev);
 
     if (!out) return UNFS_EINVAL;
     if (want == 0u) { out->count = 0u; return UNFS_EINVAL; }
 
     out->first_blk = 0u;
     out->count     = 0u;
     out->group     = 0u;
 
     if (read_sb(dev, &sb) != UNFS_OK) return UNFS_EIO;
 
     if (sb.s_blocks_per_group > UNFS_BITS_PER_BITMAP_BLOCK) return UNFS_ENOTSUP;
 
     for (group = 0u; group < sb.s_group_count; group++) {
         uiox_uint32_t bmap_blk = group_bmap_blk(&sb, group);
         uiox_uint32_t first_data = group_first_data_blk(&sb, group);
 
         if (unfs_bdev_read(dev, bmap_blk, bmbuf) != UNFS_OK) return UNFS_EIO;

          /* Capture the PRE-image HERE, before the bits move.  This is the
          * only moment the block's on-disk contents exist in the local:
          * bit_set_used() below modifies bmbuf in place, so a capture
          * after the loop records the POST-image and the journal then
          * describes nothing about what changed. */
         if (jr) memcpy(prebuf, bmbuf, UNFS_BLOCK_SIZE);
 
         /* how many bits this group actually uses */
         limit = sb.s_blocks_per_group;
         if (group == sb.s_group_count - 1u) {
             uiox_uint32_t used = first_data + sb.s_blocks_per_group;
             if (used > (uiox_uint32_t)sb.s_block_count)
                 limit = (uiox_uint32_t)sb.s_block_count - first_data;
         }
 
         /* The first data block's offset within the bitmap is first_data
          * minus the group's own start.  mkfs marks the metadata ahead of
          * it used, so bit 0 corresponds to the group's first block. */
         if (!find_run(bmbuf, limit, want, &start)) continue;
 
         /* ── claim it: clear the bits, write the bitmap, then count ── */
         for (k = 0u; k < want; k++) bit_set_used(bmbuf, start + k);

         /* ── the pair, before the write lands ───────────────────────── */
         if (jr) {
                 uiox_jr_vfs_get_write_access(jr, bmap_blk, prebuf);
                 uiox_jr_vfs_dirty_metadata(jr, bmap_blk, bmbuf);
             }
     
 
         if (unfs_bdev_write(dev, bmap_blk, bmbuf) != UNFS_OK) return UNFS_EIO;
 
         if (sb.s_free_blocks >= (uiox_uint64_t)want)
             sb.s_free_blocks -= want;
         else
             sb.s_free_blocks = 0u;
 
         if (write_sb(dev, &sb) != UNFS_OK) return UNFS_EIO;
 
         out->first_blk = first_data + start;
         out->count     = want;
         out->group     = group;
         return UNFS_OK;
     }
 
     return UNFS_ENOSPC;
 }
 
 /* ═════════════════════════════════════════════════════════════════════
  * unfs_alloc_block — one block
  * ═════════════════════════════════════════════════════════════════════ */
 uiox_uint32_t unfs_alloc_block(uiox_uint32_t dev)
 {
     unfs_run_t r;
 
     if (unfs_alloc_run(dev, 1u, &r) != UNFS_OK) return 0u;
     return r.first_blk;
 }
 
 /* ═════════════════════════════════════════════════════════════════════
  * unfs_free_run
  * ═════════════════════════════════════════════════════════════════════ */
 int unfs_free_run(uiox_uint32_t dev, uiox_uint32_t first_blk,
                   uiox_uint32_t count)
 {
     unfs_sb_t     sb;
     uiox_uint8_t  bmbuf[UNFS_BLOCK_SIZE];
     uiox_uint8_t  prebuf[UNFS_BLOCK_SIZE]; //for journal
     uiox_uint32_t group;
     uiox_uint32_t start;
     uiox_uint32_t k;

     uiox_jr_ctx_t *jr = uiox_jr_ctx_for((uiox_uint8_t)dev);//for journal
 
     if (count == 0u) return UNFS_EINVAL;
     if (read_sb(dev, &sb) != UNFS_OK) return UNFS_EIO;
 
     /* which group, and the offset inside it */
     group = 0u;
     start = first_blk;
 
     for (group = 0u; group < sb.s_group_count; group++) {
         uiox_uint32_t first_data = group_first_data_blk(&sb, group);
         uiox_uint32_t last_data  = first_data + sb.s_blocks_per_group;
 
         if (first_blk >= first_data && first_blk < last_data) {
             start = first_blk - first_data;
             break;
         }
     }
     if (group >= sb.s_group_count) return UNFS_EINVAL;
 
     {
         uiox_uint32_t bmap_blk = group_bmap_blk(&sb, group);
 
         if (unfs_bdev_read(dev, bmap_blk, bmbuf) != UNFS_OK) return UNFS_EIO;

         if (jr) memcpy(prebuf, bmbuf, UNFS_BLOCK_SIZE); //for journal
 
         for (k = 0u; k < count; k++) {
             if (start + k >= sb.s_blocks_per_group) break;
             bit_set_free(bmbuf, start + k);
         }

         if (jr) { // for journal
            uiox_jr_vfs_get_write_access(jr, bmap_blk, prebuf);
            uiox_jr_vfs_dirty_metadata(jr, bmap_blk, bmbuf);
        }
 
         if (unfs_bdev_write(dev, bmap_blk, bmbuf) != UNFS_OK) return UNFS_EIO;
     }
 
     sb.s_free_blocks += count;
     return write_sb(dev, &sb);
 }
 
 /* ═════════════════════════════════════════════════════════════════════
  * unfs_group_free_blocks — count set bits
  *
  * The invariant check: s_free_blocks should equal the sum of this over
  * all groups (minus any blocks reserved past the last group's limit).
  * A disagreement means the ordering rules were violated somewhere.
  * ═════════════════════════════════════════════════════════════════════ */
 uiox_uint32_t unfs_group_free_blocks(uiox_uint32_t dev, uiox_uint32_t group)
 {
     unfs_sb_t     sb;
     uiox_uint8_t  bmbuf[UNFS_BLOCK_SIZE];
     uiox_uint32_t n = 0u;
     uiox_uint32_t b;
     uiox_uint32_t limit;
 
     if (read_sb(dev, &sb) != UNFS_OK) return 0u;
     if (group >= sb.s_group_count) return 0u;
 
     if (unfs_bdev_read(dev, group_bmap_blk(&sb, group), bmbuf) != UNFS_OK)
         return 0u;
 
     limit = sb.s_blocks_per_group;
     for (b = 0u; b < limit; b++)
         if (bit_is_free(bmbuf, b)) n++;
 
     return n;
 }
 
 /* ═════════════════════════════════════════════════════════════════════
  * Inode bitmap — same convention, same ordering.
  *
  * The inode table is UNFS_GROUP0_ITABLE with UNFS_INODE_BYTES per slot,
  * the geometry unfs_iget.c already uses.  Bit 0 is inode 1, so the
  * inode number for a bit is bit + 1 — and mkfs marks bit 0 and
  * UNFS_ROOT_INO-1 used, which is why the first allocatable bit is 1.
  * ═════════════════════════════════════════════════════════════════════ */
 int unfs_alloc_inode_bit(uiox_uint32_t dev, uiox_uint32_t *ino_out)
 {
     unfs_sb_t     sb;
     uiox_uint8_t  imbuf[UNFS_BLOCK_SIZE];
     uiox_uint8_t  prebuf[UNFS_BLOCK_SIZE];//for journal
     uiox_uint32_t b;
     uiox_uint32_t limit;

     uiox_jr_ctx_t *jr = uiox_jr_ctx_for((uiox_uint8_t)dev);
 
     if (!ino_out) return UNFS_EINVAL;
     *ino_out = 0u;
     if (read_sb(dev, &sb) != UNFS_OK) return UNFS_EIO;
 
     if (sb.s_inodes_per_group > UNFS_BITS_PER_BITMAP_BLOCK) return UNFS_ENOTSUP;
 
     if (unfs_bdev_read(dev, UNFS_GROUP0_IBMAP, imbuf) != UNFS_OK) return UNFS_EIO;

     if (jr) memcpy(prebuf, imbuf, UNFS_BLOCK_SIZE); //for journal
 
     limit = sb.s_inodes_per_group;
 
     for (b = 0u; b < limit; b++) {
         if (!bit_is_free(imbuf, b)) continue;
 
         bit_set_used(imbuf, b);
         //start journal 
         /* THIS ONE MATTERS AS MUCH AS THE BLOCK BITMAP.  ialloc() clears
          * the bit, then iget()s the inode and iupdate() writes it — the
          * same two-part update, and the same leak if a crash lands
          * between. */
         if (jr) {
                 uiox_jr_vfs_get_write_access(jr, UNFS_GROUP0_IBMAP, prebuf);
                 uiox_jr_vfs_dirty_metadata(jr, UNFS_GROUP0_IBMAP, imbuf);
        }


         // end ehe journal section
         if (unfs_bdev_write(dev, UNFS_GROUP0_IBMAP, imbuf) != UNFS_OK)
             return UNFS_EIO;
 
         if (sb.s_free_inodes > 0u) sb.s_free_inodes--;
         if (write_sb(dev, &sb) != UNFS_OK) return UNFS_EIO;
 
         *ino_out = b + 1u;
         return UNFS_OK;
     }
 
     return UNFS_ENOSPC;
 }
 
 int unfs_free_inode_bit(uiox_uint32_t dev, uiox_uint32_t ino)
 {
     unfs_sb_t     sb;
     uiox_uint8_t  imbuf[UNFS_BLOCK_SIZE];
     uiox_uint8_t  prebuf[UNFS_BLOCK_SIZE]; // for journal
     uiox_uint32_t bit;

     uiox_jr_ctx_t *jr = uiox_jr_ctx_for((uiox_uint8_t)dev); // for journal
 
     if (ino == 0u || ino == UNFS_NIL_INO) return UNFS_EINVAL;
     if (read_sb(dev, &sb) != UNFS_OK) return UNFS_EIO;
 
     bit = ino - 1u;
     if (bit >= sb.s_inodes_per_group) return UNFS_EINVAL;
 
     if (unfs_bdev_read(dev, UNFS_GROUP0_IBMAP, imbuf) != UNFS_OK) return UNFS_EIO;

     if (jr) memcpy(prebuf, imbuf, UNFS_BLOCK_SIZE); // for journal
 
     bit_set_free(imbuf, bit); 

     if (jr) { //for journal
        uiox_jr_vfs_get_write_access(jr, UNFS_GROUP0_IBMAP, prebuf);
        uiox_jr_vfs_dirty_metadata(jr, UNFS_GROUP0_IBMAP, imbuf);
      }

     if (unfs_bdev_write(dev, UNFS_GROUP0_IBMAP, imbuf) != UNFS_OK) return UNFS_EIO;
 
     sb.s_free_inodes++;
     return write_sb(dev, &sb);
 }
 