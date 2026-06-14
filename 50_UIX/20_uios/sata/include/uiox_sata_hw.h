/**
 * @file  uiox_sata_hw.h
 * @brief UIOX SATA III Hardware Abstraction Layer (HAL).
 *
 * Supports:
 *   - AHCI 1.3.1 host controllers (PCIe BAR5 MMIO)
 *   - SATA III (6 Gb/s) — HDD and SSD
 *   - Legacy IDE/ATA compatibility mode (fallback)
 *   - NCQ (Native Command Queuing) up to 32 tags
 *   - SATA OOB (Out-Of-Band) COMRESET / COMINIT / COMWAKE
 *
 * Owns:
 *   - AHCI Generic Host Control registers (GHC)
 *   - Per-port registers (PxCLB, PxFB, PxCMD, PxIS, PxIE, PxSSTS)
 *   - Command List Base (PxCLB) — up to 32 command headers
 *   - FIS Receive Base (PxFB)  — received FIS buffer
 *   - Physical Region Descriptor Table (PRDT)
 *   - GPIO: DEVSLP (device sleep), SGPIO (status LEDs)
 *   - MSI / MSI-X interrupt routing
 *
 * @version 1.0.0
 * @date    2026-06-12
 */

 #ifndef UIOX_SATA_HW_H
 #define UIOX_SATA_HW_H
 
 #include <stdint.h>
 #include <stdbool.h>
 #include <stddef.h>
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * Controller variant
  * ====================================================================== */
 
 typedef enum {
     UIOX_SATA_CTRL_AHCI     = 0,  /**< AHCI 1.3.1 (standard)            */
     UIOX_SATA_CTRL_LEGACY,         /**< Legacy IDE / ATA compatibility    */
     UIOX_SATA_CTRL_ASPM,           /**< AHCI + PCIe ASPM power mgmt      */
 } uiox_sata_ctrl_t;
 
 /* =========================================================================
  * Device type (detected via IDENTIFY / signature)
  * ====================================================================== */
 
 typedef enum {
     UIOX_SATA_DEV_NONE    = 0,
     UIOX_SATA_DEV_ATA,             /**< ATA HDD or SSD                   */
     UIOX_SATA_DEV_ATAPI,           /**< ATAPI (optical drive)            */
     UIOX_SATA_DEV_SEMB,            /**< Enclosure management bridge      */
     UIOX_SATA_DEV_PM,              /**< Port multiplier                  */
 } uiox_sata_dev_t;
 
 /* =========================================================================
  * Hardware capability flags
  * ====================================================================== */
 
 #define UIOX_SATA_CAP_NCQ           (1u << 0)  /**< Native Command Queuing*/
 #define UIOX_SATA_CAP_SATA3        (1u << 1)  /**< 6 Gb/s link speed     */
 #define UIOX_SATA_CAP_SATA2        (1u << 2)  /**< 3 Gb/s link speed     */
 #define UIOX_SATA_CAP_AHCI         (1u << 3)  /**< AHCI mode             */
 #define UIOX_SATA_CAP_DMA          (1u << 4)  /**< DMA transfers         */
 #define UIOX_SATA_CAP_48BIT_LBA    (1u << 5)  /**< 48-bit LBA addressing */
 #define UIOX_SATA_CAP_SMART        (1u << 6)  /**< S.M.A.R.T support     */
 #define UIOX_SATA_CAP_TRIM         (1u << 7)  /**< TRIM (SSD)            */
 #define UIOX_SATA_CAP_DEVSLP       (1u << 8)  /**< Device sleep (DEVSLP) */
 #define UIOX_SATA_CAP_HOTPLUG      (1u << 9)  /**< Hot-plug support      */
 #define UIOX_SATA_CAP_PM           (1u << 10) /**< Port multiplier       */
 #define UIOX_SATA_CAP_FBS          (1u << 11) /**< FIS-based switching   */
 #define UIOX_SATA_CAP_ALPM         (1u << 12) /**< Aggressive link PM    */
 #define UIOX_SATA_CAP_LED          (1u << 13) /**< SGPIO activity LED    */
 
 /* =========================================================================
  * AHCI Generic Host Control Registers (GHC — offset from BAR5)
  * ====================================================================== */
 
 #define AHCI_GHC_CAP                0x0000u  /**< Host capabilities       */
 #define AHCI_GHC_GHC                0x0004u  /**< Global host control     */
 #define AHCI_GHC_IS                 0x0008u  /**< Interrupt status        */
 #define AHCI_GHC_PI                 0x000Cu  /**< Ports implemented       */
 #define AHCI_GHC_VS                 0x0010u  /**< AHCI version            */
 #define AHCI_GHC_CCC_CTL            0x0014u  /**< CCC control             */
 #define AHCI_GHC_CCC_PORTS          0x0018u  /**< CCC ports               */
 #define AHCI_GHC_EM_LOC             0x001Cu  /**< Enclosure mgmt location */
 #define AHCI_GHC_EM_CTL             0x0020u  /**< Enclosure mgmt control  */
 #define AHCI_GHC_CAP2               0x0024u  /**< Extended capabilities   */
 #define AHCI_GHC_BOHC               0x0028u  /**< BIOS/OS handoff         */
 
 /* GHC bits */
 #define AHCI_GHC_GHC_AHCI_EN       (1u << 31)
 #define AHCI_GHC_GHC_MRSM          (1u << 2)
 #define AHCI_GHC_GHC_IE            (1u << 1)
 #define AHCI_GHC_GHC_HR            (1u << 0)  /**< HBA reset              */
 
 /* CAP bits */
 #define AHCI_CAP_S64A              (1u << 31) /**< 64-bit addr support    */
 #define AHCI_CAP_SNCQ              (1u << 30) /**< NCQ support            */
 #define AHCI_CAP_SSNTF             (1u << 29) /**< SNotification          */
 #define AHCI_CAP_SMPS              (1u << 28) /**< Mech presence switch   */
 #define AHCI_CAP_SSS               (1u << 27) /**< Staggered spin-up      */
 #define AHCI_CAP_SALP              (1u << 26) /**< Aggressive link PM     */
 #define AHCI_CAP_SIS               (1u << 22) /**< Single interrupt per S */
 #define AHCI_CAP_NCS_SHIFT         8u
 #define AHCI_CAP_NCS_MASK          (0x1Fu << AHCI_CAP_NCS_SHIFT)
 #define AHCI_CAP_NP_MASK           0x1Fu      /**< Number of ports - 1    */
 
 /* =========================================================================
  * AHCI Per-Port Registers (PxREG — base = BAR5 + 0x100 + port*0x80)
  * ====================================================================== */
 
 #define AHCI_PORT_BASE(p)           (0x0100u + (uint32_t)(p) * 0x80u)
 
 #define AHCI_PX_CLB                 0x00u  /**< Cmd list base addr (lo)   */
 #define AHCI_PX_CLBU                0x04u  /**< Cmd list base addr (hi)   */
 #define AHCI_PX_FB                  0x08u  /**< FIS base addr (lo)        */
 #define AHCI_PX_FBU                 0x0Cu  /**< FIS base addr (hi)        */
 #define AHCI_PX_IS                  0x10u  /**< Interrupt status          */
 #define AHCI_PX_IE                  0x14u  /**< Interrupt enable          */
 #define AHCI_PX_CMD                 0x18u  /**< Command and status        */
 #define AHCI_PX_TFD                 0x20u  /**< Task file data            */
 #define AHCI_PX_SIG                 0x24u  /**< Signature                 */
 #define AHCI_PX_SSTS                0x28u  /**< SATA status (SStatus)     */
 #define AHCI_PX_SCTL                0x2Cu  /**< SATA control (SControl)   */
 #define AHCI_PX_SERR                0x30u  /**< SATA error (SError)       */
 #define AHCI_PX_SACT                0x34u  /**< SATA active (NCQ)         */
 #define AHCI_PX_CI                  0x38u  /**< Command issue             */
 #define AHCI_PX_SNTF                0x3Cu  /**< SATA notification         */
 #define AHCI_PX_FBS                 0x40u  /**< FIS-based switching ctrl  */
 
 /* PxCMD bits */
 #define AHCI_PX_CMD_ST              (1u << 0)   /**< Start                */
 #define AHCI_PX_CMD_SUD             (1u << 1)   /**< Spin-up device       */
 #define AHCI_PX_CMD_POD             (1u << 2)   /**< Power on device      */
 #define AHCI_PX_CMD_CLO             (1u << 3)   /**< Cmd list override    */
 #define AHCI_PX_CMD_FRE             (1u << 4)   /**< FIS receive enable   */
 #define AHCI_PX_CMD_FR              (1u << 14)  /**< FIS receive running  */
 #define AHCI_PX_CMD_CR              (1u << 15)  /**< Cmd list running     */
 #define AHCI_PX_CMD_ASP             (1u << 27)  /**< Aggressive slumber PM*/
 #define AHCI_PX_CMD_ALPE            (1u << 26)  /**< Aggressive link PM   */
 #define AHCI_PX_CMD_ICC_ACTIVE      (1u << 28)  /**< Interface active     */
 
 /* PxIS / PxIE bits */
 #define AHCI_PX_IS_DHRS             (1u << 0)   /**< D2H Register FIS     */
 #define AHCI_PX_IS_PSS              (1u << 1)   /**< PIO setup FIS        */
 #define AHCI_PX_IS_DSS              (1u << 2)   /**< DMA setup FIS        */
 #define AHCI_PX_IS_SDBS             (1u << 3)   /**< Set device bits FIS  */
 #define AHCI_PX_IS_UFS              (1u << 4)   /**< Unknown FIS          */
 #define AHCI_PX_IS_DPS              (1u << 5)   /**< Descriptor processed */
 #define AHCI_PX_IS_PCS              (1u << 6)   /**< Port connect changed */
 #define AHCI_PX_IS_DMPS             (1u << 7)   /**< Device mechanical PS */
 #define AHCI_PX_IS_TFES             (1u << 30)  /**< Task file error      */
 #define AHCI_PX_IS_HBFS             (1u << 29)  /**< Host bus fatal error */
 #define AHCI_PX_IS_HBDS             (1u << 28)  /**< Host bus data error  */
 #define AHCI_PX_IS_IFS              (1u << 27)  /**< Interface fatal err  */
 #define AHCI_PX_IS_INFS             (1u << 26)  /**< Interface non-fatal  */
 #define AHCI_PX_IS_OFS              (1u << 24)  /**< Overflow             */
 #define AHCI_PX_IS_IPMS             (1u << 23)  /**< Incorrect PM         */
 
 /* PxSSTS DET field (device detection) */
 #define AHCI_PX_SSTS_DET_MASK       0x0Fu
 #define AHCI_PX_SSTS_DET_NONE       0x00u  /**< No device, no phy        */
 #define AHCI_PX_SSTS_DET_PRESENT    0x01u  /**< Device present, no comm  */
 #define AHCI_PX_SSTS_DET_COMM       0x03u  /**< Device present, comm OK  */
 #define AHCI_PX_SSTS_DET_OFFLINE    0x04u  /**< Phy offline              */
 
 /* PxSSTS SPD field (interface speed) */
 #define AHCI_PX_SSTS_SPD_SHIFT      4u
 #define AHCI_PX_SSTS_SPD_MASK       (0x0Fu << AHCI_PX_SSTS_SPD_SHIFT)
 #define AHCI_PX_SSTS_SPD_GEN1       (0x01u << AHCI_PX_SSTS_SPD_SHIFT) /* 1.5G */
 #define AHCI_PX_SSTS_SPD_GEN2       (0x02u << AHCI_PX_SSTS_SPD_SHIFT) /* 3G   */
 #define AHCI_PX_SSTS_SPD_GEN3       (0x03u << AHCI_PX_SSTS_SPD_SHIFT) /* 6G   */
 
 /* PxSIG — device signature values */
 #define AHCI_SIG_ATA                0x00000101u  /**< ATA device           */
 #define AHCI_SIG_ATAPI              0xEB140101u  /**< ATAPI device         */
 #define AHCI_SIG_SEMB               0xC33C0101u  /**< SEMB                 */
 #define AHCI_SIG_PM                 0x96690101u  /**< Port multiplier      */
 
 /* =========================================================================
  * ATA commands (subset used by this driver)
  * ====================================================================== */
 
 #define ATA_CMD_IDENTIFY            0xECu
 #define ATA_CMD_IDENTIFY_PACKET     0xA1u
 #define ATA_CMD_READ_DMA_EXT        0x25u  /**< 48-bit LBA DMA read       */
 #define ATA_CMD_WRITE_DMA_EXT       0x35u  /**< 48-bit LBA DMA write      */
 #define ATA_CMD_READ_FPDMA_QUEUED   0x60u  /**< NCQ read                  */
 #define ATA_CMD_WRITE_FPDMA_QUEUED  0x61u  /**< NCQ write                 */
 #define ATA_CMD_FLUSH_CACHE_EXT     0xEAu  /**< Flush write cache         */
 #define ATA_CMD_DATA_SET_MGMT       0x06u  /**< TRIM (DSM)                */
 #define ATA_CMD_SMART               0xB0u
 #define ATA_CMD_SMART_READ_VALUES   0xD0u  /**< SMART sub-command         */
 #define ATA_CMD_SMART_ENABLE        0xD8u
 #define ATA_CMD_SET_FEATURES        0xEFu
 #define ATA_CMD_STANDBY_IMMEDIATE   0xE0u
 #define ATA_CMD_SLEEP               0xE6u
 #define ATA_CMD_SECURITY_FREEZE     0xF5u
 
 /* ATA STATUS register bits */
 #define ATA_SR_BSY                  0x80u
 #define ATA_SR_DRDY                 0x40u
 #define ATA_SR_DF                   0x20u
 #define ATA_SR_DSC                  0x10u
 #define ATA_SR_DRQ                  0x08u
 #define ATA_SR_CORR                 0x04u
 #define ATA_SR_IDX                  0x02u
 #define ATA_SR_ERR                  0x01u
 
 /* =========================================================================
  * FIS types
  * ====================================================================== */
 
 #define FIS_TYPE_REG_H2D            0x27u  /**< Host → Device register    */
 #define FIS_TYPE_REG_D2H            0x34u  /**< Device → Host register    */
 #define FIS_TYPE_DMA_ACT            0x39u  /**< DMA activate              */
 #define FIS_TYPE_DMA_SETUP          0x41u  /**< DMA setup                 */
 #define FIS_TYPE_DATA               0x46u  /**< Bidirectional data        */
 #define FIS_TYPE_BIST               0x58u  /**< BIST activate             */
 #define FIS_TYPE_PIO_SETUP          0x5Fu  /**< PIO setup                 */
 #define FIS_TYPE_DEV_BITS           0xA1u  /**< Set device bits           */
 
 /* H2D Register FIS (20 bytes) */
 typedef struct __attribute__((packed)) {
     uint8_t  fis_type;    /**< FIS_TYPE_REG_H2D                          */
     uint8_t  c_pm;        /**< Bit 7 = C (1=cmd, 0=ctrl), [3:0]=PM port */
     uint8_t  command;     /**< ATA command                               */
     uint8_t  featurel;    /**< Feature low                               */
     uint8_t  lba0;        /**< LBA[7:0]                                  */
     uint8_t  lba1;        /**< LBA[15:8]                                 */
     uint8_t  lba2;        /**< LBA[23:16]                                */
     uint8_t  device;      /**< Device register                           */
     uint8_t  lba3;        /**< LBA[31:24]                                */
     uint8_t  lba4;        /**< LBA[39:32]                                */
     uint8_t  lba5;        /**< LBA[47:40]                                */
     uint8_t  featureh;    /**< Feature high                              */
     uint8_t  countl;      /**< Sector count low                          */
     uint8_t  counth;      /**< Sector count high                         */
     uint8_t  icc;         /**< Isochronous command completion            */
     uint8_t  control;     /**< Control register                          */
     uint8_t  rsv[4];
 } uiox_sata_fis_h2d_t;
 
 /* D2H Register FIS (20 bytes) */
 typedef struct __attribute__((packed)) {
     uint8_t  fis_type;
     uint8_t  flags;
     uint8_t  status;
     uint8_t  error;
     uint8_t  lba0, lba1, lba2, device;
     uint8_t  lba3, lba4, lba5, rsv0;
     uint8_t  countl, counth;
     uint8_t  rsv1[6];
 } uiox_sata_fis_d2h_t;
 
 /* =========================================================================
  * Command header (32 bytes, part of Command List)
  * ====================================================================== */
 
 typedef struct __attribute__((packed, aligned(32))) {
     uint16_t cfl_flags;   /**< Cmd FIS length [4:0] in DW, flags [15:5] */
     uint16_t prdtl;       /**< PRDT length (entries)                    */
     uint32_t prdbc;       /**< PRD byte count (written by HBA)           */
     uint32_t ctba;        /**< Cmd table base addr lo                    */
     uint32_t ctbau;       /**< Cmd table base addr hi                    */
     uint32_t rsv[4];
 } uiox_sata_cmd_hdr_t;
 
 /* cmd_hdr cfl_flags bits */
 #define SATA_CH_WRITE               (1u << 6)   /**< Write direction       */
 #define SATA_CH_ATAPI               (1u << 5)   /**< ATAPI command         */
 #define SATA_CH_RESET               (1u << 8)   /**< Reset                 */
 #define SATA_CH_PREFETCH            (1u << 7)   /**< Prefetchable          */
 #define SATA_CH_CLR_BUSY            (1u << 10)  /**< Clear busy on R_OK    */
 
 /* =========================================================================
  * Physical Region Descriptor Table entry (8 bytes)
  * ====================================================================== */
 
 typedef struct __attribute__((packed)) {
     uint32_t dba;         /**< Data base address lo                      */
     uint32_t dbau;        /**< Data base address hi                      */
     uint32_t rsv;
     uint32_t dbc;         /**< Byte count - 1, bit31 = interrupt on comp */
 } uiox_sata_prd_t;
 
 #define SATA_PRD_IOC                (1u << 31)  /**< Interrupt on complete */
 #define SATA_PRD_MAX_BYTES          (4u*1024u*1024u - 2u) /* 4 MB - 2     */
 
 /* =========================================================================
  * NCQ constants
  * ====================================================================== */
 
 #define SATA_NCQ_DEPTH_MAX          32u
 #define SATA_SECTOR_SIZE            512u
 
 /* =========================================================================
  * IDENTIFY data offsets (word indices)
  * ====================================================================== */
 
 #define ATA_ID_WORDS                256u
 #define ATA_ID_MODEL                27u   /**< Words 27–46: model string  */
 #define ATA_ID_FW_REV               23u   /**< Words 23–26: FW revision   */
 #define ATA_ID_SERIAL               10u   /**< Words 10–19: serial number */
 #define ATA_ID_CAPABILITIES         49u
 #define ATA_ID_COMMAND_SET_2        83u
 #define ATA_ID_LBA48_SECTORS        100u  /**< Words 100–103: 48-bit LBA  */
 #define ATA_ID_SECTOR_SIZE          106u
 #define ATA_ID_QUEUE_DEPTH          75u   /**< NCQ queue depth - 1        */
 #define ATA_ID_SATA_CAPS            76u
 #define ATA_ID_RPM                  217u  /**< Rotation rate (0 = SSD)    */
 
 /* =========================================================================
  * Hardware device descriptor
  * ====================================================================== */
 
 #define UIOX_SATA_MODEL_LEN         48u
 #define UIOX_SATA_MAX_PORTS         32u
 #define UIOX_SATA_MAX_NCQ_CMDS      32u
 #define UIOX_SATA_PRD_PER_CMD       8u    /**< PRD entries per command    */
 
 typedef struct {
     /* Identify data */
     char     model[ATA_ID_WORDS];      /**< Raw IDENTIFY word buffer      */
     char     model_str[41];            /**< Null-term model string        */
     char     serial_str[21];           /**< Null-term serial number       */
     char     fw_str[9];                /**< Null-term firmware rev        */
     uint64_t lba48_sectors;            /**< Total sectors (48-bit LBA)   */
     uint64_t capacity_bytes;
     uint32_t rpm;                      /**< 0 = SSD                      */
     uint8_t  ncq_depth;               /**< Queue depth (1–32)            */
     bool     is_ssd;
     bool     trim_supported;
     bool     smart_supported;
 } uiox_sata_ident_t;
 
 typedef struct {
     uintptr_t          bar5;           /**< AHCI BAR5 MMIO base           */
     uint32_t           irq;
     uint32_t           caps;
     uiox_sata_ctrl_t   ctrl_type;
     char               model[UIOX_SATA_MODEL_LEN];
     uint8_t            port;           /**< Active SATA port (0–31)       */
     uint8_t            num_ports;      /**< Ports implemented             */
     /* Device state */
     uiox_sata_dev_t    dev_type;
     uiox_sata_ident_t  ident;
     bool               dev_present;
     bool               dev_ready;
     /* NCQ tag bitmap (bit N = tag N in use) */
     uint32_t           ncq_active;
     /* Pending IRQ */
     volatile uint32_t  pending_irq;
     /* Private (ops vtable) */
     void              *priv;
 } uiox_sata_hw_t;
 
 /* Pending IRQ bits */
 #define UIOX_SATA_IRQ_D2H           (1u << 0)  /**< D2H FIS received      */
 #define UIOX_SATA_IRQ_DMA_DONE      (1u << 1)  /**< DMA complete          */
 #define UIOX_SATA_IRQ_ERROR         (1u << 2)  /**< Task file error       */
 #define UIOX_SATA_IRQ_HOTPLUG       (1u << 3)  /**< Port connect change   */
 #define UIOX_SATA_IRQ_NCQ_DONE      (1u << 4)  /**< Set Device Bits FIS   */
 
 /* =========================================================================
  * Hardware operations vtable (18-op table)
  * ====================================================================== */
 
 typedef struct {
     /* Lifecycle */
     int  (*init)         (uiox_sata_hw_t *hw);
     void (*deinit)       (uiox_sata_hw_t *hw);
     int  (*port_start)   (uiox_sata_hw_t *hw, uint8_t port);
     void (*port_stop)    (uiox_sata_hw_t *hw, uint8_t port);
 
     /* AHCI MMIO — GHC registers */
     uint32_t (*ghc_read) (uiox_sata_hw_t *hw, uint32_t offset);
     void (*ghc_write)    (uiox_sata_hw_t *hw, uint32_t offset,
                           uint32_t val);
 
     /* AHCI MMIO — per-port registers */
     uint32_t (*px_read)  (uiox_sata_hw_t *hw, uint8_t port,
                           uint32_t offset);
     void (*px_write)     (uiox_sata_hw_t *hw, uint8_t port,
                           uint32_t offset, uint32_t val);
 
     /* Command issue */
     int  (*cmd_issue)    (uiox_sata_hw_t *hw, uint8_t port,
                           uint8_t slot,
                           const uiox_sata_fis_h2d_t *fis,
                           bool write,
                           uintptr_t data_phys, uint32_t data_len);
 
     /* Data transfer */
     int  (*read_sectors) (uiox_sata_hw_t *hw, uint64_t lba,
                           uint8_t *buf, uint32_t count);
     int  (*write_sectors)(uiox_sata_hw_t *hw, uint64_t lba,
                           const uint8_t *buf, uint32_t count);
 
     /* NCQ */
     int  (*ncq_read)     (uiox_sata_hw_t *hw, uint64_t lba,
                           uint8_t *buf, uint32_t count, uint8_t tag);
     int  (*ncq_write)    (uiox_sata_hw_t *hw, uint64_t lba,
                           const uint8_t *buf, uint32_t count, uint8_t tag);
 
     /* Port reset (COMRESET) */
     int  (*port_reset)   (uiox_sata_hw_t *hw, uint8_t port);
 
     /* GPIO */
     void (*gpio_write)   (uiox_sata_hw_t *hw, uint32_t pin, bool val);
     bool (*gpio_read)    (uiox_sata_hw_t *hw, uint32_t pin);
 
     /* SMART */
     int  (*smart_read)   (uiox_sata_hw_t *hw, uint8_t *buf);
 
     /* ISR */
     void (*isr)          (uiox_sata_hw_t *hw);
 } uiox_sata_hw_ops_t;
 
 /* =========================================================================
  * HAL public API
  * ====================================================================== */
 
 int      uiox_sata_hw_init        (uiox_sata_hw_t *hw,
                                     const uiox_sata_hw_ops_t *ops);
 void     uiox_sata_hw_deinit      (uiox_sata_hw_t *hw);
 int      uiox_sata_hw_port_start  (uiox_sata_hw_t *hw, uint8_t port);
 void     uiox_sata_hw_port_stop   (uiox_sata_hw_t *hw, uint8_t port);
 uint32_t uiox_sata_hw_ghc_read    (uiox_sata_hw_t *hw, uint32_t off);
 void     uiox_sata_hw_ghc_write   (uiox_sata_hw_t *hw,
                                     uint32_t off, uint32_t val);
 uint32_t uiox_sata_hw_px_read     (uiox_sata_hw_t *hw,
                                     uint8_t port, uint32_t off);
 void     uiox_sata_hw_px_write    (uiox_sata_hw_t *hw,
                                     uint8_t port, uint32_t off, uint32_t val);
 int      uiox_sata_hw_cmd_issue   (uiox_sata_hw_t *hw, uint8_t port,
                                     uint8_t slot,
                                     const uiox_sata_fis_h2d_t *fis,
                                     bool write,
                                     uintptr_t data_phys, uint32_t len);
 int      uiox_sata_hw_read_sectors(uiox_sata_hw_t *hw, uint64_t lba,
                                     uint8_t *buf, uint32_t count);
 int      uiox_sata_hw_write_sectors(uiox_sata_hw_t *hw, uint64_t lba,
                                      const uint8_t *buf, uint32_t count);
 int      uiox_sata_hw_port_reset  (uiox_sata_hw_t *hw, uint8_t port);
 int      uiox_sata_hw_smart_read  (uiox_sata_hw_t *hw, uint8_t *buf);
 
 static inline bool uiox_sata_dev_present(const uiox_sata_hw_t *hw)
 { return hw ? hw->dev_present : false; }
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_SATA_HW_H */
 