/*
 * 02_FwHal/src/uiox_fw_none.c
 * Template storage driver — copy to uiox_fw_<new>.c to add a controller.
 * Reports present=false so dispatch always skips it; safe to register.
 */
#include "uiox_fw_none.h"

static uiox_fw_err_t none_read(void *priv, uint64_t lba,
                               uint8_t *buf, uint32_t count)
{ (void)priv; (void)lba; (void)buf; (void)count; return UIOX_FW_ERR_NODEV; }

static uiox_fw_stor_dev_t s_dev;

uiox_fw_err_t uiox_fw_none_init(void)
{
    s_dev.type        = UIOX_FW_STOR_RAMDISK;
    s_dev.sector_size = UIOX_FW_STOR_SECTOR_SIZE;
    s_dev.num_sectors = 0u;
    s_dev.read_only   = true;
    s_dev.present     = false;
    s_dev.priv        = 0;
    s_dev.read        = none_read;
    s_dev.write       = 0;
    s_dev.flush       = 0;
    { const char *n = "none"; for (uint32_t i=0u; n[i] && i < UIOX_FW_STOR_NAME_LEN-1u; i++) s_dev.name[i]=n[i]; }
    return uiox_fw_stor_register(&s_dev);
}

uiox_fw_stor_dev_t *uiox_fw_none_stor_dev(void) { return &s_dev; }
