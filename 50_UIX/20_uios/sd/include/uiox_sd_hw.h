/**
 * @file  uiox_sd_hw.h
 * @brief UIOX SD Card Reader Hardware Abstraction Layer (HAL).
 *
 * Supports:
 *   - SDIO 1-bit / 4-bit native mode (SD Host Controller Spec v3/v4)
 *   - SPI mode (fallback, all SD cards)
 *   - Micro-SD (push-push slot) and full-size SD (push-pull slot)
 *   - Card families: SDSC (≤2 GB), SDHC (4–32 GB), SDXC (64 GB–2 TB)
 *
 * Owns:
 *   - SDIO host controller MMIO register access
 *   - SPI master register access (clock, CS, MOSI/MISO)
 *   - GPIO: CD# (card detect, active-low), WP (write-protect, active-high)
 *   - DMA transfer descriptors for block read/write
 *   - CRC7 (command) and CRC16 (data) hardware or SW calculation
 *   - Clock frequency control (400 kHz ID, up to 50/104 MHz transfer)
 *   - Bus width control (1-bit → 4-bit after ACMD6)
 *
 * @version 1.0.0
 * @date    2026-06-11
 */

 #ifndef UIOX_SD_HW_H
 #define UIOX_SD_HW_H
 
 #include <stdint.h>
 #include <stdbool.h>
 #include <stddef.h>
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * Bus interface variant
  * ====================================================================== */
 
 typedef enum {
     UIOX_SD_BUS_SDIO_1BIT = 0,  /**< SDIO native 1-bit (DAT0 only)      */
     UIOX_SD_BUS_SDIO_4BIT,       /**< SDIO native 4-bit (DAT0–3)         */
     UIOX_SD_BUS_SPI,             /**< SPI mode (CS/CLK/MOSI/MISO)        */
 } uiox_sd_bus_t;
 
 /* =========================================================================
  * Card type (detected during initialisation)
  * ====================================================================== */
 
 typedef enum {
     UIOX_SD_CARD_NONE   = 0,
     UIOX_SD_CARD_SDSC,           /**< Standard Capacity  ≤ 2 GB          */
     UIOX_SD_CARD_SDHC,           /**< High Capacity      4–32 GB         */
     UIOX_SD_CARD_SDXC,           /**< Extended Capacity  64 GB–2 TB      */
     UIOX_SD_CARD_UNKNOWN,
 } uiox_sd_card_type_t;
 
 /* =========================================================================
  * Hardware capability flags
  * ====================================================================== */
 
 #define UIOX_SD_CAP_SDIO_4BIT       (1u << 0)  /**< 4-bit SDIO bus        */
 #define UIOX_SD_CAP_SPI             (1u << 1)  /**< SPI fallback mode     */
 #define UIOX_SD_CAP_DMA             (1u << 2)  /**< DMA block transfer    */
 #define UIOX_SD_CAP_HIGH_SPEED      (1u << 3)  /**< 50 MHz HS mode        */
 #define UIOX_SD_CAP_UHS_I           (1u << 4)  /**< UHS-I (104 MHz)       */
 #define UIOX_SD_CAP_CD_GPIO         (1u << 5)  /**< Card-detect GPIO      */
 #define UIOX_SD_CAP_WP_GPIO         (1u << 6)  /**< Write-protect GPIO    */
 #define UIOX_SD_CAP_CRC_HW          (1u << 7)  /**< Hardware CRC engine   */
 #define UIOX_SD_CAP_MULTIBLOCK      (1u << 8)  /**< CMD18/25 multi-block  */
 #define UIOX_SD_CAP_AUTO_CMD12      (1u << 9)  /**< Auto-CMD12 stop       */
 #define UIOX_SD_CAP_VOLTAGE_18V     (1u << 10) /**< 1.8 V signalling      */
 
 /* =========================================================================
  * SDIO Host Controller register offsets (SD Host Spec v3)
  * ====================================================================== */
 
 #define SDIO_REG_DMA_ADDR           0x00u
 #define SDIO_REG_BLK_SIZE           0x04u
 #define SDIO_REG_BLK_COUNT          0x06u
 #define SDIO_REG_ARG                0x08u
 #define SDIO_REG_TRANS_MODE         0x0Cu
 #define SDIO_REG_CMD                0x0Eu
 #define SDIO_REG_RESP0              0x10u
 #define SDIO_REG_RESP1              0x14u
 #define SDIO_REG_RESP2              0x18u
 #define SDIO_REG_RESP3              0x1Cu
 #define SDIO_REG_DATA               0x20u
 #define SDIO_REG_PRESENT_STATE      0x24u
 #define SDIO_REG_HOST_CTRL1         0x28u
 #define SDIO_REG_PWR_CTRL           0x29u
 #define SDIO_REG_BLK_GAP_CTRL       0x2Au
 #define SDIO_REG_WAKEUP_CTRL        0x2Bu
 #define SDIO_REG_CLK_CTRL           0x2Cu
 #define SDIO_REG_TIMEOUT_CTRL       0x2Eu
 #define SDIO_REG_SW_RESET           0x2Fu
 #define SDIO_REG_INT_STATUS         0x30u
 #define SDIO_REG_INT_STATUS_EN      0x34u
 #define SDIO_REG_INT_SIGNAL_EN      0x38u
 #define SDIO_REG_AUTO_CMD_STATUS    0x3Cu
 #define SDIO_REG_HOST_CTRL2         0x3Eu
 #define SDIO_REG_CAPS0              0x40u
 #define SDIO_REG_CAPS1              0x44u
 #define SDIO_REG_ADMA_ADDR_LO       0x58u
 #define SDIO_REG_ADMA_ADDR_HI       0x5Cu
 
 /* PRESENT_STATE bits */
 #define SDIO_PS_CMD_INHIBIT         (1u << 0)
 #define SDIO_PS_DAT_INHIBIT         (1u << 1)
 #define SDIO_PS_CARD_INSERTED       (1u << 16)
 #define SDIO_PS_WRITE_PROTECT       (1u << 19)
 #define SDIO_PS_DATA_AVAILABLE      (1u << 11)
 #define SDIO_PS_SPACE_AVAILABLE     (1u << 10)
 
 /* HOST_CTRL1 bits */
 #define SDIO_HC1_LED                (1u << 0)
 #define SDIO_HC1_4BIT_MODE          (1u << 1)
 #define SDIO_HC1_HIGH_SPEED         (1u << 2)
 #define SDIO_HC1_DMA_SEL_ADMA2      (0x02u << 3)
 
 /* CLK_CTRL bits */
 #define SDIO_CLK_INT_CLK_EN         (1u << 0)
 #define SDIO_CLK_INT_CLK_STABLE     (1u << 1)
 #define SDIO_CLK_SD_CLK_EN          (1u << 2)
 #define SDIO_CLK_FREQ_SEL_SHIFT     8u
 
 /* INT_STATUS bits */
 #define SDIO_INT_CMD_COMPLETE       (1u << 0)
 #define SDIO_INT_XFER_COMPLETE      (1u << 1)
 #define SDIO_INT_DMA_INT            (1u << 3)
 #define SDIO_INT_BUF_WR_READY       (1u << 4)
 #define SDIO_INT_BUF_RD_READY       (1u << 5)
 #define SDIO_INT_CARD_INSERT        (1u << 6)
 #define SDIO_INT_CARD_REMOVE        (1u << 7)
 #define SDIO_INT_ERROR              (1u << 15)
 #define SDIO_INT_CMD_TIMEOUT        (1u << 16)
 #define SDIO_INT_CMD_CRC_ERR        (1u << 17)
 #define SDIO_INT_DAT_TIMEOUT        (1u << 20)
 #define SDIO_INT_DAT_CRC_ERR        (1u << 21)
 
 /* TRANS_MODE bits */
 #define SDIO_TM_DMA_EN              (1u << 0)
 #define SDIO_TM_BLK_CNT_EN         (1u << 1)
 #define SDIO_TM_AUTO_CMD12          (1u << 2)
 #define SDIO_TM_DATA_DIR_READ       (1u << 4)
 #define SDIO_TM_MULTI_BLOCK         (1u << 5)
 
 /* SW_RESET bits */
 #define SDIO_SW_RESET_ALL           0x01u
 #define SDIO_SW_RESET_CMD           0x02u
 #define SDIO_SW_RESET_DAT           0x04u
 
 /* =========================================================================
  * SD Commands
  * ====================================================================== */
 
 #define SD_CMD0_GO_IDLE             0u
 #define SD_CMD2_ALL_SEND_CID        2u
 #define SD_CMD3_SEND_REL_ADDR       3u
 #define SD_CMD4_SET_DSR             4u
 #define SD_CMD6_SWITCH_FUNC         6u
 #define SD_CMD7_SELECT_CARD         7u
 #define SD_CMD8_SEND_IF_COND        8u
 #define SD_CMD9_SEND_CSD            9u
 #define SD_CMD10_SEND_CID           10u
 #define SD_CMD12_STOP_XMISSION      12u
 #define SD_CMD13_SEND_STATUS        13u
 #define SD_CMD16_SET_BLOCKLEN       16u
 #define SD_CMD17_READ_SINGLE_BLOCK  17u
 #define SD_CMD18_READ_MULTI_BLOCK   18u
 #define SD_CMD24_WRITE_BLOCK        24u
 #define SD_CMD25_WRITE_MULTI_BLOCK  25u
 #define SD_CMD55_APP_CMD            55u
 #define SD_CMD58_READ_OCR           58u  /* SPI only */
 #define SD_ACMD6_SET_BUS_WIDTH      6u
 #define SD_ACMD41_SD_SEND_OP_COND   41u
 #define SD_ACMD51_SEND_SCR          51u
 
 /* CMD8 voltage pattern */
 #define SD_CMD8_VHS_27_36V          0x100u
 #define SD_CMD8_CHECK_PATTERN       0xAAu
 
 /* ACMD41 flags */
 #define SD_ACMD41_HCS               (1u << 30) /**< Host supports SDHC/X  */
 #define SD_ACMD41_S18R              (1u << 24) /**< 1.8 V request         */
 #define SD_ACMD41_OCR_BUSY          (1u << 31) /**< Card powered-up flag  */
 #define SD_ACMD41_CCS               (1u << 30) /**< Card Capacity Status  */
 
 /* =========================================================================
  * Response types
  * ====================================================================== */
 
 typedef enum {
     SD_RESP_NONE = 0,
     SD_RESP_R1,        /**< 48-bit: card status                           */
     SD_RESP_R1B,       /**< 48-bit + busy signal                          */
     SD_RESP_R2,        /**< 136-bit: CID or CSD                          */
     SD_RESP_R3,        /**< 48-bit: OCR register                         */
     SD_RESP_R6,        /**< 48-bit: published RCA                        */
     SD_RESP_R7,        /**< 48-bit: card interface condition              */
 } uiox_sd_resp_t;
 
 /* =========================================================================
  * SD Card descriptor
  * ====================================================================== */
 
 #define UIOX_SD_BLOCK_SIZE          512u
 #define UIOX_SD_CID_LEN             16u
 #define UIOX_SD_CSD_LEN             16u
 #define UIOX_SD_SCR_LEN             8u
 #define UIOX_SD_MODEL_LEN           48u
 
 typedef struct {
     uint8_t  cid[UIOX_SD_CID_LEN];  /**< Card Identification Register    */
     uint8_t  csd[UIOX_SD_CSD_LEN];  /**< Card Specific Data Register     */
     uint8_t  scr[UIOX_SD_SCR_LEN];  /**< SD Configuration Register       */
     uint16_t rca;                     /**< Relative Card Address           */
     uint64_t capacity_blocks;        /**< Total blocks (512 B each)       */
     uint64_t capacity_bytes;         /**< Total capacity in bytes         */
     uint32_t ocr;                     /**< OCR register                    */
     uiox_sd_card_type_t card_type;
 } uiox_sd_card_t;
 
 /* =========================================================================
  * GPIO pin IDs
  * ====================================================================== */
 
 #define UIOX_SD_GPIO_CD             0u  /**< Card-detect (active-low)     */
 #define UIOX_SD_GPIO_WP             1u  /**< Write-protect (active-high)  */
 #define UIOX_SD_GPIO_PWR_EN         2u  /**< Card power enable            */
 
 /* =========================================================================
  * Hardware device descriptor
  * ====================================================================== */
 
 typedef struct {
     uintptr_t        base;           /**< SDIO host controller MMIO base  */
     uint32_t         irq;            /**< Host controller IRQ             */
     uint32_t         caps;
     uiox_sd_bus_t    bus_type;
     char             model[UIOX_SD_MODEL_LEN];
     /* SPI-mode extras */
     uint32_t         spi_bus;        /**< SPI bus index                   */
     uint32_t         spi_cs;         /**< SPI chip-select pin             */
     uint32_t         spi_hz;         /**< SPI clock frequency (Hz)        */
     /* Clock frequencies */
     uint32_t         clk_id_hz;      /**< Identification clock (400 kHz)  */
     uint32_t         clk_xfer_hz;    /**< Transfer clock (25/50/104 MHz)  */
     /* Runtime card info */
     uiox_sd_card_t   card;
     bool             card_present;
     bool             write_protect;
     /* Pending IRQ bitmask */
     volatile uint32_t pending_irq;
     /* Private (ops vtable) */
     void            *priv;
 } uiox_sd_hw_t;
 
 /* Pending IRQ bits */
 #define UIOX_SD_IRQ_CMD_DONE        (1u << 0)
 #define UIOX_SD_IRQ_XFER_DONE       (1u << 1)
 #define UIOX_SD_IRQ_CARD_INSERT     (1u << 2)
 #define UIOX_SD_IRQ_CARD_REMOVE     (1u << 3)
 #define UIOX_SD_IRQ_ERROR           (1u << 4)
 
 /* =========================================================================
  * Hardware operations vtable (18-op table)
  * ====================================================================== */
 
 typedef struct {
     /* Lifecycle */
     int  (*init)         (uiox_sd_hw_t *hw);
     void (*deinit)       (uiox_sd_hw_t *hw);
     int  (*power_on)     (uiox_sd_hw_t *hw);
     void (*power_off)    (uiox_sd_hw_t *hw);
 
     /* SDIO host register access */
     uint32_t (*reg_read) (uiox_sd_hw_t *hw, uint32_t offset);
     void (*reg_write)    (uiox_sd_hw_t *hw, uint32_t offset, uint32_t val);
 
     /* Clock */
     int  (*set_clock)    (uiox_sd_hw_t *hw, uint32_t hz);
 
     /* Bus width */
     int  (*set_bus_width)(uiox_sd_hw_t *hw, uint8_t width);
 
     /* Command / response */
     int  (*send_cmd)     (uiox_sd_hw_t *hw, uint8_t cmd,
                           uint32_t arg, uiox_sd_resp_t resp_type,
                           uint32_t *resp);
 
     /* Data transfer */
     int  (*read_blocks)  (uiox_sd_hw_t *hw, uint32_t lba,
                           uint8_t *buf, uint32_t count);
     int  (*write_blocks) (uiox_sd_hw_t *hw, uint32_t lba,
                           const uint8_t *buf, uint32_t count);
 
     /* DMA */
     int  (*dma_read)     (uiox_sd_hw_t *hw, uint32_t lba,
                           uintptr_t phys, uint32_t count);
     int  (*dma_write)    (uiox_sd_hw_t *hw, uint32_t lba,
                           uintptr_t phys, uint32_t count);
 
     /* CRC */
     uint8_t  (*crc7)     (const uint8_t *data, uint8_t len);
     uint16_t (*crc16)    (const uint8_t *data, uint32_t len);
 
     /* GPIO */
     void (*gpio_write)   (uiox_sd_hw_t *hw, uint32_t pin, bool val);
     bool (*gpio_read)    (uiox_sd_hw_t *hw, uint32_t pin);
 
     /* ISR */
     void (*isr)          (uiox_sd_hw_t *hw);
 } uiox_sd_hw_ops_t;
 
 /* =========================================================================
  * HAL public API
  * ====================================================================== */
 
 int      uiox_sd_hw_init        (uiox_sd_hw_t *hw,
                                   const uiox_sd_hw_ops_t *ops);
 void     uiox_sd_hw_deinit      (uiox_sd_hw_t *hw);
 int      uiox_sd_hw_power_on    (uiox_sd_hw_t *hw);
 void     uiox_sd_hw_power_off   (uiox_sd_hw_t *hw);
 uint32_t uiox_sd_hw_reg_read    (uiox_sd_hw_t *hw, uint32_t offset);
 void     uiox_sd_hw_reg_write   (uiox_sd_hw_t *hw,
                                   uint32_t offset, uint32_t val);
 int      uiox_sd_hw_set_clock   (uiox_sd_hw_t *hw, uint32_t hz);
 int      uiox_sd_hw_set_bus_width(uiox_sd_hw_t *hw, uint8_t width);
 int      uiox_sd_hw_send_cmd    (uiox_sd_hw_t *hw, uint8_t cmd,
                                   uint32_t arg, uiox_sd_resp_t resp_type,
                                   uint32_t *resp);
 int      uiox_sd_hw_read_blocks (uiox_sd_hw_t *hw, uint32_t lba,
                                   uint8_t *buf, uint32_t count);
 int      uiox_sd_hw_write_blocks(uiox_sd_hw_t *hw, uint32_t lba,
                                   const uint8_t *buf, uint32_t count);
 uint8_t  uiox_sd_hw_crc7        (uiox_sd_hw_t *hw,
                                   const uint8_t *data, uint8_t len);
 uint16_t uiox_sd_hw_crc16       (uiox_sd_hw_t *hw,
                                   const uint8_t *data, uint32_t len);
 
 static inline bool uiox_sd_card_present(const uiox_sd_hw_t *hw)
 { return hw ? hw->card_present : false; }
 
 static inline bool uiox_sd_write_protected(const uiox_sd_hw_t *hw)
 { return hw ? hw->write_protect : false; }
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_SD_HW_H */
 