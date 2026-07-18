/**
 * @file    uiox_soc_pcie.h
 * @brief   UIOX SoC HAL — PCIe early initialisation.
 *
 * Handles PCIe config space access, BAR assignment, and link training
 * for NVMe SSDs, SATA controllers, and network cards during SoC init
 * before the kernel takes over PCI enumeration.
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
 
 /* ── ECAM base addresses ────────────────────────────────── */
 #define UIOX_SOC_PCIE_ECAM_ARM64    0x3F000000u  /**< QEMU virt           */
 #define UIOX_SOC_PCIE_ECAM_X86_Q35  0xB0000000u  /**< QEMU q35            */
 
 /**
  * ECAM offset formula:
  *   base + ((bus << 20) | (dev << 15) | (fn << 12) | offset)
  */
 #define UIOX_SOC_PCIE_ECAM_ADDR(ecam, bus, dev, fn, off) \
     ((ecam) + (((uint32_t)(bus) << 20u) | \
                ((uint32_t)(dev) << 15u) | \
                ((uint32_t)(fn)  << 12u) | \
                (uint32_t)(off)))
 
 /* ── Standard PCI config registers ─────────────────────── */
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
 
 /* ── Command register bits ──────────────────────────────── */
 #define PCI_CMD_IO_EN           (1u <<  0)
 #define PCI_CMD_MEM_EN          (1u <<  1)
 #define PCI_CMD_BUS_MASTER      (1u <<  2)
 #define PCI_CMD_INT_DISABLE     (1u << 10)
 
 /* ── Class codes ────────────────────────────────────────── */
 #define PCI_CLASS_STORAGE_NVME  0x010802u  /**< NVMe controller           */
 #define PCI_CLASS_STORAGE_SATA  0x010601u  /**< AHCI SATA                 */
 #define PCI_CLASS_NETWORK_ETH   0x020000u  /**< Ethernet controller       */
 #define PCI_CLASS_DISPLAY_VGA   0x030000u  /**< VGA display               */
 
 #define PCI_HDR_MULTIFUNCTION   0x80u
 #define PCI_VENDOR_INVALID      0xFFFFu
 
 /* ── PCIe device descriptor ─────────────────────────────── */
 typedef struct {
     uint16_t vendor_id;
     uint16_t device_id;
     uint16_t subsys_vendor;
     uint16_t subsys_id;
     uint32_t class_code;   /**< 24-bit: class(23:16), sub(15:8), iface(7:0) */
     uint8_t  bus;
     uint8_t  dev;
     uint8_t  fn;
     uint8_t  irq_line;
     uint64_t bar[6];       /**< Decoded BAR physical addresses            */
     uint32_t bar_size[6];  /**< BAR sizes in bytes                        */
     bool     bar_is_64[6]; /**< True if 64-bit BAR                        */
     bool     bar_is_io[6]; /**< True if I/O space BAR                     */
 } uiox_soc_pcie_dev_t;
 
 #define UIOX_SOC_PCIE_MAX_DEVICES   32u
 
 /* ── PCIe controller context ────────────────────────────── */
 typedef struct {
     uintptr_t            ecam_base;
     uint8_t              bus_start;
     uint8_t              bus_end;
     uint64_t             mem32_base;   /**< 32-bit MMIO window base       */
     uint64_t             mem32_size;
     uint64_t             mem64_base;   /**< 64-bit MMIO window base       */
     uint64_t             mem64_size;
     uint64_t             mem32_alloc;  /**< Current allocation pointer    */
     uint64_t             mem64_alloc;
     uiox_soc_pcie_dev_t  devices[UIOX_SOC_PCIE_MAX_DEVICES];
     uint32_t             num_devices;
     bool                 initialized;
 } uiox_soc_pcie_ctrl_t;
 
 /* ── PCIe HAL API ───────────────────────────────────────── */
 uiox_soc_err_t uiox_soc_pcie_init       (uiox_soc_pcie_ctrl_t *ctrl,
                                           uintptr_t ecam_base);
 uiox_soc_err_t uiox_soc_pcie_scan       (uiox_soc_pcie_ctrl_t *ctrl);
 uiox_soc_err_t uiox_soc_pcie_assign_bars(uiox_soc_pcie_ctrl_t *ctrl,
                                           uiox_soc_pcie_dev_t  *dev);
 uiox_soc_err_t uiox_soc_pcie_enable_dev (uiox_soc_pcie_ctrl_t *ctrl,
                                           uiox_soc_pcie_dev_t  *dev);
 
 /* Config space read/write */
 uint32_t       uiox_soc_pcie_cfg_read32  (uiox_soc_pcie_ctrl_t *ctrl,
                                            uint8_t bus, uint8_t dev,
                                            uint8_t fn, uint16_t off);
 uint16_t       uiox_soc_pcie_cfg_read16  (uiox_soc_pcie_ctrl_t *ctrl,
                                            uint8_t bus, uint8_t dev,
                                            uint8_t fn, uint16_t off);
 void           uiox_soc_pcie_cfg_write32 (uiox_soc_pcie_ctrl_t *ctrl,
                                            uint8_t bus, uint8_t dev,
                                            uint8_t fn, uint16_t off,
                                            uint32_t val);
 
 /** Find a device by class code. */
 uiox_soc_pcie_dev_t *uiox_soc_pcie_find_class(uiox_soc_pcie_ctrl_t *ctrl,
                                                 uint32_t class_code);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_SOC_PCIE_H */
 