/**
 * @file  uiox_fw_storage.h
 * @brief UIOX Firmware — Block storage HAL.
 *        Matches 30_DeviceDrivers/01_block / BlockDrivers.h in UIOX.
 * @version 1.0.0
 * @date    2026-06-21
 */

 #ifndef UIOX_FW_STORAGE_H
 #define UIOX_FW_STORAGE_H
 
 #include "uiox_fw_hw.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 #define UIOX_FW_STOR_SECTOR_SIZE    512u
 #define UIOX_FW_STOR_MAX_DEVS       8u
 #define UIOX_FW_STOR_NAME_LEN       16u
 
 typedef enum {
     UIOX_FW_STOR_VIRTIO_BLK = 0,
     UIOX_FW_STOR_SD,
     UIOX_FW_STOR_EMMC,
     UIOX_FW_STOR_NVME,
     UIOX_FW_STOR_IDE,
     UIOX_FW_STOR_RAMDISK,
 } uiox_fw_stor_type_t;
 
 typedef struct {
     uiox_fw_stor_type_t type;
     char                name[UIOX_FW_STOR_NAME_LEN];
     uintptr_t           base;        /**< MMIO or I/O base               */
     uint32_t            irq;
     uint64_t            num_sectors;
     uint32_t            sector_size;
     bool                read_only;
     bool                present;
     void               *priv;
     /* Ops */
     uiox_fw_err_t (*read) (void *priv, uint64_t lba,
                             uint8_t *buf, uint32_t count);
     uiox_fw_err_t (*write)(void *priv, uint64_t lba,
                             const uint8_t *buf, uint32_t count);
     uiox_fw_err_t (*flush)(void *priv);
     /* Stats */
     uint64_t            reads;
     uint64_t            writes;
     uint32_t            errors;
 } uiox_fw_stor_dev_t;
 
 /* API */
 uiox_fw_err_t uiox_fw_stor_init        (void);
 uiox_fw_err_t uiox_fw_stor_register    (uiox_fw_stor_dev_t *dev);
 uiox_fw_stor_dev_t *uiox_fw_stor_get   (uint32_t idx);
 uint32_t      uiox_fw_stor_count       (void);
 uiox_fw_err_t uiox_fw_stor_read        (uiox_fw_stor_dev_t *dev,
                                           uint64_t lba,
                                           uint8_t *buf, uint32_t count);
 uiox_fw_err_t uiox_fw_stor_write       (uiox_fw_stor_dev_t *dev,
                                           uint64_t lba,
                                           const uint8_t *buf, uint32_t count);
 uiox_fw_err_t uiox_fw_stor_flush       (uiox_fw_stor_dev_t *dev);
 void          uiox_fw_stor_print       (void);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_FW_STORAGE_H */
 