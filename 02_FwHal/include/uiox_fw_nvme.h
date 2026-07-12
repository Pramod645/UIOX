/* ─────────────────────────────────────────────────────────────────────────
 * uiox_fw_nvme.h — NVMe M.2 PCIe HAL (BAR0 MMIO, Admin+IO queues)
 * ───────────────────────────────────────────────────────────────────────── */
#ifndef UIOX_FW_NVME_H
#define UIOX_FW_NVME_H
#include "uiox_fw_types.h"
#ifdef __cplusplus
extern "C" {
#endif

#define NVME_REG_CAP        0x0000u
#define NVME_REG_CC         0x0014u
#define NVME_REG_CSTS       0x001Cu
#define NVME_REG_AQA        0x0024u
#define NVME_REG_ASQ_LO     0x0028u
#define NVME_REG_ACQ_LO     0x0030u
#define NVME_CC_EN          (1u << 0)
#define NVME_CSTS_RDY       (1u << 0)
#define NVME_CSTS_CFS       (1u << 1)
#define NVME_ADMIN_IDENTIFY 0x06u
#define NVME_IO_READ        0x02u
#define NVME_IO_WRITE       0x01u
#define NVME_IO_FLUSH       0x00u
#define NVME_IO_DSM         0x09u
#define NVME_BLOCK_SIZE     512u

typedef struct {
    uintptr_t  bar0;
    uint32_t   irq;
    uint64_t   capacity_lba;
    uint64_t   capacity_bytes;
    char       model[41];
    char       serial[21];
    char       fw_rev[9];
    uint32_t   nsid;
    bool       volatile_wc;
    bool       trim_supported;
    bool       initialized;
    void      *priv;
} uiox_nvme_dev_t;

typedef struct {
    uiox_fw_err_t (*init)    (uiox_nvme_dev_t *dev);
    void          (*deinit)  (uiox_nvme_dev_t *dev);
    uiox_fw_err_t (*read)    (uiox_nvme_dev_t *dev, uint64_t lba,
                               uint8_t *buf, uint32_t sectors);
    uiox_fw_err_t (*write)   (uiox_nvme_dev_t *dev, uint64_t lba,
                               const uint8_t *buf, uint32_t sectors);
    uiox_fw_err_t (*flush)   (uiox_nvme_dev_t *dev);
    uiox_fw_err_t (*trim)    (uiox_nvme_dev_t *dev,
                               uint64_t lba, uint32_t sectors);
    uiox_fw_err_t (*identify)(uiox_nvme_dev_t *dev);
} uiox_nvme_ops_t;

uiox_fw_err_t uiox_fw_nvme_init    (uiox_nvme_dev_t *dev, const uiox_nvme_ops_t *ops);
void          uiox_fw_nvme_deinit  (uiox_nvme_dev_t *dev);
uiox_fw_err_t uiox_fw_nvme_read    (uiox_nvme_dev_t *dev, uint64_t lba,
                                      uint8_t *buf, uint32_t sectors);
uiox_fw_err_t uiox_fw_nvme_write   (uiox_nvme_dev_t *dev, uint64_t lba,
                                      const uint8_t *buf, uint32_t sectors);
uiox_fw_err_t uiox_fw_nvme_flush   (uiox_nvme_dev_t *dev);
uiox_fw_err_t uiox_fw_nvme_init_bar0(uiox_nvme_dev_t *dev, uintptr_t bar0,
                                       uint32_t irq);
#ifdef __cplusplus
}
#endif
#endif /* UIOX_FW_NVME_H */
