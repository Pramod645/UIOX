/**
 * @file  uiox_jrnl_io.h
 * @brief UIOX Filesystem Journaling — block I/O abstraction.
 *
 * Decouples the journal engine from the concrete block device driver.
 * The BSP / buffer-cache layer implements the weak-symbol hooks below.
 *
 * @version 1.0.0
 * @date    2026-07-08
 */
#ifndef UIOX_JRNL_IO_H
#define UIOX_JRNL_IO_H

#include "uiox_jrnl_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * Platform I/O hooks — implement in 31_BufferCache or BSP
 * ====================================================================== */

/**
 * @brief Read one journal block (@blocknr) into @buf (UIOX_JNL_BLOCK_SIZE).
 *        Production: call buf_cache_read() / bread().
 */
__attribute__((weak))
uiox_jnl_err_t uiox_jnl_plat_read_block(uint32_t  blocknr,
                                          uint8_t  *buf);

/**
 * @brief Write one journal block synchronously.
 *        Production: call buf_cache_write() / bwrite() with B_SYNC.
 */
__attribute__((weak))
uiox_jnl_err_t uiox_jnl_plat_write_block(uint32_t        blocknr,
                                           const uint8_t  *buf);

/**
 * @brief Issue a barrier / flush to guarantee ordering.
 *        Production: blkdev_issue_flush() or equivalent.
 */
__attribute__((weak))
uiox_jnl_err_t uiox_jnl_plat_barrier(void);

/**
 * @brief Allocate a zeroed 4096-byte block buffer.
 *        Production: use slab/kmem allocator.
 */
__attribute__((weak))
uint8_t *uiox_jnl_plat_alloc_block(void);

/**
 * @brief Free a block buffer previously allocated by uiox_jnl_plat_alloc_block.
 */
__attribute__((weak))
void uiox_jnl_plat_free_block(uint8_t *buf);

/**
 * @brief Return current time as a Unix timestamp (seconds).
 */
__attribute__((weak))
uint64_t uiox_jnl_plat_time(void);

/**
 * @brief Compute CRC-32 of @len bytes at @data.
 */
__attribute__((weak))
uint32_t uiox_jnl_plat_crc32(const uint8_t *data, size_t len);

/**
 * @brief Compute SHA-256 of @len bytes at @data into @digest[32].
 *        Default: delegates to uiox_ks_sha256() from 12_ksign when linked.
 */
__attribute__((weak))
void uiox_jnl_plat_sha256(const uint8_t *data, size_t len,
                            uint8_t digest[UIOX_JNL_HASH_LEN]);

/* =========================================================================
 * Internal I/O helpers (used by journal engine, not by callers)
 * ====================================================================== */

/** Write a journal block and verify it can be read back (debug builds). */
uiox_jnl_err_t uiox_jnl_io_write_verify(uint32_t blocknr,
                                          const uint8_t *buf);

/** Write a sequence of contiguous journal blocks. */
uiox_jnl_err_t uiox_jnl_io_write_seq(uint32_t start_blocknr,
                                       const uint8_t *bufs,
                                       uint32_t count);

#ifdef __cplusplus
}
#endif
#endif /* UIOX_JRNL_IO_H */
