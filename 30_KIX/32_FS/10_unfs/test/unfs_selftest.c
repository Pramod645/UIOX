/*
 * 30_KIX/32_FS/10_unfs/test/unfs_selftest.c
 *
 * UNFS self-test — format a volume, mount it, and read it back.
 *
 * ── WHY THIS EXISTS ───────────────────────────────────────────────────
 * libfs.a compiles.  That proves the TYPES line up; it proves nothing
 * about the arithmetic.  The two places most likely to be wrong in a way
 * no compiler can see are:
 *
 *   1. the group offsets in unfs_alloc.c's group_bmap_blk() /
 *      group_first_data_blk() — derived, never executed
 *   2. the extent split in bmap_extent_place() — case 2 rewrites three
 *      entries and a wrong stride corrupts the map silently
 *
 * So this runs the stack: format, mount, iget, bmap_alloc, bmap, and
 * compares what comes back.
 *
 * ── THE DEVICE ────────────────────────────────────────────────────────
 * bcache's platform layer addresses memory directly:
 *
 *     plat_addr(dev, blkno) = bcache_plat_dram_base()
 *                           + dev * BCACHE_DEV_STRIDE_DEFAULT
 *                           + blkno * BCACHE_SECTOR_SIZE
 *
 * NO override is needed and NONE is attempted — unfs_io.c already owns
 * unfs_bdev_read/_write, and a second definition would fail the link.
 * This harness only CALLS.
 *
 * ── ONE TRAP THIS TEST GUARDS AGAINST ─────────────────────────────────
 * bcache_plat_write_block() DROPS a write past the device end and only
 * prints a line:
 *
 *     if (nblocks != 0u && blkno >= nblocks) { printf(...); return; }
 *
 * So a format/mount round-trip can appear to pass while a block never
 * landed.  Every check below therefore reads the block BACK and asserts
 * on content, rather than trusting the absence of an error.
 *
 * @version 1.0.0  @date 2026-09-27
 */

 #include "unfs_fs.h"
 #include "unfs_format.h"
 #include "unfs_io.h"
 #include "unfs_alloc.h"
 #include "unfs_errno.h"
 #include "inode.h"
 #include "bmap.h"
 #include "bcache.h"
 
 /* printf comes from the SoC stdio layer, as it does for bcache_init.c */
 #include "uiox_soc_stdio.h"
 
 /* ── test geometry ─────────────────────────────────────────────────────
  * UNFS_GROUP0_DATA (269) plus 256 data blocks.  Deliberately SMALLER
  * than one group, so it exercises the group-0 path mkfs actually writes
  * and never reaches the multi-group branch that mkfs does not produce.
  * A single-group volume is exactly what a 32 MiB target yields. */
 #define TEST_BLOCKS   (UNFS_GROUP0_DATA + 256u)
 
 #define TEST_DEV      0u
 
 /* ── pass/fail bookkeeping ─────────────────────────────────────────── */
 static int g_fail;
 
 static void check(int ok, const char *what)
 {
     if (ok) {
         printf("  [ ok ] %s\n", what);
     } else {
         printf("  [FAIL] %s\n", what);
         g_fail++;
     }
 }
 
 /* ─────────────────────────────────────────────────────────────────────
  * step 0 — what does the device layer actually look like?
  *
  * Printed rather than assumed.  If bcache_plat_num_blocks() reports a
  * count smaller than TEST_BLOCKS, every write past that point is DROPPED
  * by the platform hook and the test below would be meaningless.
  * ───────────────────────────────────────────────────────────────────── */
 static void report_device(void)
 {
     uint32_t nblocks = bcache_plat_num_blocks(TEST_DEV);
 
     printf("device %u: %u blocks of %u bytes = %u bytes\n",
            (unsigned)TEST_DEV,
            (unsigned)nblocks,
            (unsigned)BCACHE_SECTOR_SIZE,
            (unsigned)(nblocks * BCACHE_SECTOR_SIZE));
 
     printf("  dram base   = 0x%lx\n", (unsigned long)bcache_plat_dram_base());
     printf("  device size = %u sectors\n", (unsigned)BCACHE_DEV_STRIDE_DEFAULT / BCACHE_SECTOR_SIZE);
     printf("  test needs  = %u UNFS blocks = %u sectors\n",
            (unsigned)TEST_BLOCKS,
            (unsigned)TEST_BLOCKS * UNFS_SECTORS_PER_BLOCK);
     printf("\n");
 
     check(nblocks >= TEST_BLOCKS * UNFS_SECTORS_PER_BLOCK,
           "device is large enough for the test volume");
 }
 
 /* ─────────────────────────────────────────────────────────────────────
  * step 1 — format
  * ───────────────────────────────────────────────────────────────────── */
 static int do_format(uint32_t *root_ino_out)
 {
     int rc;
 
     printf("format %u blocks ...\n", (unsigned)TEST_BLOCKS);
     rc = unfs_format(TEST_DEV, TEST_BLOCKS, root_ino_out);
     check(rc == UNFS_OK, "unfs_format returned UNFS_OK");
     if (rc != UNFS_OK) printf("        rc = %d\n", rc);
     return rc;
 }
 
 /* ─────────────────────────────────────────────────────────────────────
  * step 2 — read block 0 back and check the superblock
  *
  * UNFS_SB_OFFSET is 0: the superblock occupies block 0 IN FULL.
  * ───────────────────────────────────────────────────────────────────── */
 static void check_superblock(void)
 {
     unfs_sb_t sb;
     uint8_t  *p = (uint8_t *)&sb;
     uint8_t   blk0[UNFS_BLOCK_SIZE];
     uint32_t  k;
     int       rc;
 
     rc = unfs_bdev_read(TEST_DEV, 0u, blk0);
     check(rc == UNFS_OK, "read back block 0");
     if (rc != UNFS_OK) return;
 
     for (k = 0u; k < (uint32_t)sizeof(sb); k++) p[k] = blk0[UNFS_SB_OFFSET + k];
 
     check(sb.s_magic == UNFS_MAGIC,             "s_magic == UNFS_MAGIC");
     check(sb.s_block_size == UNFS_BLOCK_SIZE,   "s_block_size == 4096");
     check(sb.s_inode_size == UNFS_INODE_SIZE,   "s_inode_size == 256");
     check(sb.s_block_count == TEST_BLOCKS,      "s_block_count matches");
     check(sb.s_group_count >= 1u,               "s_group_count >= 1");
     check(sb.s_free_blocks > 0u,                "s_free_blocks > 0");
 
     printf("        magic=0x%x groups=%u blocks=%u free=%u\n",
            (unsigned)sb.s_magic, (unsigned)sb.s_group_count,
            (unsigned)sb.s_block_count, (unsigned)sb.s_free_blocks);
 }
 
 /* ─────────────────────────────────────────────────────────────────────
  * step 3 — mount
  * ───────────────────────────────────────────────────────────────────── */
 static void do_mount(unfs_fs_t *fs)
 {
     int rc;
 
     printf("\nmount ...\n");
     rc = unfs_kern_mount(fs, TEST_DEV);
     check(rc == UNFS_OK, "unfs_kern_mount returned UNFS_OK");
     if (rc != UNFS_OK) { printf("        rc = %d\n", rc); return; }
 
     check(fs->mounted == 1u,                "fs->mounted set");
     check(fs->n_groups == fs->disk.s_group_count, "n_groups matches the superblock");
     check(fs->groups[0].bg_block_bitmap == UNFS_GROUP0_BBMAP,
           "group 0 block bitmap at UNFS_GROUP0_BBMAP (259)");
     check(fs->groups[0].bg_inode_table == UNFS_GROUP0_ITABLE,
           "group 0 inode table at UNFS_GROUP0_ITABLE (261)");
 
     printf("        groups=%u bmap=%u ibmap=%u itable=%u\n",
            (unsigned)fs->n_groups,
            (unsigned)fs->groups[0].bg_block_bitmap,
            (unsigned)fs->groups[0].bg_inode_bitmap,
            (unsigned)fs->groups[0].bg_inode_table);
 }
 
 /* ─────────────────────────────────────────────────────────────────────
  * step 4 — read the root inode
  *
  * The root is a DIRECTORY: i_mode must carry UNFS_IFDIR (0040000).  This
  * is also the check that would catch the FT_DIR / UNFS_IFCHR collision
  * an earlier revision had — a directory reporting as a block device.
  * ───────────────────────────────────────────────────────────────────── */
 static void check_root_inode(void)
 {
     unfs_inode_t ri;
     int          rc;
 
     printf("\nroot inode %u ...\n", (unsigned)UNFS_ROOT_INO);
     rc = unfs_read_inode(TEST_DEV, UNFS_ROOT_INO, &ri);
     check(rc == UNFS_OK, "unfs_read_inode returned UNFS_OK");
     if (rc != UNFS_OK) { printf("        rc = %d\n", rc); return; }
 
     check((ri.i_mode & UNFS_IFMT) == UNFS_IFDIR, "i_mode is a DIRECTORY");
     check(ri.i_nlink >= 2u,                      "i_nlink >= 2 ('.' and '..')");
     check(ri.i_size == UNFS_BLOCK_SIZE,          "i_size == one block");
     check(ri.i_extents[0].e_len == 1u,           "extent 0 has length 1");
     check(ri.i_extents[0].e_physical >= UNFS_GROUP0_DATA,
           "extent 0 points at a data block");
     check((ri.i_extents[0].e_flags & UNFS_EXT_LEAF) != 0u,
           "extent 0 is a LEAF");
 
     printf("        mode=0%o nlink=%u size=%u ext0: lb=%u pb=%u len=%u flags=0x%x\n",
            (unsigned)ri.i_mode, (unsigned)ri.i_nlink, (unsigned)ri.i_size,
            (unsigned)ri.i_extents[0].e_logical,
            (unsigned)ri.i_extents[0].e_physical,
            (unsigned)ri.i_extents[0].e_len,
            (unsigned)ri.i_extents[0].e_flags);
 }
 
 /* ─────────────────────────────────────────────────────────────────────
  * step 5 — the root directory's two entries
  *
  * namei.h: the record header is EIGHT bytes and d_rec_len is the walk's
  * authority, rounded up to UNFS_DIRENT_ALIGN (4).  A record of 10 would
  * be misaligned and every entry after it would be parsed from the wrong
  * offset — which is what the unfs_format.c fix corrects.
  * ───────────────────────────────────────────────────────────────────── */
 static void check_root_dirents(const unfs_inode_t *ri)
 {
     uint8_t       dbuf[UNFS_BLOCK_SIZE];
     unfs_dirent_t *d0;
     unfs_dirent_t *d1;
     uint32_t      pb;
     int           rc;
 
     if (!ri || ri->i_extents[0].e_len == 0u) return;
 
     pb = ri->i_extents[0].e_physical;
 
     printf("\nroot dirents (data block %u) ...\n", (unsigned)pb);
     rc = unfs_bdev_read(TEST_DEV, pb, dbuf);
     check(rc == UNFS_OK, "read the root data block");
     if (rc != UNFS_OK) return;
 
     d0 = (unfs_dirent_t *)dbuf;
     check(d0->d_ino == UNFS_ROOT_INO,          "'.' names the root inode");
     check(d0->d_name_len == 1u,                "'.' name length 1");
     check(d0->d_name[0] == '.',                "'.' is a dot");
     check((d0->d_rec_len % 4u) == 0u,          "'.' record is 4-byte ALIGNED");
     check(d0->d_rec_len >= 12u,                "'.' record >= 12 (8 header + name)");
 
     d1 = (unfs_dirent_t *)(dbuf + d0->d_rec_len);
     check(d1->d_name_len == 2u,                "'..' name length 2");
     check(d1->d_name[0] == '.' && d1->d_name[1] == '.', "'..' is two dots");
     check((d0->d_rec_len + d1->d_rec_len) == UNFS_BLOCK_SIZE,
           "the two records cover the block exactly");
 
     printf("        '.':  ino=%u reclen=%u\n", (unsigned)d0->d_ino, (unsigned)d0->d_rec_len);
     printf("        '..': ino=%u reclen=%u\n", (unsigned)d1->d_ino, (unsigned)d1->d_rec_len);
 }
 
 /* ─────────────────────────────────────────────────────────────────────
  * step 6 — the allocator
  *
  * unfs_alloc_run() scans the group bitmap for SET bits (1 = FREE) and
  * clears them.  Every allocated block must be >= UNFS_GROUP0_DATA, since
  * mkfs marks everything below it used.
  * ───────────────────────────────────────────────────────────────────── */
 static void check_alloc(void)
 {
     unfs_run_t    run;
     unfs_run_t    run2;
     uint32_t      free_before;
     int           rc;
 
     printf("\nallocator ...\n");
 
     free_before = unfs_group_free_blocks(TEST_DEV, 0u);
     printf("        group 0 free blocks (counted) = %u\n", (unsigned)free_before);
     check(free_before > 0u, "group 0 has free blocks");
 
     rc = unfs_alloc_run(TEST_DEV, 1u, &run);
     check(rc == UNFS_OK, "unfs_alloc_run(1) returned UNFS_OK");
     if (rc != UNFS_OK) return;
 
     check(run.count == 1u,                    "run.count == 1");
     check(run.first_blk >= UNFS_GROUP0_DATA,  "run is in the DATA area");
     check(run.first_blk < TEST_BLOCKS,        "run is inside the volume");
     printf("        allocated blk %u (group %u)\n",
            (unsigned)run.first_blk, (unsigned)run.group);
 
     /* a second allocation must NOT return the same block */
     rc = unfs_alloc_run(TEST_DEV, 1u, &run2);
     check(rc == UNFS_OK,                "second unfs_alloc_run returned UNFS_OK");
     check(run2.first_blk != run.first_blk,
           "the second allocation is a DIFFERENT block");
     printf("        allocated blk %u (group %u)\n",
            (unsigned)run2.first_blk, (unsigned)run2.group);
 
     /* the count must have dropped by two */
     check(unfs_group_free_blocks(TEST_DEV, 0u) == free_before - 2u,
           "bitmap free count dropped by exactly 2");
 }
 
 /* ─────────────────────────────────────────────────────────────────────
  * step 7 — bmap on the root inode, then the extent write ops
  * ───────────────────────────────────────────────────────────────────── */
 static void check_bmap(void)
 {
     unfs_inode_t ri;
     BmapResult   r;
     int          rc;
 
     printf("\nbmap ...\n");
 
     rc = unfs_read_inode(TEST_DEV, UNFS_ROOT_INO, &ri);
     if (rc != UNFS_OK) { check(0, "read root inode for bmap"); return; }
 
     /* bmap() takes an InCoreInode.  Fill one from the disk image so the
      * call path is the real one, not a hand-built struct. */
     {
         InCoreInode ip;
         uint32_t    k;
 
         for (k = 0u; k < (uint32_t)sizeof(ip); k++) ((uint8_t *)&ip)[k] = 0u;
         ip.ino    = UNFS_ROOT_INO;
         ip.dev    = (uint8_t)TEST_DEV;
         ip.size   = (uint32_t)ri.i_size;
         ip.i_extent_tree = ri.i_extent_tree;
         for (k = 0u; k < UNFS_INLINE_EXTENTS; k++) ip.i_extents[k] = ri.i_extents[k];
 
         /* offset 0 is the first entry of the root directory */
         r = bmap(&ip, 0u);
         check(r.valid,                    "bmap(root, 0) is valid");
         check(r.blkno == ri.i_extents[0].e_physical,
               "bmap returns the extent's physical block");
         check(r.blk_offset == 0u,         "offset 0 -> blk_offset 0");
         check(r.io_bytes == UNFS_BLOCK_SIZE, "io_bytes is a whole block");
         printf("        bmap: blkno=%u off=%u io=%u valid=%d\n",
                (unsigned)r.blkno, (unsigned)r.blk_offset,
                (unsigned)r.io_bytes, (int)r.valid);
 
         /* an offset past the single mapped block must NOT be valid */
         r = bmap(&ip, UNFS_BLOCK_SIZE);
         check(!r.valid, "bmap past EOF is NOT valid");
 
         /* ── the extent write ops ──────────────────────────────────────
          * Case 1: logical block 0 is covered by a length-1 extent, so
          * placing a new block REPOINTS it.  Case 2's split cannot be
          * reached here without a multi-block extent, so it is not
          * exercised — stated rather than implied. */
         {
             int prc = bmap_extent_place(&ip, 0u, ri.i_extents[0].e_physical + 1u);
             check(prc == UNFS_OK, "bmap_extent_place case 1 returned UNFS_OK");
             check(ip.i_extents[0].e_physical == ri.i_extents[0].e_physical + 1u,
                   "case 1 repointed the extent");
         }
     }
 }
 
 /* ═════════════════════════════════════════════════════════════════════
  * main
  * ═════════════════════════════════════════════════════════════════════ */
 int main(void)
 {
     unfs_fs_t    fs;
     unfs_inode_t ri;
     uint32_t     root_ino = 0u;
     uint32_t     k;
 
     for (k = 0u; k < (uint32_t)sizeof(fs); k++) ((uint8_t *)&fs)[k] = 0u;
     for (k = 0u; k < (uint32_t)sizeof(ri); k++) ((uint8_t *)&ri)[k] = 0u;
 
     printf("\n=== UNFS self-test ===\n\n");
     printf("block=%u inode=%u sectors/block=%u\n",
            (unsigned)UNFS_BLOCK_SIZE, (unsigned)UNFS_INODE_SIZE,
            (unsigned)UNFS_SECTORS_PER_BLOCK);
     printf("layout: sb=0 desc=%u bmap=%u ibmap=%u itable=%u data=%u\n\n",
            (unsigned)UNFS_GROUP0_DESC, (unsigned)UNFS_GROUP0_BBMAP,
            (unsigned)UNFS_GROUP0_IBMAP, (unsigned)UNFS_GROUP0_ITABLE,
            (unsigned)UNFS_GROUP0_DATA);
 
     report_device();
 
     printf("\nbcache init ...\n");
     bcache_init();
     check(1, "bcache_init returned");
 
     if (do_format(&root_ino) != UNFS_OK) {
         printf("\nRESULT: FAIL (format)\n");
         return 1;
     }
     check(root_ino == UNFS_ROOT_INO, "unfs_format reported UNFS_ROOT_INO");
 
     check_superblock();
 
     do_mount(&fs);
     if (!fs.mounted) { printf("\nRESULT: FAIL (mount)\n"); return 1; }
 
     check_root_inode();
     if (unfs_read_inode(TEST_DEV, UNFS_ROOT_INO, &ri) == UNFS_OK)
         check_root_dirents(&ri);
 
     check_alloc();
     check_bmap();
 
     printf("\n=== RESULT: %s (%d failure%s) ===\n\n",
            g_fail == 0 ? "PASS" : "FAIL", g_fail, g_fail == 1 ? "" : "s");
 
     return g_fail == 0 ? 0 : 1;
 }
 