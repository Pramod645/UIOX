/**
 * @file  uiox_fw_emmc.h
 * @brief UIOX FwHal — eMMC 5.1 SDIO host controller (HS400 / HS200).
 */

 #ifndef UIOX_FW_EMMC_H
 #define UIOX_FW_EMMC_H
 
 #include "uiox_fw_types.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* ── eMMC speed modes ────────────────────────────────────────── */
 typedef enum {
     UIOX_EMMC_SPEED_IDENT  = 0,   /**< 400 kHz ID                    */
     UIOX_EMMC_SPEED_DS     = 1,   /**< 25 MHz default                */
     UIOX_EMMC_SPEED_HS52   = 2,   /**< 52 MHz high-speed             */
     UIOX_EMMC_SPEED_HS200  = 3,   /**< 200 MHz SDR                   */
     UIOX_EMMC_SPEED_HS400  = 4,   /**< 200 MHz DDR                   */
 } uiox_emmc_speed_t;
 
 typedef enum {
     UIOX_EMMC_PART_USER  = 0,
     UIOX_EMMC_PART_BOOT1 = 1,
     UIOX_EMMC_PART_BOOT2 = 2,
     UIOX_EMMC_PART_RPMB  = 3,
 } uiox_emmc_part_t;
 
 /* ── SDIO Host Controller (SD Host Spec v3 subset) ───────────── */
 #define EMMC_HC_BASE_ARM64      0xFE340000u  /**< QEMU virt            */
 #define EMMC_HC_BLK_SIZE        0x0004u
 #define EMMC_HC_BLK_COUNT       0x0006u
 #define EMMC_HC_ARG             0x0008u
 #define EMMC_HC_TRANS_MODE      0x000Cu
 #define EMMC_HC_CMD             0x000Eu
 #define EMMC_HC_RESP0           0x0010u
 #define EMMC_HC_PRESENT_STATE   0x0024u
 #define EMMC_HC_CLK_CTRL        0x002Cu
 #define EMMC_HC_INT_STATUS      0x0030u
 #define EMMC_HC_INT_EN          0x0034u
 #define EMMC_PS_CMD_INHIBIT     (1u << 0)
 #define EMMC_INT_CMD_COMPLETE   (1u << 0)
 #define EMMC_INT_XFER_COMPLETE  (1u << 1)
 #define EMMC_INT_ERROR          (1u << 15)
 #define EMMC_CLK_INT_EN         (1u << 0)
 #define EMMC_CLK_STABLE         (1u << 1)
 #define EMMC_CLK_SD_EN          (1u << 2)
 
 /* ── eMMC commands ───────────────────────────────────────────── */
 #define MMC_CMD0_IDLE           0u
 #define MMC_CMD1_SEND_OP_COND   1u
 #define MMC_CMD2_ALL_CID        2u
 #define MMC_CMD3_SET_RCA        3u
 #define MMC_CMD6_SWITCH         6u
 #define MMC_CMD7_SELECT         7u
 #define MMC_CMD8_SEND_EXT_CSD   8u
 #define MMC_CMD17_READ          17u
 #define MMC_CMD18_READ_MULTI    18u
 #define MMC_CMD24_WRITE         24u
 #define MMC_CMD25_WRITE_MULTI   25u
 
 #define EMMC_BLOCK_SIZE         512u
 #define EMMC_EXT_CSD_LEN        512u
 
 typedef struct {
     uintptr_t        base;
     uiox_emmc_speed_t speed;
     uint8_t          bus_width;   /**< 1, 4, or 8                    */
     uint16_t         rca;
     uint64_t         capacity_sectors;
     uint64_t         capacity_bytes;
     uint8_t          ext_csd[EMMC_EXT_CSD_LEN];
     uiox_emmc_part_t active_part;
     bool             cache_enabled;
     bool             initialized;
     void            *priv;
 } uiox_emmc_dev_t;
 
 typedef struct {
     uiox_fw_err_t (*init)         (uiox_emmc_dev_t *dev);
     void          (*deinit)       (uiox_emmc_dev_t *dev);
     uiox_fw_err_t (*read)         (uiox_emmc_dev_t *dev,
                                     uiox_emmc_part_t part,
                                     uint32_t lba,
                                     uint8_t *buf, uint32_t sectors);
     uiox_fw_err_t (*write)        (uiox_emmc_dev_t *dev,
                                     uiox_emmc_part_t part,
                                     uint32_t lba,
                                     const uint8_t *buf, uint32_t sectors);
     uiox_fw_err_t (*flush)        (uiox_emmc_dev_t *dev);
     uiox_fw_err_t (*switch_speed) (uiox_emmc_dev_t *dev,
                                     uiox_emmc_speed_t speed);
     uiox_fw_err_t (*select_part)  (uiox_emmc_dev_t *dev,
                                     uiox_emmc_part_t part);
 } uiox_emmc_ops_t;
 
 uiox_fw_err_t uiox_fw_emmc_init         (uiox_emmc_dev_t *dev,
                                            const uiox_emmc_ops_t *ops);
 void          uiox_fw_emmc_deinit       (uiox_emmc_dev_t *dev);
 uiox_fw_err_t uiox_fw_emmc_read         (uiox_emmc_dev_t *dev,
                                            uiox_emmc_part_t part,
                                            uint32_t lba,
                                            uint8_t *buf, uint32_t sectors);
 uiox_fw_err_t uiox_fw_emmc_write        (uiox_emmc_dev_t *dev,
                                            uiox_emmc_part_t part,
                                            uint32_t lba,
                                            const uint8_t *buf,
                                            uint32_t sectors);
 uiox_fw_err_t uiox_fw_emmc_flush        (uiox_emmc_dev_t *dev);
 uiox_fw_err_t uiox_fw_emmc_init_sdhost  (uiox_emmc_dev_t *dev,
                                            uintptr_t base);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_FW_EMMC_H */
 