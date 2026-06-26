/**
 * @file  uiox_fw_storage.c
 * @brief UIOX Firmware — block storage device registry.
 * @date  2026-06-21
 */

 #include "uiox_fw.h"

 static uiox_fw_stor_dev_t *s_devs[UIOX_FW_STOR_MAX_DEVS];
 static uint32_t             s_count = 0u;
 
 uiox_fw_err_t uiox_fw_stor_init(void)
 {
     for (uint32_t i = 0u; i < UIOX_FW_STOR_MAX_DEVS; i++) s_devs[i] = NULL;
     s_count = 0u;
     FW_LOG("STOR", "block storage registry init OK");
     return UIOX_FW_OK;
 }
 
 uiox_fw_err_t uiox_fw_stor_register(uiox_fw_stor_dev_t *dev)
 {
     if (!dev || s_count >= UIOX_FW_STOR_MAX_DEVS)
         return UIOX_FW_ERR_OVERFLOW;
     dev->reads  = 0u;
     dev->writes = 0u;
     dev->errors = 0u;
     s_devs[s_count++] = dev;
     FW_LOG("STOR", "registered %s  sectors=%llu  ro=%d",
            dev->name,
            (unsigned long long)dev->num_sectors,
            (int)dev->read_only);
     return UIOX_FW_OK;
 }
 
 uiox_fw_stor_dev_t *uiox_fw_stor_get(uint32_t idx)
 { return idx < s_count ? s_devs[idx] : NULL; }
 
 uint32_t uiox_fw_stor_count(void) { return s_count; }
 
 uiox_fw_err_t uiox_fw_stor_read(uiox_fw_stor_dev_t *dev,
                                   uint64_t lba,
                                   uint8_t *buf, uint32_t count)
 {
     if (!dev || !dev->read || !buf || count == 0u)
         return UIOX_FW_ERR_INVAL;
     if (lba + count > dev->num_sectors) return UIOX_FW_ERR_OVERFLOW;
     uiox_fw_err_t rc = dev->read(dev->priv, lba, buf, count);
     if (rc == UIOX_FW_OK) dev->reads += count;
     else                   dev->errors++;
     return rc;
 }
 
 uiox_fw_err_t uiox_fw_stor_write(uiox_fw_stor_dev_t *dev,
                                    uint64_t lba,
                                    const uint8_t *buf, uint32_t count)
 {
     if (!dev || !dev->write || !buf || count == 0u)
         return UIOX_FW_ERR_INVAL;
     if (dev->read_only) return UIOX_FW_ERR_PERM;
     if (lba + count > dev->num_sectors) return UIOX_FW_ERR_OVERFLOW;
     uiox_fw_err_t rc = dev->write(dev->priv, lba, buf, count);
     if (rc == UIOX_FW_OK) dev->writes += count;
     else                   dev->errors++;
     return rc;
 }
 
 uiox_fw_err_t uiox_fw_stor_flush(uiox_fw_stor_dev_t *dev)
 {
     if (!dev) return UIOX_FW_ERR_INVAL;
     if (dev->flush) return dev->flush(dev->priv);
     return UIOX_FW_OK;
 }
 
 void uiox_fw_stor_print(void)
 {
     static const char *type_names[] = {
         "virtio-blk", "SD", "eMMC", "NVMe", "IDE", "ramdisk"
     };
     uiox_fw_printf("[FW] Block devices (%u):\n", s_count);
     for (uint32_t i = 0u; i < s_count; i++) {
         const uiox_fw_stor_dev_t *d = s_devs[i];
         if (!d) continue;
         uint8_t t = (uint8_t)d->type;
         uiox_fw_printf("  [%u] %-12s  type=%-10s  sectors=%llu  "
                         "ro=%d  reads=%llu  writes=%llu\n",
                         i, d->name,
                         t < 6u ? type_names[t] : "?",
                         (unsigned long long)d->num_sectors,
                         (int)d->read_only,
                         (unsigned long long)d->reads,
                         (unsigned long long)d->writes);
     }
 }
 