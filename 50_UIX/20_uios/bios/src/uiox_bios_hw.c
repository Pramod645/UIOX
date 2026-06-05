/**
 * @file    uiox_bios_hw.c
 * @brief   UIOX BIOS HAL implementation.
 * @date    2026-06-04
 */

 #include "uiox_bios_hw.h"
 #include <string.h>
 #include <errno.h>
 
 int uiox_bios_hw_init(uiox_bios_hw_t *hw, const uiox_bios_hw_ops_t *ops)
 {
     if (!hw || !ops || !ops->init) return -EINVAL;
     hw->priv        = (void *)ops;
     hw->initialised = false;
     hw->flash_busy  = false;
     hw->wp_active   = true;  /* Default: write-protected */
     int rc = ops->init(hw);
     if (rc == 0) hw->initialised = true;
     return rc;
 }
 
 void uiox_bios_hw_deinit(uiox_bios_hw_t *hw)
 {
     if (!hw || !hw->priv) return;
     const uiox_bios_hw_ops_t *ops = (const uiox_bios_hw_ops_t *)hw->priv;
     if (ops->deinit) ops->deinit(hw);
     hw->priv        = NULL;
     hw->initialised = false;
 }
 
 int uiox_bios_hw_spi_read(uiox_bios_hw_t *hw,
                            uint32_t offset, void *buf, uint32_t len)
 {
     if (!hw || !hw->priv || !buf) return -EINVAL;
     if (offset + len > hw->geo.total_bytes) return -ERANGE;
     const uiox_bios_hw_ops_t *ops = (const uiox_bios_hw_ops_t *)hw->priv;
     if (!ops->spi_read) return -ENOSYS;
     return ops->spi_read(hw, offset, buf, len);
 }
 
 int uiox_bios_hw_spi_write(uiox_bios_hw_t *hw,
                             uint32_t offset, const void *buf, uint32_t len)
 {
     if (!hw || !hw->priv || !buf) return -EINVAL;
     if (hw->wp_active) return -EACCES;
     if (offset + len > hw->geo.total_bytes) return -ERANGE;
     const uiox_bios_hw_ops_t *ops = (const uiox_bios_hw_ops_t *)hw->priv;
     if (!ops->spi_write) return -ENOSYS;
     return ops->spi_write(hw, offset, buf, len);
 }
 
 int uiox_bios_hw_spi_erase_sector(uiox_bios_hw_t *hw, uint32_t offset)
 {
     if (!hw || !hw->priv) return -EINVAL;
     if (hw->wp_active) return -EACCES;
     if (offset % hw->geo.sector_bytes) return -EINVAL;
     const uiox_bios_hw_ops_t *ops = (const uiox_bios_hw_ops_t *)hw->priv;
     if (!ops->spi_erase_sector) return -ENOSYS;
     return ops->spi_erase_sector(hw, offset);
 }
 
 int uiox_bios_hw_spi_erase_block(uiox_bios_hw_t *hw, uint32_t offset)
 {
     if (!hw || !hw->priv) return -EINVAL;
     if (hw->wp_active) return -EACCES;
     if (offset % hw->geo.block_bytes) return -EINVAL;
     const uiox_bios_hw_ops_t *ops = (const uiox_bios_hw_ops_t *)hw->priv;
     if (!ops->spi_erase_block) return -ENOSYS;
     return ops->spi_erase_block(hw, offset);
 }
 
 int uiox_bios_hw_set_wp(uiox_bios_hw_t *hw, bool protect)
 {
     if (!hw || !hw->priv) return -EINVAL;
     const uiox_bios_hw_ops_t *ops = (const uiox_bios_hw_ops_t *)hw->priv;
     if (!ops->set_wp) return -ENOSYS;
     int rc = ops->set_wp(hw, protect);
     if (rc == 0) hw->wp_active = protect;
     return rc;
 }
 
 int uiox_bios_hw_read_jedec(uiox_bios_hw_t *hw,
                              uint8_t *mfr, uint16_t *dev)
 {
     if (!hw || !hw->priv || !mfr || !dev) return -EINVAL;
     const uiox_bios_hw_ops_t *ops = (const uiox_bios_hw_ops_t *)hw->priv;
     if (!ops->spi_read_jedec) return -ENOSYS;
     return ops->spi_read_jedec(hw, mfr, dev);
 }
 
 uint8_t uiox_bios_hw_cmos_read(uiox_bios_hw_t *hw, uint8_t index)
 {
     if (!hw || !hw->priv) return 0u;
     const uiox_bios_hw_ops_t *ops = (const uiox_bios_hw_ops_t *)hw->priv;
     if (!ops->cmos_read) return 0u;
     return ops->cmos_read(hw, index);
 }
 
 void uiox_bios_hw_cmos_write(uiox_bios_hw_t *hw,
                               uint8_t index, uint8_t val)
 {
     if (!hw || !hw->priv) return;
     const uiox_bios_hw_ops_t *ops = (const uiox_bios_hw_ops_t *)hw->priv;
     if (ops->cmos_write) ops->cmos_write(hw, index, val);
 }
 
 int uiox_bios_hw_tpm_send(uiox_bios_hw_t *hw,
                            const uint8_t *cmd, uint16_t cmd_len,
                            uint8_t *resp, uint16_t *resp_len)
 {
     if (!hw || !hw->priv || !cmd || !resp || !resp_len) return -EINVAL;
     const uiox_bios_hw_ops_t *ops = (const uiox_bios_hw_ops_t *)hw->priv;
     if (!ops->tpm_send) return -ENOSYS;
     return ops->tpm_send(hw, cmd, cmd_len, resp, resp_len);
 }
 