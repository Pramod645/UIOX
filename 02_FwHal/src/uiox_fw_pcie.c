/**
 * @file  uiox_fw_pcie.c
 * @brief UIOX Firmware HAL — PCIe ECAM config space and BAR assignment.
 * @date  2026-07-07
 */

 #include "../include/uiox_fw_pcie.h"
 #include "../include/uiox_fw_hw.h"
 
 /* =========================================================================
  * ECAM config space accessors
  * ====================================================================== */
 
 uint32_t uiox_fw_pcie_cfg_read32(uiox_pcie_ctrl_t *ctrl,
                                    uint8_t bus, uint8_t dev,
                                    uint8_t fn, uint16_t off)
 {
     uintptr_t addr = UIOX_PCIE_ECAM_ADDR(ctrl->ecam_base, bus, dev, fn, off);
     return *((volatile uint32_t *)addr);
 }
 
 uint16_t uiox_fw_pcie_cfg_read16(uiox_pcie_ctrl_t *ctrl,
                                    uint8_t bus, uint8_t dev,
                                    uint8_t fn, uint16_t off)
 {
     uintptr_t addr = UIOX_PCIE_ECAM_ADDR(ctrl->ecam_base, bus, dev, fn, off);
     return *((volatile uint16_t *)addr);
 }
 
 void uiox_fw_pcie_cfg_write32(uiox_pcie_ctrl_t *ctrl,
                                 uint8_t bus, uint8_t dev,
                                 uint8_t fn, uint16_t off, uint32_t val)
 {
     uintptr_t addr = UIOX_PCIE_ECAM_ADDR(ctrl->ecam_base, bus, dev, fn, off);
     *((volatile uint32_t *)addr) = val;
 }
 
 /* =========================================================================
  * Scan and enumerate
  * ====================================================================== */
 
 uiox_fw_err_t uiox_fw_pcie_init(uiox_pcie_ctrl_t *ctrl, uintptr_t ecam_base)
 {
     if (!ctrl) return UIOX_FW_ERR_INVAL;
     uint8_t *p = (uint8_t *)ctrl;
     for (size_t i = 0u; i < sizeof(*ctrl); i++) p[i] = 0u;
     ctrl->ecam_base   = ecam_base;
     ctrl->bus_start   = 0u;
     ctrl->bus_end     = 255u;
     ctrl->mem32_base  = 0x10000000ULL;
     ctrl->mem32_size  = 0x2EFF0000ULL;
     ctrl->mem64_base  = 0x8000000000ULL;
     ctrl->mem64_size  = 0x8000000000ULL;
     ctrl->mem32_alloc = ctrl->mem32_base;
     ctrl->mem64_alloc = ctrl->mem64_base;
     ctrl->initialized = true;
     return UIOX_FW_OK;
 }
 
 uiox_fw_err_t uiox_fw_pcie_scan(uiox_pcie_ctrl_t *ctrl)
 {
     if (!ctrl) return UIOX_FW_ERR_INVAL;
     ctrl->num_devices = 0u;
 
     for (uint32_t bus = ctrl->bus_start;
          bus <= ctrl->bus_end && ctrl->num_devices < UIOX_PCIE_MAX_DEVICES;
          bus++) {
         for (uint32_t dev = 0u;
              dev < 32u && ctrl->num_devices < UIOX_PCIE_MAX_DEVICES;
              dev++) {
             /* Check slot 0 of each device */
             uint32_t id = uiox_fw_pcie_cfg_read32(ctrl,
                            (uint8_t)bus, (uint8_t)dev, 0u,
                            PCI_CFG_VENDOR_ID);
             uint16_t vendor = (uint16_t)(id & 0xFFFFu);
             if (vendor == PCI_VENDOR_INVALID) continue;
 
             uint8_t hdr = (uint8_t)(uiox_fw_pcie_cfg_read32(ctrl,
                            (uint8_t)bus, (uint8_t)dev, 0u,
                            PCI_CFG_HEADER_TYPE) >> 16u);
             uint8_t nfn = (hdr & PCI_HDR_MULTIFUNCTION) ? 8u : 1u;
 
             for (uint8_t fn = 0u; fn < nfn; fn++) {
                 uint32_t id2 = uiox_fw_pcie_cfg_read32(ctrl,
                                 (uint8_t)bus, (uint8_t)dev, fn,
                                 PCI_CFG_VENDOR_ID);
                 if ((uint16_t)(id2 & 0xFFFFu) == PCI_VENDOR_INVALID) continue;
 
                 uiox_pcie_dev_t *d = &ctrl->devices[ctrl->num_devices++];
                 uint8_t *pd = (uint8_t *)d;
                 for (size_t i = 0u; i < sizeof(*d); i++) pd[i] = 0u;
 
                 d->vendor_id = (uint16_t)(id2 & 0xFFFFu);
                 d->device_id = (uint16_t)(id2 >> 16u);
                 d->bus       = (uint8_t)bus;
                 d->dev       = (uint8_t)dev;
                 d->fn        = fn;
 
                 uint32_t cls = uiox_fw_pcie_cfg_read32(ctrl,
                                 (uint8_t)bus, (uint8_t)dev, fn,
                                 PCI_CFG_CLASS_REV);
                 d->class_code = cls >> 8u;
 
                 d->irq_line = (uint8_t)(uiox_fw_pcie_cfg_read32(ctrl,
                                (uint8_t)bus, (uint8_t)dev, fn,
                                PCI_CFG_IRQ_LINE) & 0xFFu);
             }
         }
     }
     return UIOX_FW_OK;
 }
 
 uiox_fw_err_t uiox_fw_pcie_assign_bars(uiox_pcie_ctrl_t *ctrl,
                                           uiox_pcie_dev_t *dev)
 {
     if (!ctrl || !dev) return UIOX_FW_ERR_INVAL;
     for (uint32_t i = 0u; i < 6u; i++) {
         /* Write all-ones to discover BAR size */
         uiox_fw_pcie_cfg_write32(ctrl, dev->bus, dev->dev, dev->fn,
                                   (uint16_t)(PCI_CFG_BAR0 + i * 4u),
                                   0xFFFFFFFFu);
         uint32_t r = uiox_fw_pcie_cfg_read32(ctrl, dev->bus, dev->dev,
                                                dev->fn,
                                                (uint16_t)(PCI_CFG_BAR0 + i * 4u));
         if (r == 0u) continue;
 
         bool is_io   = !!(r & 0x1u);
         bool is_64   = ((r & 0x6u) == 0x4u);
         uint32_t mask = is_io ? (r & ~0x3u) : (r & ~0xFu);
         uint32_t size = ~mask + 1u;
 
         dev->bar_is_io[i] = is_io;
         dev->bar_is_64[i] = is_64;
         dev->bar_size[i]  = size;
 
         if (!is_io && size > 0u) {
             /* Allocate from 32-bit MMIO window */
             ctrl->mem32_alloc = (ctrl->mem32_alloc + size - 1u) & ~((uint64_t)size - 1u);
             dev->bar[i] = ctrl->mem32_alloc;
             ctrl->mem32_alloc += size;
             uiox_fw_pcie_cfg_write32(ctrl, dev->bus, dev->dev, dev->fn,
                                       (uint16_t)(PCI_CFG_BAR0 + i * 4u),
                                       (uint32_t)dev->bar[i]);
         }
         if (is_64) i++;  /* Skip the high 32-bit BAR slot */
     }
     return UIOX_FW_OK;
 }
 
 uiox_fw_err_t uiox_fw_pcie_enable_dev(uiox_pcie_ctrl_t *ctrl,
                                          uiox_pcie_dev_t *dev)
 {
     if (!ctrl || !dev) return UIOX_FW_ERR_INVAL;
     uiox_fw_pcie_cfg_write32(ctrl, dev->bus, dev->dev, dev->fn,
                                PCI_CFG_COMMAND,
                                PCI_CMD_MEM_EN | PCI_CMD_BUS_MASTER);
     return UIOX_FW_OK;
 }
 
 uiox_pcie_dev_t *uiox_fw_pcie_find_class(uiox_pcie_ctrl_t *ctrl,
                                            uint32_t class_code)
 {
     if (!ctrl) return NULL;
     for (uint32_t i = 0u; i < ctrl->num_devices; i++) {
         if (ctrl->devices[i].class_code == class_code)
             return &ctrl->devices[i];
     }
     return NULL;
 }
 