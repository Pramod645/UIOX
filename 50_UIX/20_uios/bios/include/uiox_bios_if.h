/**
 * @file    uiox_bios_if.h
 * @brief   UIOX BIOS interface driver (SPI flash, MMIO, regions).
 *
 * Manages:
 *   - Safe read-modify-write of flash sectors
 *   - Flash region access enforcement
 *   - WP# management before/after writes
 *   - SFDP parameter table parsing
 *   - Flash update progress tracking
 *   - Interface statistics
 *
 * @date    2026-06-04
 */
//Layer 2 — Interface Driver
 #ifndef UIOX_BIOS_IF_H
 #define UIOX_BIOS_IF_H
 
 #include "uiox_bios_hw.h"
 #include "uiox_bios_buf.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 typedef struct {
     uint64_t  bytes_read;
     uint64_t  bytes_written;
     uint32_t  sectors_erased;
     uint32_t  blocks_erased;
     uint32_t  wp_removes;
     uint32_t  write_errors;
     uint32_t  erase_errors;
 } uiox_bios_if_stats_t;
 
 typedef struct {
     uiox_bios_hw_t      *hw;
     uiox_bios_if_stats_t stats;
     uint32_t             write_progress_pct; /**< 0..100 for fw update    */
     bool                 update_in_progress;
     bool                 primed;
 } uiox_bios_if_t;
 
 int  uiox_bios_if_config  (uiox_bios_if_t *bif, uiox_bios_hw_t *hw);
 
 /**
  * @brief  Safe read from flash (any alignment, any length).
  */
 int  uiox_bios_if_read    (uiox_bios_if_t *bif,
                             uint32_t offset, void *buf, uint32_t len);
 
 /**
  * @brief  Safe write to flash (handles sector erase + page program).
  *         Performs read-modify-write for partial sector writes.
  *         Removes WP#, writes, then restores WP#.
  */
 int  uiox_bios_if_write   (uiox_bios_if_t *bif,
                             uint32_t offset, const void *buf, uint32_t len);
 
 /** Erase a single 4 KB sector. */
 int  uiox_bios_if_erase_sector(uiox_bios_if_t *bif, uint32_t offset);
 
 /** Erase a 64 KB block. */
 int  uiox_bios_if_erase_block (uiox_bios_if_t *bif, uint32_t offset);
 
 /** Full chip erase (use with caution). */
 int  uiox_bios_if_erase_chip  (uiox_bios_if_t *bif);
 
 /**
  * @brief  Verify flash contents match expected buffer (byte-compare).
  */
 int  uiox_bios_if_verify  (uiox_bios_if_t *bif,
                             uint32_t offset,
                             const void *expected, uint32_t len);
 
 /** Find a flash region by name. */
 const uiox_bios_region_t *uiox_bios_if_find_region(
     const uiox_bios_if_t *bif, const char *name);
 
 void uiox_bios_if_stats_get  (const uiox_bios_if_t *bif,
                                uiox_bios_if_stats_t *out);
 void uiox_bios_if_stats_reset(uiox_bios_if_t *bif);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_BIOS_IF_H */
 