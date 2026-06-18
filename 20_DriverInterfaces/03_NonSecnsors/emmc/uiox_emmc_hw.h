/**
 * @file  uiox_emmc_hw.h
 * @brief UIOX eMMC Hardware Abstraction Layer (HAL).
 *
 * Supports:
 *   - eMMC 5.1 (JEDEC JESD84-B51): HS200, HS400, HS400-ES
 *   - Bus widths: 1-bit, 4-bit, 8-bit
 *   - Partitions: User, Boot1, Boot2, RPMB, GP1–GP4
 *   - Reliable write, packed commands, cache control
 *   - Device Health Report (EXT_CSD[267])
 *   - Power-off notification (POWER_OFF_NOTIFICATION)
 *
 * Owns:
 *   - SDIO host controller MMIO register access
 *   - CMD/DAT line control (MMC command framing)
 *   - CRC7 (command) and CRC16 (data block) calculation
 *   - GPIO: RST_N (hardware reset), PWR_EN (power enable)
 *   - Clock: ident (400 kHz) → HS (52 MHz) → HS200 (200 MHz)
 *                           → HS400 (200 MHz DDR 8-bit)
 *
 * @version 1.0.0
 * @date    2026-06-12
 */

 #ifndef UIOX_EMMC_HW_H
 #define UIOX_EMMC_HW_H
 
 #include <stdint.h>
 #include <stdbool.h>
 #include <stddef.h>
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * eMMC bus speed mode
  * ====================================================================== */
 
 typedef enum {
     UIOX_EMMC_SPEED_IDENT   = 0,  /**< 400 kHz identification           */
     UIOX_EMMC_SPEED_DS,            /**< 25 MHz  default speed            */
     UIOX_EMMC_SPEED_HS52,          /**< 52 MHz  high speed               */
     UIOX_EMMC_SPEED_HS200,         /**< 200 MHz HS200 (SDR)              */
     UIOX_EMMC_SPEED_HS400,         /**< 200 MHz HS400 (DDR 8-bit)        */
     UIOX_EMMC_SPEED_HS400ES,       /**< HS400 Enhanced Strobe            */
 } uiox_emmc_speed_t;
 
 /* =========================================================================
  * eMMC partition access
  * ====================================================================== */
 
 typedef enum {
     UIOX_EMMC_PART_USER     = 0,  /**< User data area                   */
     UIOX_EMMC_PART_BOOT1    = 1,  /**< Boot partition 1                 */
     UIOX_EMMC_PART_BOOT2    = 2,  /**< Boot partition 2                 */
     UIOX_EMMC_PART_RPMB     = 3,  /**< Replay Protected Memory Block    */
     UIOX_EMMC_PART_GP1      = 4,  /**< General purpose 1                */
     UIOX_EMMC_PART_GP2      = 5,  /**< General purpose 2                */
     UIOX_EMMC_PART_GP3      = 6,  /**< General purpose 3                */
     UIOX_EMMC_PART_GP4      = 7,  /**< General purpose 4                */
 } uiox_emmc_part_t;
 
 /* =========================================================================
  * Hardware capability flags
  * ====================================================================== */
 
 #define UIOX_EMMC_CAP_8BIT          (1u << 0)  /**< 8-bit data bus        */
 #define UIOX_EMMC_CAP_4BIT          (1u << 1)  /**< 4-bit data bus        */
 #define UIOX_EMMC_CAP_HS200         (1u << 2)  /**< HS200 speed mode      */
 #define UIOX_EMMC_CAP_HS400         (1u << 3)  /**< HS400 speed mode      */
 #define UIOX_EMMC_CAP_HS400ES       (1u << 4)  /**< HS400 Enhanced Strobe */
 #define UIOX_EMMC_CAP_DDR52         (1u << 5)  /**< DDR 52 MHz            */
 #define UIOX_EMMC_CAP_DMA           (1u << 6)  /**< ADMA2/SDMA DMA        */
 #define UIOX_EMMC_CAP_CACHE         (1u << 7)  /**< eMMC write cache      */
 #define UIOX_EMMC_CAP_TRIM          (1u << 8)  /**< TRIM / Discard        */
 #define UIOX_EMMC_CAP_RPMB          (1u << 9)  /**< RPMB partition        */
 #define UIOX_EMMC_CAP_PACKED_CMD    (1u << 10) /**< Packed commands       */
 #define UIOX_EMMC_CAP_HPI           (1u << 11) /**< High Priority Interrupt*/
 #define UIOX_EMMC_CAP_BKOPS         (1u << 12) /**< Background operations */
 #define UIOX_EMMC_CAP_PON           (1u << 13) /**< Power-off notification*/
 #define UIOX_EMMC_CAP_HEALTH        (1u << 14) /**< Device health report  */
 #define UIOX_EMMC_CAP_RST_N         (1u << 15) /**< Hardware reset RST_N  */
 
 /* =========================================================================
  * SDIO Host Controller registers (SD Host Spec v3, same as uiox_sd)
  * ====================================================================== */
 
 #define EMMC_HC_DMA_ADDR            0x0000u
 #define EMMC_HC_BLK_SIZE            0x0004u
 #define EMMC_HC_BLK_COUNT           0x0006u
 #define EMMC_HC_ARG                 0x0008u
 #define EMMC_HC_TRANS_MODE          0x000Cu
 #define EMMC_HC_CMD                 0x000Eu
 #define EMMC_HC_RESP0               0x0010u
 #define EMMC_HC_RESP1               0x0014u
 #define EMMC_HC_RESP2               0x0018u
 #define EMMC_HC_RESP3               0x001Cu
 #define EMMC_HC_DATA                0x0020u
 #define EMMC_HC_PRESENT_STATE       0x0024u
 #define EMMC_HC_HOST_CTRL1          0x0028u
 #define EMMC_HC_PWR_CTRL            0x0029u
 #define EMMC_HC_CLK_CTRL            0x002Cu
 #define EMMC_HC_TIMEOUT_CTRL        0x002Eu
 #define EMMC_HC_SW_RESET            0x002Fu
 #define EMMC_HC_INT_STATUS          0x0030u
 #define EMMC_HC_INT_STATUS_EN       0x0034u
 #define EMMC_HC_INT_SIGNAL_EN       0x0038u
 #define EMMC_HC_HOST_CTRL2          0x003Eu
 #define EMMC_HC_CAPS0               0x0040u
 #define EMMC_HC_CAPS1               0x0044u
 #define EMMC_HC_ADMA_ADDR_LO        0x0058u
 #define EMMC_HC_ADMA_ADDR_HI        0x005Cu
 
 /* HOST_CTRL1 bits */
 #define EMMC_HC1_4BIT               (1u << 1)
 #define EMMC_HC1_HIGH_SPEED         (1u << 2)
 #define EMMC_HC1_8BIT               (1u << 5)
 #define EMMC_HC1_DMA_ADMA2          (0x02u << 3)
 
 /* HOST_CTRL2 bits (UHS mode select) */
 #define EMMC_HC2_UHS_SDR12          0x0000u
 #define EMMC_HC2_UHS_SDR25          0x0001u
 #define EMMC_HC2_UHS_SDR50          0x0002u
 #define EMMC_HC2_UHS_SDR104         0x0003u
 #define EMMC_HC2_UHS_DDR50          0x0004u
 #define EMMC_HC2_HS400              0x0007u
 #define EMMC_HC2_1V8_EN             (1u << 3)
 #define EMMC_HC2_DRV_STRENGTH_B     (0x01u << 4)
 #define EMMC_HC2_EXEC_TUNING        (1u << 6)
 #define EMMC_HC2_TUNING_OK          (1u << 7)
 #define EMMC_HC2_PRESET_EN          (1u << 15)
 
 /* PRESENT_STATE bits */
 #define EMMC_PS_CMD_INHIBIT         (1u << 0)
 #define EMMC_PS_DAT_INHIBIT         (1u << 1)
 #define EMMC_PS_DAT_ACTIVE          (1u << 2)
 #define EMMC_PS_CARD_INSERTED       (1u << 16)
 
 /* INT_STATUS bits */
 #define EMMC_INT_CMD_COMPLETE       (1u << 0)
 #define EMMC_INT_XFER_COMPLETE      (1u << 1)
 #define EMMC_INT_DMA_INT            (1u << 3)
 #define EMMC_INT_BUF_WR_READY      (1u << 4)
 #define EMMC_INT_BUF_RD_READY      (1u << 5)
 #define EMMC_INT_ERROR              (1u << 15)
 #define EMMC_INT_CMD_TIMEOUT        (1u << 16)
 #define EMMC_INT_CMD_CRC_ERR        (1u << 17)
 #define EMMC_INT_DAT_TIMEOUT        (1u << 20)
 #define EMMC_INT_DAT_CRC_ERR        (1u << 21)
 
 /* TRANS_MODE bits */
 #define EMMC_TM_DMA_EN              (1u << 0)
 #define EMMC_TM_BLK_CNT_EN         (1u << 1)
 #define EMMC_TM_AUTO_CMD12          (1u << 2)
 #define EMMC_TM_AUTO_CMD23          (3u << 2)
 #define EMMC_TM_DATA_DIR_READ       (1u << 4)
 #define EMMC_TM_MULTI_BLOCK         (1u << 5)
 
 /* SW_RESET bits */
 #define EMMC_SW_RESET_ALL           0x01u
 #define EMMC_SW_RESET_CMD           0x02u
 #define EMMC_SW_RESET_DAT           0x04u
 
 /* =========================================================================
  * MMC / eMMC Commands
  * ====================================================================== */
 
 #define MMC_CMD0_GO_IDLE            0u
 #define MMC_CMD1_SEND_OP_COND       1u
 #define MMC_CMD2_ALL_SEND_CID       2u
 #define MMC_CMD3_SET_REL_ADDR       3u
 #define MMC_CMD6_SWITCH             6u   /**< EXT_CSD write (SWITCH)      */
 #define MMC_CMD7_SELECT_CARD        7u
 #define MMC_CMD8_SEND_EXT_CSD       8u   /**< Read EXT_CSD (512 B)        */
 #define MMC_CMD9_SEND_CSD           9u
 #define MMC_CMD10_SEND_CID          10u
 #define MMC_CMD12_STOP_XMISSION     12u
 #define MMC_CMD13_SEND_STATUS       13u
 #define MMC_CMD14_BUSTEST_R         14u
 #define MMC_CMD16_SET_BLOCKLEN      16u
 #define MMC_CMD17_READ_SINGLE       17u
 #define MMC_CMD18_READ_MULTI        18u
 #define MMC_CMD19_BUSTEST_W         19u
 #define MMC_CMD21_SEND_TUNING       21u  /**< HS200 tuning                */
 #define MMC_CMD23_SET_BLOCK_COUNT   23u
 #define MMC_CMD24_WRITE_BLOCK       24u
 #define MMC_CMD25_WRITE_MULTI       25u
 #define MMC_CMD35_ERASE_START       35u
 #define MMC_CMD36_ERASE_END         36u
 #define MMC_CMD38_ERASE             38u
 #define MMC_CMD49_RPMB_WRITE        49u
 #define MMC_CMD43_GET_LOAD          43u
 #define MMC_CMD55_APP_CMD           55u  /**< Not used in eMMC native mode */
 
 /* CMD1 OCR / capacity bits */
 #define MMC_CMD1_HCS                (1u << 30) /**< High capacity support  */
 #define MMC_CMD1_S18R               (1u << 24)
 #define MMC_CMD1_OCR_BUSY           (1u << 31)
 #define MMC_CMD1_CCS                (1u << 30) /**< Card Capacity Status   */
 
 /* CMD6 (SWITCH) access modes */
 #define MMC_SWITCH_CMD_SET          0u
 #define MMC_SWITCH_SET_BITS         1u
 #define MMC_SWITCH_CLR_BITS         2u
 #define MMC_SWITCH_WRITE_BYTE       3u
 
 /* Frequently used EXT_CSD indices */
 #define EXT_CSD_CMDQ_MODE_EN        15u
 #define EXT_CSD_FLUSH_CACHE         32u
 #define EXT_CSD_CACHE_CTRL          33u
 #define EXT_CSD_POWER_OFF_NOTIF     34u
 #define EXT_CSD_PACKED_FAILURE      35u
 #define EXT_CSD_PACKED_CMD_STATUS   36u
 #define EXT_CSD_EXP_EVENTS_STATUS   54u
 #define EXT_CSD_BKOPS_STATUS        246u
 #define EXT_CSD_CORRECTLY_PRG_SECTORS_NUM 242u
 #define EXT_CSD_INI_TIMEOUT_EMU     241u
 #define EXT_CSD_CLASS_6_CTRL        59u
 #define EXT_CSD_HS_TIMING           185u  /**< Bus speed mode selection   */
 #define EXT_CSD_BUS_WIDTH           183u  /**< Bus width selection        */
 #define EXT_CSD_STROBE_SUPPORT      184u  /**< Enhanced strobe support    */
 #define EXT_CSD_PARTITION_CONFIG    179u  /**< Partition access           */
 #define EXT_CSD_BOOT_CONFIG_PROT    178u
 #define EXT_CSD_BOOT_BUS_CONDITIONS 177u
 #define EXT_CSD_ERASE_GROUP_DEF     175u
 #define EXT_CSD_BOOT_SIZE_MULT      226u  /**< Boot partition size ×128KB */
 #define EXT_CSD_RPMB_SIZE_MULT      168u  /**< RPMB size ×128KB          */
 #define EXT_CSD_HC_WP_GRP_SIZE      221u
 #define EXT_CSD_HC_ERASE_GRP_SIZE   224u
 #define EXT_CSD_SEC_COUNT           212u  /**< Sector count (4 bytes LE)  */
 #define EXT_CSD_DEVICE_TYPE         196u  /**< Supported speed modes      */
 #define EXT_CSD_CACHE_SIZE          249u  /**< Cache size (4 bytes LE KB) */
 #define EXT_CSD_DEVICE_LIFE_EST_A   268u  /**< Health: type A (0–10)      */
 #define EXT_CSD_DEVICE_LIFE_EST_B   269u  /**< Health: type B (0–10)      */
 #define EXT_CSD_PRE_EOL_INFO        267u  /**< Pre-EOL info (1/2/3)       */
 
 /* EXT_CSD HS_TIMING values */
 #define EXT_CSD_HS_TIMING_HS        1u
 #define EXT_CSD_HS_TIMING_HS200     2u
 #define EXT_CSD_HS_TIMING_HS400     3u
 
 /* EXT_CSD BUS_WIDTH values */
 #define EXT_CSD_BUS_WIDTH_1         0u
 #define EXT_CSD_BUS_WIDTH_4         1u
 #define EXT_CSD_BUS_WIDTH_8         2u
 #define EXT_CSD_BUS_WIDTH_4_DDR     5u
 #define EXT_CSD_BUS_WIDTH_8_DDR     6u
 
 /* EXT_CSD DEVICE_TYPE bits */
 #define EXT_CSD_DEVICE_TYPE_HS_52   (1u << 1)
 #define EXT_CSD_DEVICE_TYPE_DDR_52  (1u << 2)
 #define EXT_CSD_DEVICE_TYPE_HS200   (1u << 4)
 #define EXT_CSD_DEVICE_TYPE_HS400   (1u << 6)
 
 /* EXT_CSD PRE_EOL values */
 #define EXT_CSD_PRE_EOL_NORMAL      1u
 #define EXT_CSD_PRE_EOL_WARNING     2u
 #define EXT_CSD_PRE_EOL_URGENT      3u
 
 /* GPIO pin IDs */
 #define UIOX_EMMC_GPIO_RST_N        0u  /**< Hardware reset (active-low) */
 #define UIOX_EMMC_GPIO_PWR_EN       1u  /**< Power enable (active-high)  */
 
 /* =========================================================================
  * eMMC response types
  * ====================================================================== */
 
 typedef enum {
     EMMC_RESP_NONE = 0,
     EMMC_RESP_R1,         /**< 48-bit card status                         */
     EMMC_RESP_R1B,        /**< 48-bit + busy DAT0                         */
     EMMC_RESP_R2,         /**< 136-bit CID / CSD                          */
     EMMC_RESP_R3,         /**< 48-bit OCR                                 */
     EMMC_RESP_R4,         /**< 48-bit SDIO (not used in eMMC)             */
     EMMC_RESP_R5,
 } uiox_emmc_resp_t;
 
 /* =========================================================================
  * eMMC device descriptor
  * ====================================================================== */
 
 #define UIOX_EMMC_EXT_CSD_LEN      512u
 #define UIOX_EMMC_CID_LEN          16u
 #define UIOX_EMMC_CSD_LEN          16u
 #define UIOX_EMMC_MODEL_LEN        48u
 #define UIOX_EMMC_BLOCK_SIZE       512u
 #define UIOX_EMMC_MAX_PARTS        8u
 
 /* Per-partition descriptor */
 typedef struct {
     uiox_emmc_part_t  id;
     uint64_t          size_bytes;
     bool              enabled;
 } uiox_emmc_part_info_t;
 
 /* Device identity (parsed from CID + EXT_CSD) */
 typedef struct {
     uint8_t   cid[UIOX_EMMC_CID_LEN];
     uint8_t   csd[UIOX_EMMC_CSD_LEN];
     uint8_t   ext_csd[UIOX_EMMC_EXT_CSD_LEN];
     char      product_name[7];    /**< CID product name (6 chars + NUL)  */
     char      manufacturer_id;    /**< CID MID byte                      */
     uint8_t   fwrev;              /**< CID firmware revision             */
     uint64_t  capacity_sectors;   /**< EXT_CSD SEC_COUNT                 */
     uint64_t  capacity_bytes;
     uint32_t  cache_size_kb;      /**< EXT_CSD CACHE_SIZE                */
     uint32_t  erase_group_sectors;/**< HC erase group size               */
     uint8_t   device_type;        /**< EXT_CSD DEVICE_TYPE               */
     uint8_t   pre_eol_info;       /**< EXT_CSD PRE_EOL_INFO              */
     uint8_t   life_est_a;         /**< EXT_CSD DEVICE_LIFE_EST_A (0–10) */
     uint8_t   life_est_b;         /**< EXT_CSD DEVICE_LIFE_EST_B (0–10) */
     bool      cache_supported;
     bool      trim_supported;
     bool      hpi_supported;
     bool      bkops_supported;
     bool      pon_supported;
     bool      packed_cmd_supported;
     uiox_emmc_part_info_t parts[UIOX_EMMC_MAX_PARTS];
 } uiox_emmc_ident_t;
 
 /* Main hardware descriptor */
 typedef struct {
     uintptr_t            base;        /**< SDIO HC MMIO base              */
     uint32_t             irq;
     uint32_t             caps;
     char                 model[UIOX_EMMC_MODEL_LEN];
     /* Bus state */
     uiox_emmc_speed_t    speed;
     uint8_t              bus_width;   /**< 1, 4, or 8                    */
     uint32_t             clk_hz;      /**< Current clock frequency        */
     uint16_t             rca;         /**< Relative card address          */
     /* Device info */
     uiox_emmc_ident_t    ident;
     bool                 dev_ready;
     uiox_emmc_part_t     active_part; /**< Current partition              */
     /* Cache state */
     bool                 cache_enabled;
     /* Pending IRQ */
     volatile uint32_t    pending_irq;
     /* Private (ops vtable) */
     void                *priv;
 } uiox_emmc_hw_t;
 
 /* Pending IRQ bits */
 #define UIOX_EMMC_IRQ_CMD_DONE      (1u << 0)
 #define UIOX_EMMC_IRQ_XFER_DONE    (1u << 1)
 #define UIOX_EMMC_IRQ_ERROR         (1u << 2)
 #define UIOX_EMMC_IRQ_HPI          (1u << 3)
 
 /* =========================================================================
  * Hardware operations vtable (18-op table)
  * ====================================================================== */
 
 typedef struct {
     int  (*init)          (uiox_emmc_hw_t *hw);
     void (*deinit)        (uiox_emmc_hw_t *hw);
     int  (*power_on)      (uiox_emmc_hw_t *hw);
     void (*power_off)     (uiox_emmc_hw_t *hw);
 
     uint32_t (*reg_read)  (uiox_emmc_hw_t *hw, uint32_t offset);
     void (*reg_write)     (uiox_emmc_hw_t *hw,
                            uint32_t offset, uint32_t val);
 
     int  (*set_clock)     (uiox_emmc_hw_t *hw, uint32_t hz);
     int  (*set_bus_width) (uiox_emmc_hw_t *hw, uint8_t width);
     int  (*set_speed_mode)(uiox_emmc_hw_t *hw, uiox_emmc_speed_t speed);
 
     int  (*send_cmd)      (uiox_emmc_hw_t *hw, uint8_t cmd,
                            uint32_t arg, uiox_emmc_resp_t resp_type,
                            uint32_t *resp);
 
     int  (*read_blocks)   (uiox_emmc_hw_t *hw, uint32_t lba,
                            uint8_t *buf, uint32_t count);
     int  (*write_blocks)  (uiox_emmc_hw_t *hw, uint32_t lba,
                            const uint8_t *buf, uint32_t count);
 
     int  (*read_ext_csd)  (uiox_emmc_hw_t *hw,
                            uint8_t *buf);
     int  (*write_ext_csd) (uiox_emmc_hw_t *hw,
                            uint8_t index, uint8_t val);
 
     int  (*tuning)        (uiox_emmc_hw_t *hw);
 
     uint8_t  (*crc7)      (const uint8_t *data, uint8_t len);
     uint16_t (*crc16)     (const uint8_t *data, uint32_t len);
 
     void (*gpio_write)    (uiox_emmc_hw_t *hw, uint32_t pin, bool val);
     bool (*gpio_read)     (uiox_emmc_hw_t *hw, uint32_t pin);
 
     void (*isr)           (uiox_emmc_hw_t *hw);
 } uiox_emmc_hw_ops_t;
 
 /* =========================================================================
  * HAL public API
  * ====================================================================== */
 
 int      uiox_emmc_hw_init         (uiox_emmc_hw_t *hw,
                                      const uiox_emmc_hw_ops_t *ops);
 void     uiox_emmc_hw_deinit       (uiox_emmc_hw_t *hw);
 int      uiox_emmc_hw_power_on     (uiox_emmc_hw_t *hw);
 void     uiox_emmc_hw_power_off    (uiox_emmc_hw_t *hw);
 uint32_t uiox_emmc_hw_reg_read     (uiox_emmc_hw_t *hw, uint32_t offset);
 void     uiox_emmc_hw_reg_write    (uiox_emmc_hw_t *hw,
                                      uint32_t offset, uint32_t val);
 int      uiox_emmc_hw_set_clock    (uiox_emmc_hw_t *hw, uint32_t hz);
 int      uiox_emmc_hw_set_bus_width(uiox_emmc_hw_t *hw, uint8_t width);
 int      uiox_emmc_hw_set_speed    (uiox_emmc_hw_t *hw,
                                      uiox_emmc_speed_t speed);
 int      uiox_emmc_hw_send_cmd     (uiox_emmc_hw_t *hw, uint8_t cmd,
                                      uint32_t arg,
                                      uiox_emmc_resp_t resp_type,
                                      uint32_t *resp);
 int      uiox_emmc_hw_read_blocks  (uiox_emmc_hw_t *hw, uint32_t lba,
                                      uint8_t *buf, uint32_t count);
 int      uiox_emmc_hw_write_blocks (uiox_emmc_hw_t *hw, uint32_t lba,
                                      const uint8_t *buf, uint32_t count);
 int      uiox_emmc_hw_read_ext_csd (uiox_emmc_hw_t *hw, uint8_t *buf);
 int      uiox_emmc_hw_write_ext_csd(uiox_emmc_hw_t *hw,
                                      uint8_t index, uint8_t val);
 int      uiox_emmc_hw_tuning       (uiox_emmc_hw_t *hw);
 uint8_t  uiox_emmc_hw_crc7         (uiox_emmc_hw_t *hw,
                                      const uint8_t *d, uint8_t len);
 uint16_t uiox_emmc_hw_crc16        (uiox_emmc_hw_t *hw,
                                      const uint8_t *d, uint32_t len);
 
 static inline uint32_t uiox_emmc_caps(const uiox_emmc_hw_t *hw)
 { return hw ? hw->caps : 0u; }
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_EMMC_HW_H */
 