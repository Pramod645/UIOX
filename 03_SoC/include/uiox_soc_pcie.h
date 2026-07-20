/**
 * @file    uiox_soc_pcie.h
 * @brief   UIOX SoC HAL — PCIe ECAM early initialisation.
 *
 * Handles PCIe config space access, BAR assignment, and device enable
 * for NVMe SSDs, SATA controllers, and network cards during SoC init
 * before the kernel takes over PCI enumeration.
 *
 * Layer: 03_SoC  (SoC peripheral — not architecture, not FwHal)
 *
 * @version 1.0.0
 * @date    2026-07-07
 */

 #ifndef UIOX_SOC_PCIE_H
 #define UIOX_SOC_PCIE_H
 
 #include "uiox_soc_types.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * PCIe ECAM base addresses  (SoC-specific — different per chip)
  * ====================================================================== */
 #define UIOX_SOC_PCIE_ECAM_ARM64    0x3F000000u  /**< QEMU virt PCIe ECAM   */
 #define UIOX_SOC_PCIE_ECAM_X86_Q35  0xB0000000u  /**< QEMU q35 PCIe ECAM    */
 
 /* ECAM offset formula:
  *   base + ((bus << 20) | (dev << 15) | (fn << 12) | offset) */
 #define UIOX_SOC_PCIE_ECAM_ADDR(ecam, bus, dev, fn, off) \
     ((ecam) + (((uiox_uint32_t)(bus) << 20u) | \
                ((uiox_uint32_t)(dev) << 15u) | \
                ((uiox_uint32_t)(fn)  << 12u) | \
                (uiox_uint32_t)(off)))
 
 /* =========================================================================
  * Standard PCI configuration space register offsets
  * (PCI Express Base Spec §7.2 — architecture-independent)
  * ====================================================================== */
 #define PCI_CFG_VENDOR_ID       0x00u
 #define PCI_CFG_DEVICE_ID       0x02u
 #define PCI_CFG_COMMAND         0x04u
 #define PCI_CFG_STATUS          0x06u
 #define PCI_CFG_CLASS_REV       0x08u
 #define PCI_CFG_CACHE_LINE      0x0Cu
 #define PCI_CFG_LATENCY         0x0Du
 #define PCI_CFG_HEADER_TYPE     0x0Eu
 #define PCI_CFG_BAR0            0x10u
 #define PCI_CFG_BAR1            0x14u
 #define PCI_CFG_BAR2            0x18u
 #define PCI_CFG_BAR3            0x1Cu
 #define PCI_CFG_BAR4            0x20u
 #define PCI_CFG_BAR5            0x24u
 #define PCI_CFG_SUBSYS_VENDOR   0x2Cu
 #define PCI_CFG_SUBSYS_ID       0x2Eu
 #define PCI_CFG_CAP_PTR         0x34u
 #define PCI_CFG_IRQ_LINE        0x3Cu
 #define PCI_CFG_IRQ_PIN         0x3Du
 
 /* Command register bits */
 #define PCI_CMD_IO_EN           (1u <<  0)
 #define PCI_CMD_MEM_EN          (1u <<  1)
 #define PCI_CMD_BUS_MASTER      (1u <<  2)
 #define PCI_CMD_INT_DISABLE     (1u << 10)
 
 /* Class codes */
 #define PCI_CLASS_STORAGE_NVME  0x010802u  /**< NVMe controller          */
 #define PCI_CLASS_STORAGE_SATA  0x010601u  /**< AHCI SATA                */
 #define PCI_CLASS_NETWORK_ETH   0x020000u  /**< Ethernet controller      */
 #define PCI_CLASS_DISPLAY_VGA   0x030000u  /**< VGA display              */
 
 /* Header type */
 #define PCI_HDR_MULTIFUNCTION   0x80u
 
 /* Invalid vendor (slot empty) */
 #define PCI_VENDOR_INVALID      0xFFFFu
 
 /* =========================================================================
  * PCIe device descriptor
  * ====================================================================== */
 typedef struct {
     uiox_uint16_t vendor_id;
     uiox_uint16_t device_id;
     uiox_uint16_t subsys_vendor;
     uiox_uint16_t subsys_id;
     uiox_uint32_t class_code;   /**< 24-bit: class(23:16), sub(15:8), iface(7:0) */
     uiox_uint8_t  bus;
     uiox_uint8_t  dev;
     uiox_uint8_t  fn;
     uiox_uint8_t  irq_line;
     uiox_uint64_t bar[6];       /**< Decoded BAR physical addresses      */
     uiox_uint32_t bar_size[6];  /**< BAR sizes in bytes                  */
     uiox_bool_t   bar_is_64[6]; /**< True if 64-bit BAR                  */
     uiox_bool_t   bar_is_io[6]; /**< True if I/O space BAR               */
 } uiox_soc_pcie_dev_t;
 
 #define UIOX_SOC_PCIE_MAX_DEVICES   32u
 
 /* =========================================================================
  * PCIe controller context
  * ====================================================================== */
 typedef struct {
     uiox_uintptr_t    ecam_base;
     uiox_uint8_t      bus_start;
     uiox_uint8_t      bus_end;
     uiox_uint64_t     mem32_base;   /**< 32-bit MMIO window base         */
     uiox_uint64_t     mem32_size;
     uiox_uint64_t     mem64_base;   /**< 64-bit MMIO window base         */
     uiox_uint64_t     mem64_size;
     uiox_uint64_t     mem32_alloc;  /**< Current 32-bit allocation ptr   */
     uiox_uint64_t     mem64_alloc;  /**< Current 64-bit allocation ptr   */
     uiox_soc_pcie_dev_t devices[UIOX_SOC_PCIE_MAX_DEVICES];
     uiox_uint32_t     num_devices;
     uiox_bool_t       initialized;
 } uiox_soc_pcie_ctrl_t;
 
 /* =========================================================================
  * PCIe HAL public API
  * ====================================================================== */
 
 uiox_soc_err_t    uiox_soc_pcie_init        (uiox_soc_pcie_ctrl_t *ctrl,
                                                uiox_uintptr_t ecam_base);
 uiox_soc_err_t    uiox_soc_pcie_scan        (uiox_soc_pcie_ctrl_t *ctrl);
 uiox_soc_err_t    uiox_soc_pcie_assign_bars (uiox_soc_pcie_ctrl_t *ctrl,
                                                uiox_soc_pcie_dev_t  *dev);
 uiox_soc_err_t    uiox_soc_pcie_enable_dev  (uiox_soc_pcie_ctrl_t *ctrl,
                                                uiox_soc_pcie_dev_t  *dev);
 
 /* Config space read/write */
 uiox_uint32_t     uiox_soc_pcie_cfg_read32  (uiox_soc_pcie_ctrl_t *ctrl,
                                                uiox_uint8_t bus,
                                                uiox_uint8_t dev,
                                                uiox_uint8_t fn,
                                                uiox_uint16_t off);
 uiox_uint16_t     uiox_soc_pcie_cfg_read16  (uiox_soc_pcie_ctrl_t *ctrl,
                                                uiox_uint8_t bus,
                                                uiox_uint8_t dev,
                                                uiox_uint8_t fn,
                                                uiox_uint16_t off);
 void              uiox_soc_pcie_cfg_write32 (uiox_soc_pcie_ctrl_t *ctrl,
                                                uiox_uint8_t bus,
                                                uiox_uint8_t dev,
                                                uiox_uint8_t fn,
                                                uiox_uint16_t off,
                                                uiox_uint32_t val);
 
 /** Find a device by 24-bit class code.
  *  Returns pointer into ctrl->devices[], or NULL if not found. */
 uiox_soc_pcie_dev_t *uiox_soc_pcie_find_class(uiox_soc_pcie_ctrl_t *ctrl,
                                                 uiox_uint32_t class_code);
 
 /* ── Backwards-compatible wrappers for old uiox_fw_pcie_* callers ──── */
 typedef uiox_soc_pcie_ctrl_t  uiox_pcie_ctrl_t;    /* old name alias   */
 typedef uiox_soc_pcie_dev_t   uiox_pcie_dev_t;     /* old name alias   */
 
 static inline uiox_soc_err_t
 uiox_fw_pcie_init(uiox_soc_pcie_ctrl_t *c, uiox_uintptr_t b)
     { return uiox_soc_pcie_init(c, b); }
 static inline uiox_soc_err_t
 uiox_fw_pcie_scan(uiox_soc_pcie_ctrl_t *c)
     { return uiox_soc_pcie_scan(c); }
 static inline uiox_soc_err_t
 uiox_fw_pcie_assign_bars(uiox_soc_pcie_ctrl_t *c, uiox_soc_pcie_dev_t *d)
     { return uiox_soc_pcie_assign_bars(c, d); }
 static inline uiox_soc_err_t
 uiox_fw_pcie_enable_dev(uiox_soc_pcie_ctrl_t *c, uiox_soc_pcie_dev_t *d)
     { return uiox_soc_pcie_enable_dev(c, d); }
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_SOC_PCIE_H */
 