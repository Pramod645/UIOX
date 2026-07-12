/* ─────────────────────────────────────────────────────────────────────────
 * uiox_fw_sd.h — SD/SDHC/SDXC SDIO host HAL
 * ───────────────────────────────────────────────────────────────────────── */
#ifndef UIOX_FW_SD_H
#define UIOX_FW_SD_H
#include "uiox_fw_types.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef enum { UIOX_SD_CARD_NONE=0, UIOX_SD_SDSC=1,
               UIOX_SD_SDHC=2,      UIOX_SD_SDXC=3 } uiox_sd_card_t;
typedef enum { UIOX_SD_BUS_SDIO_4BIT=0, UIOX_SD_BUS_SPI=1 } uiox_sd_bus_t;

#define SDIO_HC_BASE_ARM64  0xFE340000u
#define SDIO_PS_CARD_INS    (1u << 16)
#define SDIO_PS_WP          (1u << 19)
#define SD_BLOCK_SIZE       512u
#define SD_CMD0_IDLE        0u
#define SD_CMD8_IF_COND     8u
#define SD_CMD17_READ       17u
#define SD_CMD24_WRITE      24u
#define SD_ACMD41_OP_COND   41u
#define SD_ACMD41_HCS       (1u << 30)
#define SD_CMD55_APP        55u

typedef struct {
    uintptr_t      base;
    uint32_t       irq;
    uiox_sd_bus_t  bus;
    uiox_sd_card_t card_type;
    uint16_t       rca;
    uint64_t       capacity_sectors;
    uint64_t       capacity_bytes;
    bool           card_present;
    bool           write_protect;
    bool           initialized;
    void          *priv;
} uiox_sd_dev_t;

typedef struct {
    uiox_fw_err_t (*init)       (uiox_sd_dev_t *dev);
    void          (*deinit)     (uiox_sd_dev_t *dev);
    bool          (*card_detect)(uiox_sd_dev_t *dev);
    uiox_fw_err_t (*card_init)  (uiox_sd_dev_t *dev);
    uiox_fw_err_t (*read)       (uiox_sd_dev_t *dev, uint32_t lba,
                                  uint8_t *buf, uint32_t blocks);
    uiox_fw_err_t (*write)      (uiox_sd_dev_t *dev, uint32_t lba,
                                  const uint8_t *buf, uint32_t blocks);
    void          (*isr)        (uiox_sd_dev_t *dev);
} uiox_sd_ops_t;

uiox_fw_err_t uiox_fw_sd_init        (uiox_sd_dev_t *dev, const uiox_sd_ops_t *ops);
void          uiox_fw_sd_deinit      (uiox_sd_dev_t *dev);
bool          uiox_fw_sd_card_present(uiox_sd_dev_t *dev);
uiox_fw_err_t uiox_fw_sd_card_init   (uiox_sd_dev_t *dev);
uiox_fw_err_t uiox_fw_sd_read        (uiox_sd_dev_t *dev, uint32_t lba,
                                        uint8_t *buf, uint32_t blocks);
uiox_fw_err_t uiox_fw_sd_write       (uiox_sd_dev_t *dev, uint32_t lba,
                                        const uint8_t *buf, uint32_t blocks);
uiox_fw_err_t uiox_fw_sd_init_sdhost (uiox_sd_dev_t *dev,
                                        uintptr_t base, uint32_t irq);
#ifdef __cplusplus
}
#endif
#endif /* UIOX_FW_SD_H */
