/*
 * 30_KIX/32_FS/10_unfs/src/unfs_io.c
 *
 * UNFS block-device I/O — see unfs_io.h for the contract and for why the
 * unit conversion matters.
 *
 * ── BACKED BY 00_buffcache, NOT BY A RAW DEVICE ───────────────────────
 * Every access goes through bread() / getblk() / bwrite() / brelse(), so
 * the mkfs writer and the kernel share one cache and one write path.
 * bwrite's signature is
 *
 *     void bwrite(BufHdr *buf, bool sync, bool delayed);
 *
 * — it takes a POINTER to a buffer and returns void, so the write is
 * issued on the header, not as (dev, blkno, data).  An earlier attempt to
 * call it as (dev, blkno, data) produced three -Werror diagnostics:
 * pointer-from-integer, the always-true comparison, and "void value not
 * ignored".  The shape below is the one the header declares.
 *
 * ── the conversion ────────────────────────────────────────────────────
 * One UNFS block is UNFS_SECTORS_PER_BLOCK (8) bcache buffers, because
 * BLOCK_SIZE == BCACHE_SECTOR_SIZE.  Both directions loop over the
 * sectors and place them at the right offset inside the caller's 4096
 * byte buffer.  A single-buffer implementation would move one eighth of
 * each block and corrupt the volume silently.
 *
 * @version 1.0.0  @date 2026-09-27
 */

 #include "unfs_io.h"

 #include "bcache.h"         /* BufHdr, bread, getblk, bwrite, brelse */
 #include "uiox_base_types.h"
 
 /* BCACHE_SECTOR_SIZE is what BLOCK_SIZE expands to (bcache_types.h:173).
  * Read it from bcache rather than restating 512 here — if the buffer
  * layer ever changes size, this file follows automatically. */
 #define UNFS_IO_SECTOR_SIZE   ((uiox_uint32_t)BLOCK_SIZE)
 
 /* ── unfs_bdev_read ───────────────────────────────────────────────────
  * Read one 4096-byte UNFS block into buf, sector by sector.
  *
  * On a mid-block failure the caller's buffer is left partially filled;
  * the return code is what the caller must trust.  Marking it invalid
  * rather than zeroing avoids a caller mistaking a half-read block for a
  * real one.
  * ─────────────────────────────────────────────────────────────────── */
 int unfs_bdev_read(uiox_uint32_t dev, uiox_uint32_t blk, void *buf)
 {
     uiox_uint8_t *dst = (uiox_uint8_t *)buf;
     uiox_uint32_t s;
     uiox_uint32_t base;
 
     if (!buf) return UNFS_EIO;
 
     base = blk * UNFS_SECTORS_PER_BLOCK;
 
     for (s = 0u; s < (uiox_uint32_t)UNFS_SECTORS_PER_BLOCK; s++) {
         BufHdr *bp = bread((uiox_uint8_t)dev, base + s);
         if (!bp) return UNFS_EIO;
 
         for (uiox_uint32_t k = 0u; k < UNFS_IO_SECTOR_SIZE; k++)
             dst[(s * UNFS_IO_SECTOR_SIZE) + k] = bp->data[k];
 
         brelse(bp);
     }
 
     return UNFS_OK;
 }
 
 /* ── unfs_bdev_write ──────────────────────────────────────────────────
  * Write one 4096-byte UNFS block from buf, sector by sector.
  *
  * The buffer is obtained with getblk so the copy goes into the cache's
  * own storage, then bwrite(buf, true, false) issues a SYNCHRONOUS write
  * and releases it.  Synchronous is the right choice here: this is mkfs,
  * where a write that silently stayed in the cache and was never flushed
  * would leave a volume that looks formatted and is not.
  *
  * If getblk returns NULL the pool is exhausted (Bach's scenario 4/5 with
  * the bound reached) and unfs_format.c turns that into UNFS_EIO.
  * ─────────────────────────────────────────────────────────────────── */
 int unfs_bdev_write(uiox_uint32_t dev, uiox_uint32_t blk, const void *buf)
 {
     const uiox_uint8_t *src = (const uiox_uint8_t *)buf;
     uiox_uint32_t s;
     uiox_uint32_t base;
 
     if (!buf) return UNFS_EIO;
 
     base = blk * UNFS_SECTORS_PER_BLOCK;
 
     for (s = 0u; s < (uiox_uint32_t)UNFS_SECTORS_PER_BLOCK; s++) {
         BufHdr *bp = getblk((uiox_uint8_t)dev, base + s);
         if (!bp) return UNFS_EIO;
 
         for (uiox_uint32_t k = 0u; k < UNFS_IO_SECTOR_SIZE; k++)
             bp->data[k] = src[(s * UNFS_IO_SECTOR_SIZE) + k];
 
         /* sync = true, delayed = false: write now and release. */
         bwrite(bp, true, false);
     }
 
     return UNFS_OK;
 }
 
 /* ── unfs_crc32 ───────────────────────────────────────────────────────
  * Reflected CRC-32, polynomial 0xEDB88320, init 0xFFFFFFFF, final XOR.
  *
  * MUST AGREE WITH THE BOOTLOADER.  unfs_format.c stores its result in
  * s_sb_checksum and 01_uBoot verifies that field before mounting.  If the
  * bootloader's unfs_crc32c is the Castagnoli variant instead (polynomial
  * 0x82F63B78), change UNFS_CRC32_POLY below and nothing else — the rest
  * of the reflected algorithm is identical.
  *
  * The table is built on first use and never rebuilt; a static flag keeps
  * it out of .bss initialisation ordering concerns.
  * ─────────────────────────────────────────────────────────────────── */
 #define UNFS_CRC32_POLY   0xEDB88320u
 
 static uiox_uint32_t unfs_crc32_tab[256];
 static uiox_uint8_t  unfs_crc32_ready = 0u;
 
 static void unfs_crc32_init(void)
 {
     uiox_uint32_t i;
     uiox_uint32_t c;
     uiox_uint32_t k;
 
     for (i = 0u; i < 256u; i++) {
         c = i;
         for (k = 0u; k < 8u; k++)
             c = (c & 1u) ? (UNFS_CRC32_POLY ^ (c >> 1)) : (c >> 1);
         unfs_crc32_tab[i] = c;
     }
 
     unfs_crc32_ready = 1u;
 }
 
 uiox_uint32_t unfs_crc32(const void *data, uiox_uint32_t len)
 {
     const uiox_uint8_t *p = (const uiox_uint8_t *)data;
     uiox_uint32_t crc = 0xFFFFFFFFu;
     uiox_uint32_t i;
 
     if (!data) return 0u;
 
     if (!unfs_crc32_ready) unfs_crc32_init();
 
     for (i = 0u; i < len; i++)
         crc = unfs_crc32_tab[(crc ^ p[i]) & 0xFFu] ^ (crc >> 8);
 
     return crc ^ 0xFFFFFFFFu;
 }
 