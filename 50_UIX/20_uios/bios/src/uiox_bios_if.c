/**
 * @file    uiox_bios_if.c
 * @brief   UIOX BIOS interface driver implementation.
 * @date    2026-06-04
 */

 #include "uiox_bios_if.h"
 #include <string.h>
 #include <errno.h>
 
 int uiox_bios_if_config(uiox_bios_if_t *bif, uiox_bios_hw_t *hw)
 {
     if (!bif || !hw) return -EINVAL;
     memset(bif, 0, sizeof(*bif));
     bif->hw     = hw;
     bif->primed = true;
     uiox_bios_buf_init();
     return 0;
 }
 
 int uiox_bios_if_read(uiox_bios_if_t *bif,
                        uint32_t offset, void *buf, uint32_t len)
 {
     if (!bif || !buf || !len) return -EINVAL;
     int rc = uiox_bios_hw_spi_read(bif->hw, offset, buf, len);
     if (rc == 0) bif->stats.bytes_read += len;
     return rc;
 }
 
 int uiox_bios_if_write(uiox_bios_if_t *bif,
                         uint32_t offset, const void *buf, uint32_t len)
 {
     if (!bif || !buf || !len) return -EINVAL;
 
     /* Remove write-protect */
     uiox_bios_hw_set_wp(bif->hw, false);
     bif->stats.wp_removes++;
 
     uint32_t remaining = len;
     uint32_t src_off   = 0;
     const uint8_t *src = (const uint8_t *)buf;
     int rc = 0;
 
     while (remaining > 0) {
         uint32_t sector_base = (offset + src_off) &
                                ~(bif->hw->geo.sector_bytes - 1u);
         uint32_t in_sector   = (offset + src_off) - sector_base;
         uint32_t to_write    = bif->hw->geo.sector_bytes - in_sector;
         if (to_write > remaining) to_write = remaining;
 
         /* Full sector? Skip read-modify-write */
         if (in_sector == 0 && to_write == bif->hw->geo.sector_bytes) {
             rc = uiox_bios_hw_spi_erase_sector(bif->hw, sector_base);
             if (rc < 0) { bif->stats.erase_errors++; break; }
             bif->stats.sectors_erased++;
             rc = uiox_bios_hw_spi_write(bif->hw,
                                          sector_base,
                                          src + src_off, to_write);
             if (rc < 0) { bif->stats.write_errors++; break; }
         } else {
             /* Read-modify-write */
             uiox_bios_buf_t *stage =
                 uiox_bios_buf_alloc(UIOX_BIOS_BUF_WRITE_STAGE);
             if (!stage) { rc = -ENOMEM; break; }
 
             rc = uiox_bios_hw_spi_read(bif->hw, sector_base,
                                         stage->aligned,
                                         bif->hw->geo.sector_bytes);
             if (rc < 0) { uiox_bios_buf_free(stage);
                           bif->stats.write_errors++; break; }
 
             memcpy(stage->aligned + in_sector, src + src_off, to_write);
 
             rc = uiox_bios_hw_spi_erase_sector(bif->hw, sector_base);
             if (rc < 0) { uiox_bios_buf_free(stage);
                           bif->stats.erase_errors++; break; }
             bif->stats.sectors_erased++;
 
             rc = uiox_bios_hw_spi_write(bif->hw, sector_base,
                                          stage->aligned,
                                          bif->hw->geo.sector_bytes);
             uiox_bios_buf_free(stage);
             if (rc < 0) { bif->stats.write_errors++; break; }
         }
 
         bif->stats.bytes_written += to_write;
         src_off   += to_write;
         remaining -= to_write;
     }
 
     /* Restore write-protect */
     uiox_bios_hw_set_wp(bif->hw, true);
     return rc;
 }
 
 int uiox_bios_if_erase_sector(uiox_bios_if_t *bif, uint32_t offset)
 {
     if (!bif) return -EINVAL;
     uiox_bios_hw_set_wp(bif->hw, false);
     int rc = uiox_bios_hw_spi_erase_sector(bif->hw, offset);
     if (rc == 0) bif->stats.sectors_erased++;
     else         bif->stats.erase_errors++;
     uiox_bios_hw_set_wp(bif->hw, true);
     return rc;
 }
 
 int uiox_bios_if_erase_block(uiox_bios_if_t *bif, uint32_t offset)
 {
     if (!bif) return -EINVAL;
     uiox_bios_hw_set_wp(bif->hw, false);
     const uiox_bios_hw_ops_t *ops =
         (const uiox_bios_hw_ops_t *)bif->hw->priv;
     int rc = ops->spi_erase_block ?
              ops->spi_erase_block(bif->hw, offset) : -ENOSYS;
     if (rc == 0) bif->stats.blocks_erased++;
     else         bif->stats.erase_errors++;
     uiox_bios_hw_set_wp(bif->hw, true);
     return rc;
 }
 
 int uiox_bios_if_erase_chip(uiox_bios_if_t *bif)
 {
     if (!bif) return -EINVAL;
     uiox_bios_hw_set_wp(bif->hw, false);
     const uiox_bios_hw_ops_t *ops =
         (const uiox_bios_hw_ops_t *)bif->hw->priv;
     int rc = ops->spi_erase_chip ?
              ops->spi_erase_chip(bif->hw) : -ENOSYS;
     uiox_bios_hw_set_wp(bif->hw, true);
     return rc;
 }
 
 int uiox_bios_if_verify(uiox_bios_if_t *bif,
                          uint32_t offset,
                          const void *expected, uint32_t len)
 {
     if (!bif || !expected || !len) return -EINVAL;
     uiox_bios_buf_t *stage = uiox_bios_buf_alloc(UIOX_BIOS_BUF_READ_STAGE);
     if (!stage) return -ENOMEM;
 
     uint32_t off = 0;
     int rc = 0;
     const uint8_t *exp = (const uint8_t *)expected;
 
     while (off < len && rc == 0) {
         uint32_t chunk = len - off;
         if (chunk > UIOX_BIOS_SECTOR_SIZE) chunk = UIOX_BIOS_SECTOR_SIZE;
         rc = uiox_bios_hw_spi_read(bif->hw, offset + off,
                                     stage->aligned, chunk);
         if (rc < 0) break;
         if (memcmp(stage->aligned, exp + off, chunk) != 0)
             rc = -EBADMSG;
         off += chunk;
     }
     uiox_bios_buf_free(stage);
     return rc;
 }
 
 const uiox_bios_region_t *uiox_bios_if_find_region(
     const uiox_bios_if_t *bif, const char *name)
 {
     if (!bif || !name) return NULL;
     for (uint8_t i = 0; i < bif->hw->num_regions; i++) {
         if (bif->hw->regions[i].name &&
             strcmp(bif->hw->regions[i].name, name) == 0)
             return &bif->hw->regions[i];
     }
     return NULL;
 }
 
 void uiox_bios_if_stats_get(const uiox_bios_if_t *bif,
                              uiox_bios_if_stats_t *out)
 { if (!bif || !out) return; memcpy(out, &bif->stats, sizeof(*out)); }
 
 void uiox_bios_if_stats_reset(uiox_bios_if_t *bif)
 { if (!bif) return; memset(&bif->stats, 0, sizeof(bif->stats)); }
 