/**
 * @file    uiox_soc_pcie.c
 * @brief   UIOX SoC — PCIe ECAM config space and BAR assignment.
 *
 * Layer: 03_SoC
 * @date    2026-07-07
 */

 #include "../include/uiox_soc_pcie.h"
 #include "../include/uiox_soc_hw.h"    /* uiox_soc_hw_ops() for UART output */
 
 /* =========================================================================
  * ECAM config space accessors
  * ====================================================================== */
 
 uiox_uint32_t
 uiox_soc_pcie_cfg_read32(uiox_soc_pcie_ctrl_t *ctrl,
                            uiox_uint8_t bus, uiox_uint8_t dev,
                            uiox_uint8_t fn,  uiox_uint16_t off)
 {
     uiox_uintptr_t addr =
         UIOX_SOC_PCIE_ECAM_ADDR(ctrl->ecam_base, bus, dev, fn, off);
     return *((volatile uiox_uint32_t *)addr);
 }
 
 uiox_uint16_t
 uiox_soc_pcie_cfg_read16(uiox_soc_pcie_ctrl_t *ctrl,
                            uiox_uint8_t bus, uiox_uint8_t dev,
                            uiox_uint8_t fn,  uiox_uint16_t off)
 {
     uiox_uintptr_t addr =
         UIOX_SOC_PCIE_ECAM_ADDR(ctrl->ecam_base, bus, dev, fn, off);
     return *((volatile uiox_uint16_t *)addr);
 }
 
 void
 uiox_soc_pcie_cfg_write32(uiox_soc_pcie_ctrl_t *ctrl,
                             uiox_uint8_t bus, uiox_uint8_t dev,
                             uiox_uint8_t fn,  uiox_uint16_t off,
                             uiox_uint32_t val)
 {
     uiox_uintptr_t addr =
         UIOX_SOC_PCIE_ECAM_ADDR(ctrl->ecam_base, bus, dev, fn, off);
     *((volatile uiox_uint32_t *)addr) = val;
 }
 
 /* =========================================================================
  * Initialise controller context
  * ====================================================================== */
 
 uiox_soc_err_t
 uiox_soc_pcie_init(uiox_soc_pcie_ctrl_t *ctrl, uiox_uintptr_t ecam_base)
 {
     if (!ctrl) return UIOX_SOC_ERR_INVAL;
 
     /* Zero the struct without memset (no libc) */
     uiox_uint8_t *p = (uiox_uint8_t *)ctrl;
     for (uiox_size_t i = 0u; i < sizeof(*ctrl); i++) p[i] = 0u;
 
     ctrl->ecam_base   = ecam_base;
     ctrl->bus_start   = 0u;
     ctrl->bus_end     = 255u;
 
     /* Default MMIO windows — overridden per SoC in uiox_soc_map.h */
     ctrl->mem32_base  = 0x10000000ULL;
     ctrl->mem32_size  = 0x2EFF0000ULL;
     ctrl->mem64_base  = 0x8000000000ULL;
     ctrl->mem64_size  = 0x8000000000ULL;
     ctrl->mem32_alloc = ctrl->mem32_base;
     ctrl->mem64_alloc = ctrl->mem64_base;
     ctrl->initialized = UIOX_TRUE;
 
     return UIOX_SOC_OK;
 }
 
 /* =========================================================================
  * Bus/device/function enumeration
  * ====================================================================== */
 
 uiox_soc_err_t
 uiox_soc_pcie_scan(uiox_soc_pcie_ctrl_t *ctrl)
 {
     if (!ctrl || !ctrl->initialized) return UIOX_SOC_ERR_INVAL;
     ctrl->num_devices = 0u;
 
     for (uiox_uint32_t bus = ctrl->bus_start;
          bus <= ctrl->bus_end &&
          ctrl->num_devices < UIOX_SOC_PCIE_MAX_DEVICES;
          bus++) {
 
         for (uiox_uint32_t dev = 0u;
              dev < 32u &&
              ctrl->num_devices < UIOX_SOC_PCIE_MAX_DEVICES;
              dev++) {
 
             uiox_uint32_t id = uiox_soc_pcie_cfg_read32(
                 ctrl, (uiox_uint8_t)bus, (uiox_uint8_t)dev, 0u,
                 PCI_CFG_VENDOR_ID);
 
             if ((uiox_uint16_t)(id & 0xFFFFu) == PCI_VENDOR_INVALID)
                 continue;
 
             uiox_uint8_t hdr = (uiox_uint8_t)(
                 uiox_soc_pcie_cfg_read32(
                     ctrl, (uiox_uint8_t)bus, (uiox_uint8_t)dev, 0u,
                     PCI_CFG_HEADER_TYPE) >> 16u);
 
             uiox_uint8_t nfn =
                 (hdr & PCI_HDR_MULTIFUNCTION) ? 8u : 1u;
 
             for (uiox_uint8_t fn = 0u; fn < nfn; fn++) {
                 uiox_uint32_t id2 = uiox_soc_pcie_cfg_read32(
                     ctrl, (uiox_uint8_t)bus, (uiox_uint8_t)dev,
                     fn, PCI_CFG_VENDOR_ID);
 
                 if ((uiox_uint16_t)(id2 & 0xFFFFu) == PCI_VENDOR_INVALID)
                     continue;
 
                 uiox_soc_pcie_dev_t *d =
                     &ctrl->devices[ctrl->num_devices++];
 
                 /* Zero the device descriptor */
                 uiox_uint8_t *pd = (uiox_uint8_t *)d;
                 for (uiox_size_t i = 0u; i < sizeof(*d); i++) pd[i] = 0u;
 
                 d->vendor_id = (uiox_uint16_t)(id2 & 0xFFFFu);
                 d->device_id = (uiox_uint16_t)(id2 >> 16u);
                 d->bus       = (uiox_uint8_t)bus;
                 d->dev       = (uiox_uint8_t)dev;
                 d->fn        = fn;
 
                 uiox_uint32_t cls = uiox_soc_pcie_cfg_read32(
                     ctrl, (uiox_uint8_t)bus, (uiox_uint8_t)dev,
                     fn, PCI_CFG_CLASS_REV);
                 d->class_code = cls >> 8u;
 
                 d->irq_line = (uiox_uint8_t)(
                     uiox_soc_pcie_cfg_read32(
                         ctrl, (uiox_uint8_t)bus, (uiox_uint8_t)dev,
                         fn, PCI_CFG_IRQ_LINE) & 0xFFu);
             }
         }
     }
     return UIOX_SOC_OK;
 }
 
 /* =========================================================================
  * BAR size probe and allocation
  * ====================================================================== */
 
 uiox_soc_err_t
 uiox_soc_pcie_assign_bars(uiox_soc_pcie_ctrl_t *ctrl,
                             uiox_soc_pcie_dev_t  *dev)
 {
     if (!ctrl || !dev) return UIOX_SOC_ERR_INVAL;
 
     for (uiox_uint32_t i = 0u; i < 6u; i++) {
         uiox_uint16_t reg =
             (uiox_uint16_t)(PCI_CFG_BAR0 + i * 4u);
 
         /* Write all-ones to discover BAR size */
         uiox_soc_pcie_cfg_write32(ctrl,
             dev->bus, dev->dev, dev->fn, reg, 0xFFFFFFFFu);
 
         uiox_uint32_t r = uiox_soc_pcie_cfg_read32(ctrl,
             dev->bus, dev->dev, dev->fn, reg);
 
         if (r == 0u) continue;
 
         uiox_bool_t is_io  = (r & 0x1u) ? UIOX_TRUE : UIOX_FALSE;
         uiox_bool_t is_64  = ((r & 0x6u) == 0x4u) ? UIOX_TRUE : UIOX_FALSE;
         uiox_uint32_t mask = is_io ? (r & ~0x3u) : (r & ~0xFu);
         uiox_uint32_t size = ~mask + 1u;
 
         dev->bar_is_io[i] = is_io;
         dev->bar_is_64[i] = is_64;
         dev->bar_size[i]  = size;
 
         if (!is_io && size > 0u) {
             /* Align and allocate from 32-bit MMIO window */
             uiox_uint64_t aligned =
                 (ctrl->mem32_alloc + (uiox_uint64_t)size - 1ULL)
                 & ~((uiox_uint64_t)size - 1ULL);
             dev->bar[i]        = aligned;
             ctrl->mem32_alloc  = aligned + size;
 
             uiox_soc_pcie_cfg_write32(ctrl,
                 dev->bus, dev->dev, dev->fn, reg,
                 (uiox_uint32_t)dev->bar[i]);
         }
 
         /* Skip the high 32-bit slot for 64-bit BARs */
         if (is_64) i++;
     }
     return UIOX_SOC_OK;
 }
 
 /* =========================================================================
  * Enable memory decode and bus-master for a device
  * ====================================================================== */
 
  uiox_soc_err_t
  uiox_soc_pcie_enable_dev(uiox_soc_pcie_ctrl_t *ctrl,
                             uiox_soc_pcie_dev_t  *dev)
  {
      if (!ctrl || !dev) return UIOX_SOC_ERR_INVAL;
  
      uiox_soc_pcie_cfg_write32(ctrl,
          dev->bus, dev->dev, dev->fn,
          PCI_CFG_COMMAND,
          PCI_CMD_MEM_EN | PCI_CMD_BUS_MASTER);
  
      return UIOX_SOC_OK;
  }
  
  /* =========================================================================
   * Find device by 24-bit class code
   * ====================================================================== */
  
  uiox_soc_pcie_dev_t *
  uiox_soc_pcie_find_class(uiox_soc_pcie_ctrl_t *ctrl,
                             uiox_uint32_t class_code)
  {
      if (!ctrl) return (uiox_soc_pcie_dev_t *)0;
  
      for (uiox_uint32_t i = 0u; i < ctrl->num_devices; i++) {
          if (ctrl->devices[i].class_code == class_code)
              return &ctrl->devices[i];
      }
      return (uiox_soc_pcie_dev_t *)0;
  }
  