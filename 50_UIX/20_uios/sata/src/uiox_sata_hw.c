/**
 * @file  uiox_sata_hw.c
 * @brief UIOX SATA HAL implementation.
 * @date  2026-06-12
 */

 #include "uiox_sata_hw.h"
 #include <string.h>
 #include <errno.h>
 
 #define OPS(hw) ((const uiox_sata_hw_ops_t *)(hw)->priv)
 
 int uiox_sata_hw_init(uiox_sata_hw_t *hw,
                        const uiox_sata_hw_ops_t *ops)
 {
     if (!hw || !ops || !ops->init) return -EINVAL;
     hw->priv        = (void *)ops;
     hw->pending_irq = 0u;
     hw->dev_present = false;
     hw->dev_ready   = false;
     hw->ncq_active  = 0u;
     memset(&hw->ident, 0, sizeof(hw->ident));
     return ops->init(hw);
 }
 
 void uiox_sata_hw_deinit(uiox_sata_hw_t *hw)
 {
     if (!hw || !hw->priv) return;
     if (OPS(hw)->deinit) OPS(hw)->deinit(hw);
     hw->priv = NULL;
 }
 
 int uiox_sata_hw_port_start(uiox_sata_hw_t *hw, uint8_t port)
 {
     if (!hw || !hw->priv) return -EINVAL;
     if (!OPS(hw)->port_start) return -ENOSYS;
     return OPS(hw)->port_start(hw, port);
 }
 
 void uiox_sata_hw_port_stop(uiox_sata_hw_t *hw, uint8_t port)
 {
     if (!hw || !hw->priv) return;
     if (OPS(hw)->port_stop) OPS(hw)->port_stop(hw, port);
 }
 
 uint32_t uiox_sata_hw_ghc_read(uiox_sata_hw_t *hw, uint32_t off)
 {
     if (!hw || !hw->priv || !OPS(hw)->ghc_read) return 0u;
     return OPS(hw)->ghc_read(hw, off);
 }
 
 void uiox_sata_hw_ghc_write(uiox_sata_hw_t *hw,
                               uint32_t off, uint32_t val)
 {
     if (!hw || !hw->priv || !OPS(hw)->ghc_write) return;
     OPS(hw)->ghc_write(hw, off, val);
 }
 
 uint32_t uiox_sata_hw_px_read(uiox_sata_hw_t *hw,
                                 uint8_t port, uint32_t off)
 {
     if (!hw || !hw->priv || !OPS(hw)->px_read) return 0u;
     return OPS(hw)->px_read(hw, port, off);
 }
 
 void uiox_sata_hw_px_write(uiox_sata_hw_t *hw,
                              uint8_t port, uint32_t off, uint32_t val)
 {
     if (!hw || !hw->priv || !OPS(hw)->px_write) return;
     OPS(hw)->px_write(hw, port, off, val);
 }
 
 int uiox_sata_hw_cmd_issue(uiox_sata_hw_t *hw, uint8_t port,
                             uint8_t slot,
                             const uiox_sata_fis_h2d_t *fis,
                             bool write,
                             uintptr_t data_phys, uint32_t len)
 {
     if (!hw || !hw->priv || !fis) return -EINVAL;
     if (!OPS(hw)->cmd_issue) return -ENOSYS;
     return OPS(hw)->cmd_issue(hw, port, slot, fis, write, data_phys, len);
 }
 
 int uiox_sata_hw_read_sectors(uiox_sata_hw_t *hw, uint64_t lba,
                                uint8_t *buf, uint32_t count)
 {
     if (!hw || !hw->priv || !buf) return -EINVAL;
     if (!OPS(hw)->read_sectors) return -ENOSYS;
     return OPS(hw)->read_sectors(hw, lba, buf, count);
 }
 
 int uiox_sata_hw_write_sectors(uiox_sata_hw_t *hw, uint64_t lba,
                                 const uint8_t *buf, uint32_t count)
 {
     if (!hw || !hw->priv || !buf) return -EINVAL;
     if (!OPS(hw)->write_sectors) return -ENOSYS;
     return OPS(hw)->write_sectors(hw, lba, buf, count);
 }
 
 int uiox_sata_hw_port_reset(uiox_sata_hw_t *hw, uint8_t port)
 {
     if (!hw || !hw->priv) return -EINVAL;
     if (!OPS(hw)->port_reset) return -ENOSYS;
     return OPS(hw)->port_reset(hw, port);
 }
 
 int uiox_sata_hw_smart_read(uiox_sata_hw_t *hw, uint8_t *buf)
 {
     if (!hw || !hw->priv || !buf) return -EINVAL;
     if (!OPS(hw)->smart_read) return -ENOSYS;
     return OPS(hw)->smart_read(hw, buf);
 }
 