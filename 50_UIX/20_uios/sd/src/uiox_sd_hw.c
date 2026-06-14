/**
 * @file  uiox_sd_hw.c
 * @brief UIOX SD Card Reader HAL implementation.
 * @date  2026-06-11
 */

 #include "uiox_sd_hw.h"
 #include <string.h>
 #include <errno.h>
 
 #define OPS(hw) ((const uiox_sd_hw_ops_t *)(hw)->priv)
 
 int uiox_sd_hw_init(uiox_sd_hw_t *hw, const uiox_sd_hw_ops_t *ops)
 {
     if (!hw || !ops || !ops->init) return -EINVAL;
     hw->priv          = (void *)ops;
     hw->pending_irq   = 0u;
     hw->card_present  = false;
     hw->write_protect = false;
     memset(&hw->card, 0, sizeof(hw->card));
     return ops->init(hw);
 }
 
 void uiox_sd_hw_deinit(uiox_sd_hw_t *hw)
 {
     if (!hw || !hw->priv) return;
     if (OPS(hw)->deinit) OPS(hw)->deinit(hw);
     hw->priv = NULL;
 }
 
 int uiox_sd_hw_power_on(uiox_sd_hw_t *hw)
 {
     if (!hw || !hw->priv) return -EINVAL;
     if (!OPS(hw)->power_on) return -ENOSYS;
     return OPS(hw)->power_on(hw);
 }
 
 void uiox_sd_hw_power_off(uiox_sd_hw_t *hw)
 {
     if (!hw || !hw->priv) return;
     if (OPS(hw)->power_off) OPS(hw)->power_off(hw);
 }
 
 uint32_t uiox_sd_hw_reg_read(uiox_sd_hw_t *hw, uint32_t offset)
 {
     if (!hw || !hw->priv || !OPS(hw)->reg_read) return 0u;
     return OPS(hw)->reg_read(hw, offset);
 }
 
 void uiox_sd_hw_reg_write(uiox_sd_hw_t *hw, uint32_t offset, uint32_t val)
 {
     if (!hw || !hw->priv || !OPS(hw)->reg_write) return;
     OPS(hw)->reg_write(hw, offset, val);
 }
 
 int uiox_sd_hw_set_clock(uiox_sd_hw_t *hw, uint32_t hz)
 {
     if (!hw || !hw->priv) return -EINVAL;
     if (!OPS(hw)->set_clock) return -ENOSYS;
     return OPS(hw)->set_clock(hw, hz);
 }
 
 int uiox_sd_hw_set_bus_width(uiox_sd_hw_t *hw, uint8_t width)
 {
     if (!hw || !hw->priv) return -EINVAL;
     if (!OPS(hw)->set_bus_width) return -ENOSYS;
     return OPS(hw)->set_bus_width(hw, width);
 }
 
 int uiox_sd_hw_send_cmd(uiox_sd_hw_t *hw, uint8_t cmd,
                          uint32_t arg, uiox_sd_resp_t resp_type,
                          uint32_t *resp)
 {
     if (!hw || !hw->priv) return -EINVAL;
     if (!OPS(hw)->send_cmd) return -ENOSYS;
     return OPS(hw)->send_cmd(hw, cmd, arg, resp_type, resp);
 }
 
 int uiox_sd_hw_read_blocks(uiox_sd_hw_t *hw, uint32_t lba,
                             uint8_t *buf, uint32_t count)
 {
     if (!hw || !hw->priv || !buf) return -EINVAL;
     if (!OPS(hw)->read_blocks) return -ENOSYS;
     return OPS(hw)->read_blocks(hw, lba, buf, count);
 }
 
 int uiox_sd_hw_write_blocks(uiox_sd_hw_t *hw, uint32_t lba,
                              const uint8_t *buf, uint32_t count)
 {
     if (!hw || !hw->priv || !buf) return -EINVAL;
     if (hw->write_protect) return -EROFS;
     if (!OPS(hw)->write_blocks) return -ENOSYS;
     return OPS(hw)->write_blocks(hw, lba, buf, count);
 }
 
 uint8_t uiox_sd_hw_crc7(uiox_sd_hw_t *hw,
                           const uint8_t *data, uint8_t len)
 {
     if (!hw || !hw->priv || !OPS(hw)->crc7)
         return 0u; /* SW fallback would go here */
     return OPS(hw)->crc7(data, len);
 }
 
 uint16_t uiox_sd_hw_crc16(uiox_sd_hw_t *hw,
                             const uint8_t *data, uint32_t len)
 {
     if (!hw || !hw->priv || !OPS(hw)->crc16)
         return 0u;
     return OPS(hw)->crc16(data, len);
 }
 