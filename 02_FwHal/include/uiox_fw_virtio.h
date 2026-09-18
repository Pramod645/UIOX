/*
 * 02_FwHal/include/uiox_fw_virtio.h
 * VirtIO MMIO block device — firmware HAL driver.
 * Conforms to the uiox_fw_storage.h registration model.
 */
#ifndef UIOX_FW_VIRTIO_H
#define UIOX_FW_VIRTIO_H

#include "uiox_fw_types.h"
#include "uiox_fw_storage.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uintptr_t base;
    uint32_t  irq;
    uint64_t  capacity_sectors;
    bool      present;
    bool      initialized;
    void     *priv;
} uiox_virtio_dev_t;

typedef struct {
    uiox_fw_err_t (*init)  (uiox_virtio_dev_t *dev);
    void          (*deinit)(uiox_virtio_dev_t *dev);
    uiox_fw_err_t (*probe) (uiox_virtio_dev_t *dev);
    uiox_fw_err_t (*read)  (uiox_virtio_dev_t *dev, uint32_t lba,
                            uint8_t *buf, uint32_t blocks);
    uiox_fw_err_t (*write) (uiox_virtio_dev_t *dev, uint32_t lba,
                            const uint8_t *buf, uint32_t blocks);
} uiox_virtio_ops_t;

/* Register a probed VirtIO device into the storage HAL. */
uiox_fw_err_t uiox_fw_virtio_init(void);
uiox_fw_stor_dev_t *uiox_fw_virtio_stor_dev(void);

#ifdef __cplusplus
}
#endif
#endif /* UIOX_FW_VIRTIO_H */
