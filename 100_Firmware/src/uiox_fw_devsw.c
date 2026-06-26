/**
 * @file  uiox_fw_devsw.c
 * @brief UIOX Firmware — Device switch table (char + block).
 * @date  2026-06-21
 */

 #include "uiox_fw.h"

 uiox_fw_err_t uiox_fw_devsw_init(uiox_fw_devsw_t *dsw)
 {
     if (!dsw) return UIOX_FW_ERR_INVAL;
     uiox_fw_memset(dsw, 0, sizeof(*dsw));
     dsw->magic      = UIOX_FW_DEVSW_MAGIC;
     dsw->cdev_count = 0u;
     dsw->bdev_count = 0u;
     return UIOX_FW_OK;
 }
 
 uiox_fw_err_t uiox_fw_cdev_register(uiox_fw_devsw_t *dsw,
                                       const uiox_fw_cdevsw_t *entry)
 {
     if (!dsw || !entry) return UIOX_FW_ERR_INVAL;
     if (dsw->cdev_count >= UIOX_FW_DEVSW_MAX_CHAR) return UIOX_FW_ERR_OVERFLOW;
     dsw->cdev[dsw->cdev_count++] = *entry;
     FW_LOG("DEVSW", "cdev[%u] %s major=%u",
            dsw->cdev_count - 1u, entry->name, entry->major);
     return UIOX_FW_OK;
 }
 
 uiox_fw_err_t uiox_fw_bdev_register(uiox_fw_devsw_t *dsw,
                                       const uiox_fw_bdevsw_t *entry)
 {
     if (!dsw || !entry) return UIOX_FW_ERR_INVAL;
     if (dsw->bdev_count >= UIOX_FW_DEVSW_MAX_BLOCK) return UIOX_FW_ERR_OVERFLOW;
     dsw->bdev[dsw->bdev_count++] = *entry;
     FW_LOG("DEVSW", "bdev[%u] %s major=%u blocks=%llu",
            dsw->bdev_count - 1u, entry->name, entry->major,
            (unsigned long long)entry->num_blocks);
     return UIOX_FW_OK;
 }
 
 uiox_fw_cdevsw_t *uiox_fw_cdev_get(uiox_fw_devsw_t *dsw, uint32_t major)
 {
     if (!dsw) return NULL;
     for (uint32_t i = 0u; i < dsw->cdev_count; i++)
         if (dsw->cdev[i].major == major) return &dsw->cdev[i];
     return NULL;
 }
 
 uiox_fw_bdevsw_t *uiox_fw_bdev_get(uiox_fw_devsw_t *dsw, uint32_t major)
 {
     if (!dsw) return NULL;
     for (uint32_t i = 0u; i < dsw->bdev_count; i++)
         if (dsw->bdev[i].major == major) return &dsw->bdev[i];
     return NULL;
 }
 
 void uiox_fw_devsw_print(const uiox_fw_devsw_t *dsw)
 {
     if (!dsw) return;
     uiox_fw_printf("[FW] Device switch table:\n");
     uiox_fw_printf("  Char devices (%u):\n", dsw->cdev_count);
     for (uint32_t i = 0u; i < dsw->cdev_count; i++)
         uiox_fw_printf("    [%2u] major=%-3u  %s  %s\n",
                         i, dsw->cdev[i].major, dsw->cdev[i].name,
                         dsw->cdev[i].ready ? "ready" : "not ready");
     uiox_fw_printf("  Block devices (%u):\n", dsw->bdev_count);
     for (uint32_t i = 0u; i < dsw->bdev_count; i++)
         uiox_fw_printf("    [%2u] major=%-3u  %s  blocks=%llu  %s\n",
                         i, dsw->bdev[i].major, dsw->bdev[i].name,
                         (unsigned long long)dsw->bdev[i].num_blocks,
                         dsw->bdev[i].ready ? "ready" : "not ready");
 }
 