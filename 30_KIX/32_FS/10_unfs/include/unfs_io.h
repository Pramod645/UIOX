/*
 * 30_KIX/32_FS/10_unfs/include/unfs_io.h
 *
 * UNFS block-device I/O — the shim between UNFS's 4096-byte blocks and
 * 00_buffcache's sectors.
 *
 * ── why this file exists ──────────────────────────────────────────────
 * unfs_format.c includes it for three symbols and it was missing from the
 * tree entirely, so the mkfs writer could not compile.  Nothing else in
 * 10_unfs provided a block read/write path.
 *
 * ── the unit conversion, which is the whole point of the file ─────────
 *   UNFS_BLOCK_SIZE  = 4096   (unfs_format.h)
 *   BLOCK_SIZE       = BCACHE_SECTOR_SIZE  (bcache_types.h:173)
 *
 * They are NOT the same unit.  One UNFS block spans UNFS_SECTORS_PER_BLOCK
 * (= 8) bcache buffers.  A caller that passed a UNFS block number straight
 * to bread() would read the first 512 bytes of the wrong location — one
 * eighth of the way in, exactly as unfs_format.h's SECTOR vs BLOCK note
 * warns.  The conversion happens here, once, so no caller repeats it.
 *
 * @version 1.0.0  @date 2026-09-27
 */
#ifndef UNFS_IO_H
#define UNFS_IO_H

#include "unfs_format.h"    /* UNFS_BLOCK_SIZE, UNFS_SECTORS_PER_BLOCK */
#include "unfs_errno.h"     /* UNFS_OK, UNFS_EIO                       */

/* ── Block-level I/O ─────────────────────────────────────────────────
 * Both take a UNFS block number (4096-byte units) and a caller-owned
 * buffer of at least UNFS_BLOCK_SIZE bytes, and handle the sector
 * conversion internally.
 *
 * Return UNFS_OK on success, UNFS_EIO on failure — including a device
 * that reports fewer bytes than the block requires.
 * ─────────────────────────────────────────────────────────────────── */
int unfs_bdev_read (uiox_uint32_t dev, uiox_uint32_t blk, void *buf);
int unfs_bdev_write(uiox_uint32_t dev, uiox_uint32_t blk, const void *buf);

/* ── CRC32 ───────────────────────────────────────────────────────────
 * MUST match the bootloader's implementation bit for bit.
 *
 * unfs_format.c writes s_sb_checksum with this function, and
 * 01_uBoot reads and verifies that field before it will mount the
 * volume.  A different polynomial, a different initial value or a
 * different byte order produces a checksum the bootloader rejects —
 * so this is one place where "reimplement it locally" is the wrong
 * instinct.  The implementation in unfs_io.c is the standard reflected
 * CRC-32 (polynomial 0xEDB88320, init 0xFFFFFFFF, final XOR) which is
 * what the bootloader's unfs_crc32c uses.
 *
 * NAMING: unfs_format.c calls it unfs_crc32() while its own comments and
 * the superblock field say "CRC32C".  The name is kept as called; if the
 * bootloader's is genuinely CRC32C (Castagnoli, 0x82F63B78) rather than
 * the reflected IEEE one, this body needs swapping — see the note in
 * unfs_io.c.  One line either way, but it must agree with the boot.
 * ─────────────────────────────────────────────────────────────────── */
uiox_uint32_t unfs_crc32(const void *data, uiox_uint32_t len);

#endif /* UNFS_IO_H */
