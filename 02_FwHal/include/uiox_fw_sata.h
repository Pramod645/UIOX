/* ─────────────────────────────────────────────────────────────────────────
 * uiox_fw_sata.h — SATA III AHCI HAL
 * ───────────────────────────────────────────────────────────────────────── */
#ifndef UIOX_FW_SATA_H
#define UIOX_FW_SATA_H
#include "uiox_fw_types.h"
#ifdef __cplusplus
extern "C" {
#endif

#define AHCI_GHC_CAP        0x0000u
#define AHCI_GHC_GHC        0x0004u
#define AHCI_GHC_IS         0x0008u
#define AHCI_GHC_PI         0x000Cu
#define AHCI_GHC_GHC_AE     (1u << 31)
#define AHCI_GHC_GHC_IE     (1u << 1)
#define AHCI_GHC_GHC_HR     (1u << 0)
#define AHCI_PORT_BASE(p)   (0x0100u + (p)*0x80u)
#define AHCI_PX_SSTS        0x28u
#define AHCI_PX_CMD         0x18u
#define AHCI_PX_SIG         0x24u
#define AHCI_PX_IS          0x10u
#define AHCI_PX_CMD_ST      (1u << 0)
#define AHCI_PX_CMD_FRE     (1u << 4)
#define AHCI_SIG_ATA        0x00000101u
#define ATA_CMD_IDENTIFY    0xECu
#define ATA_CMD_READ_DMA    0x25u
#define ATA_CMD_WRITE_DMA   0x35u
#define SATA_SECTOR_SIZE    512u

typedef struct {
    uintptr_t  bar5;
    uint32_t   irq;
    uint8_t    port;
    uint64_t   capacity_sectors;
    uint64_t   capacity_bytes;
    char       model[41];
    char       serial[21];
    bool       ncq_supported;
    bool       smart_supported;
    bool       initialized;
    void      *priv;
} uiox_sata_dev_t;

typedef struct {
    uiox_fw_err_t (*init)    (uiox_sata_dev_t *dev);
    void          (*deinit)  (uiox_sata_dev_t *dev);
    uiox_fw_err_t (*read)    (uiox_sata_dev_t *dev, uint64_t lba,
                               uint8_t *buf, uint32_t sectors);
    uiox_fw_err_t (*write)   (uiox_sata_dev_t *dev, uint64_t lba,
                               const uint8_t *buf, uint32_t sectors);
    uiox_fw_err_t (*flush)   (uiox_sata_dev_t *dev);
    uiox_fw_err_t (*identify)(uiox_sata_dev_t *dev);
    uiox_fw_err_t (*smart)   (uiox_sata_dev_t *dev, uint8_t *buf);
} uiox_sata_ops_t;

uiox_fw_err_t uiox_fw_sata_init     (uiox_sata_dev_t *dev, const uiox_sata_ops_t *ops);
void          uiox_fw_sata_deinit   (uiox_sata_dev_t *dev);
uiox_fw_err_t uiox_fw_sata_read     (uiox_sata_dev_t *dev, uint64_t lba,
                                       uint8_t *buf, uint32_t sectors);
uiox_fw_err_t uiox_fw_sata_write    (uiox_sata_dev_t *dev, uint64_t lba,
                                       const uint8_t *buf, uint32_t sectors);
uiox_fw_err_t uiox_fw_sata_flush    (uiox_sata_dev_t *dev);
uiox_fw_err_t uiox_fw_sata_init_ahci(uiox_sata_dev_t *dev,
                                       uintptr_t bar5, uint8_t port, uint32_t irq);
#ifdef __cplusplus
}
#endif
#endif /* UIOX_FW_SATA_H */
