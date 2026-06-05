/**
 * @file    uiox_bios_hw.h
 * @brief   UIOX BIOS Hardware Abstraction Layer (HAL).
 *
 * Lowest-level interface to BIOS/UEFI firmware hardware. Owns:
 *   - SPI NOR flash controller MMIO register access
 *   - Write-protect (WP#) GPIO control
 *   - Boot block hardware lock (HW_WRITE_LOCK)
 *   - System Management Mode (SMM) entry/exit
 *   - CMOS RTC I/O port access (0x70/0x71)
 *   - Firmware Hub (FWH) / LPC / eSPI bus interface
 *   - Trusted Platform Module (TPM) SPI/I2C
 *   - CPU microcode update interface
 *   - Flash erase/program IRQ
 *
 * Supports:
 *   - AMI Aptio UEFI (IBV reference)
 *   - coreboot (open-source BIOS)
 *   - EDK2 / TianoCore UEFI
 *   - Legacy SeaBIOS (BIOS INT services)
 *   - SPI NOR: W25Q128, MX25L12805D, GD25Q127C (16 MB typical)
 *   - eSPI flash (Intel platform hub eSPI)
 *
 * @version 1.0.0
 * @date    2026-06-04
 */
//Layer 1 — Hardware Abstraction
 #ifndef UIOX_BIOS_HW_H
 #define UIOX_BIOS_HW_H
 
 #include <stdint.h>
 #include <stdbool.h>
 #include <stddef.h>
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * Hardware capability flags
  * ====================================================================== */
 
 #define UIOX_BIOS_CAP_SPI_FLASH     (1u << 0)  /**< SPI NOR flash         */
 #define UIOX_BIOS_CAP_ESPI          (1u << 1)  /**< eSPI bus interface    */
 #define UIOX_BIOS_CAP_LPC           (1u << 2)  /**< LPC bus (legacy)      */
 #define UIOX_BIOS_CAP_SMM           (1u << 3)  /**< System Mgmt Mode      */
 #define UIOX_BIOS_CAP_TPM           (1u << 4)  /**< TPM 2.0               */
 #define UIOX_BIOS_CAP_SECURE_BOOT   (1u << 5)  /**< UEFI Secure Boot      */
 #define UIOX_BIOS_CAP_WP_GPIO       (1u << 6)  /**< HW write-protect GPIO */
 #define UIOX_BIOS_CAP_DUAL_BIOS     (1u << 7)  /**< Primary + backup flash */
 #define UIOX_BIOS_CAP_ME_FIRMWARE   (1u << 8)  /**< Intel ME / AMD PSP    */
 #define UIOX_BIOS_CAP_NVRAM_EFI     (1u << 9)  /**< EFI variable store    */
 #define UIOX_BIOS_CAP_NVRAM_CMOS    (1u << 10) /**< CMOS NVRAM (legacy)   */
 #define UIOX_BIOS_CAP_MICROCODE     (1u << 11) /**< CPU microcode update  */
 #define UIOX_BIOS_CAP_ACPI          (1u << 12) /**< ACPI tables           */
 #define UIOX_BIOS_CAP_PXE           (1u << 13) /**< Network boot (PXE)    */
 #define UIOX_BIOS_CAP_RECOVERY      (1u << 14) /**< Flash recovery mode   */
 
 /* =========================================================================
  * BIOS firmware type
  * ====================================================================== */
 
 typedef enum {
     UIOX_BIOS_TYPE_UEFI     = 0,   /**< UEFI / EDK2 / AMI Aptio          */
     UIOX_BIOS_TYPE_LEGACY,          /**< Legacy BIOS (INT 10h/13h/etc.)   */
     UIOX_BIOS_TYPE_COREBOOT,        /**< coreboot open-source             */
     UIOX_BIOS_TYPE_UBOOT,           /**< U-Boot (embedded)                */
 } uiox_bios_type_t;
 
 /* =========================================================================
  * SPI flash operation
  * ====================================================================== */
 
 typedef enum {
     UIOX_SPI_OP_READ        = 0x03u,
     UIOX_SPI_OP_FAST_READ   = 0x0Bu,
     UIOX_SPI_OP_WRITE_EN    = 0x06u,
     UIOX_SPI_OP_WRITE_DIS   = 0x04u,
     UIOX_SPI_OP_PAGE_PROG   = 0x02u,
     UIOX_SPI_OP_SECTOR_ERASE= 0x20u,  /**< 4 KB sector erase            */
     UIOX_SPI_OP_BLOCK_ERASE = 0xD8u,  /**< 64 KB block erase            */
     UIOX_SPI_OP_CHIP_ERASE  = 0x60u,
     UIOX_SPI_OP_READ_SR1    = 0x05u,  /**< Status Register 1            */
     UIOX_SPI_OP_READ_SR2    = 0x35u,  /**< Status Register 2            */
     UIOX_SPI_OP_WRITE_SR    = 0x01u,
     UIOX_SPI_OP_READ_JEDEC  = 0x9Fu,  /**< JEDEC ID (3 bytes)           */
     UIOX_SPI_OP_READ_SFDP   = 0x5Au,  /**< SFDP table                   */
 } uiox_spi_op_t;
 
 /* SPI Status Register bits */
 #define UIOX_SPI_SR_BUSY    (1u << 0)  /**< Write in progress            */
 #define UIOX_SPI_SR_WEL     (1u << 1)  /**< Write Enable Latch           */
 #define UIOX_SPI_SR_BP0     (1u << 2)  /**< Block protect bits           */
 #define UIOX_SPI_SR_BP1     (1u << 3)
 #define UIOX_SPI_SR_BP2     (1u << 4)
 #define UIOX_SPI_SR_TB      (1u << 5)  /**< Top/bottom protect           */
 #define UIOX_SPI_SR_SEC     (1u << 6)  /**< Sector protect               */
 #define UIOX_SPI_SR_SRP     (1u << 7)  /**< Status register protect      */
 
 /* =========================================================================
  * Flash geometry
  * ====================================================================== */
 
 typedef struct {
     uint32_t  total_bytes;   /**< Total flash size (e.g. 16 MB)           */
     uint32_t  sector_bytes;  /**< Sector erase size (4 KB typical)        */
     uint32_t  block_bytes;   /**< Block erase size (64 KB typical)        */
     uint32_t  page_bytes;    /**< Page program size (256 B typical)       */
     uint8_t   jedec_mfr;     /**< JEDEC manufacturer ID                   */
     uint16_t  jedec_dev;     /**< JEDEC device ID                         */
 } uiox_bios_flash_geo_t;
 
 /* =========================================================================
  * Flash region map (Intel descriptor layout)
  * ====================================================================== */
 
 #define UIOX_BIOS_MAX_REGIONS   8
 
 typedef struct {
     const char *name;
     uint32_t    offset;      /**< Offset from flash base                  */
     uint32_t    size;
     bool        readable;
     bool        writable;
     bool        protected;
 } uiox_bios_region_t;
 
 /* =========================================================================
  * Hardware device descriptor
  * ====================================================================== */
 
 #define UIOX_BIOS_VERSION_LEN   32
 #define UIOX_BIOS_VENDOR_LEN    32
 
 typedef struct {
     uintptr_t               spi_base;     /**< SPI controller MMIO base  */
     uintptr_t               flash_mmio;   /**< Flash memory-mapped base  */
     uint32_t                irq_spi;      /**< SPI completion IRQ        */
     uint32_t                caps;
     uiox_bios_type_t        type;
     uiox_bios_flash_geo_t   geo;
     char                    version[UIOX_BIOS_VERSION_LEN]; /**< e.g. "3.2.1" */
     char                    vendor[UIOX_BIOS_VENDOR_LEN];   /**< e.g. "AMI"   */
     uint32_t                build_date;   /**< YYYYMMDD                   */
 
     /* GPIO */
     uint32_t                wp_gpio_pin;  /**< Write-protect GPIO         */
     uint32_t                recovery_pin; /**< Recovery mode GPIO         */
     bool                    wp_active;    /**< Current WP state           */
 
     /* TPM */
     uintptr_t               tpm_base;
     uint8_t                 tpm_i2c_addr;
 
     /* Flash regions */
     uiox_bios_region_t      regions[UIOX_BIOS_MAX_REGIONS];
     uint8_t                 num_regions;
 
     /* State */
     bool                    initialised;
     volatile bool           flash_busy;
 
     void                   *priv;
 } uiox_bios_hw_t;
 
 /* =========================================================================
  * Hardware operations vtable
  * ====================================================================== */
 
 typedef struct {
     int  (*init)            (uiox_bios_hw_t *hw);
     void (*deinit)          (uiox_bios_hw_t *hw);
 
     /* SPI flash operations */
     int  (*spi_read)        (uiox_bios_hw_t *hw,
                              uint32_t offset, void *buf, uint32_t len);
     int  (*spi_write)       (uiox_bios_hw_t *hw,
                              uint32_t offset,
                              const void *buf, uint32_t len);
     int  (*spi_erase_sector)(uiox_bios_hw_t *hw, uint32_t offset);
     int  (*spi_erase_block) (uiox_bios_hw_t *hw, uint32_t offset);
     int  (*spi_erase_chip)  (uiox_bios_hw_t *hw);
     int  (*spi_read_status) (uiox_bios_hw_t *hw, uint8_t *sr1, uint8_t *sr2);
     int  (*spi_read_jedec)  (uiox_bios_hw_t *hw,
                              uint8_t *mfr, uint16_t *dev);
     int  (*spi_wait_ready)  (uiox_bios_hw_t *hw, uint32_t timeout_ms);
 
     /* Write-protect */
     int  (*set_wp)          (uiox_bios_hw_t *hw, bool protect);
     bool (*get_wp)          (uiox_bios_hw_t *hw);
 
     /* CMOS RTC I/O ports */
     uint8_t (*cmos_read)    (uiox_bios_hw_t *hw, uint8_t index);
     void    (*cmos_write)   (uiox_bios_hw_t *hw,
                              uint8_t index, uint8_t val);
 
     /* SMM */
     int  (*smm_enter)       (uiox_bios_hw_t *hw);
     void (*smm_exit)        (uiox_bios_hw_t *hw);
 
     /* TPM */
     int  (*tpm_send)        (uiox_bios_hw_t *hw,
                              const uint8_t *cmd, uint16_t cmd_len,
                              uint8_t *resp, uint16_t *resp_len);
 
     /* Microcode */
     int  (*microcode_update)(uiox_bios_hw_t *hw,
                              const void *ucode, uint32_t size);
 
     /* GPIO */
     bool (*gpio_read)       (uiox_bios_hw_t *hw, uint32_t pin);
     void (*gpio_write)      (uiox_bios_hw_t *hw, uint32_t pin, bool val);
 
     /* ISR */
     void (*isr_spi_done)    (uiox_bios_hw_t *hw);
 } uiox_bios_hw_ops_t;
 
 /* =========================================================================
  * HAL public API
  * ====================================================================== */
 
 int      uiox_bios_hw_init          (uiox_bios_hw_t *hw,
                                       const uiox_bios_hw_ops_t *ops);
 void     uiox_bios_hw_deinit        (uiox_bios_hw_t *hw);
 int      uiox_bios_hw_spi_read      (uiox_bios_hw_t *hw,
                                       uint32_t offset,
                                       void *buf, uint32_t len);
 int      uiox_bios_hw_spi_write     (uiox_bios_hw_t *hw,
                                       uint32_t offset,
                                       const void *buf, uint32_t len);
 int      uiox_bios_hw_spi_erase_sector(uiox_bios_hw_t *hw, uint32_t offset);
 int      uiox_bios_hw_spi_erase_block (uiox_bios_hw_t *hw, uint32_t offset);
 int      uiox_bios_hw_set_wp        (uiox_bios_hw_t *hw, bool protect);
 int      uiox_bios_hw_read_jedec    (uiox_bios_hw_t *hw,
                                       uint8_t *mfr, uint16_t *dev);
 uint8_t  uiox_bios_hw_cmos_read     (uiox_bios_hw_t *hw, uint8_t index);
 void     uiox_bios_hw_cmos_write    (uiox_bios_hw_t *hw,
                                       uint8_t index, uint8_t val);
 int      uiox_bios_hw_tpm_send      (uiox_bios_hw_t *hw,
                                       const uint8_t *cmd, uint16_t cmd_len,
                                       uint8_t *resp, uint16_t *resp_len);
 
 static inline uint32_t uiox_bios_caps(const uiox_bios_hw_t *hw)
 { return hw ? hw->caps : 0u; }
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_BIOS_HW_H */
 