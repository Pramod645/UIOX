/*
 * 01_uBoot/src/boot_media/boot_media_fwstor.c
 * Adapter over the 02_FwHal storage registry. One adapter serves every
 * media type — the kind is derived from uiox_fw_stor_dev_t.type.
 */
#include "uiox_boot.h"
#include "uiox_boot_media.h"
#include "uiox_fw_storage.h"

static uiox_fw_stor_dev_t *s_dev;
static uiox_boot_media_kind_t s_kind = UIOX_MEDIA_NONE;

static uiox_boot_media_kind_t kind_of(uiox_fw_stor_type_t t)
{
    switch (t) {
    case UIOX_FW_STOR_VIRTIO_BLK: return UIOX_MEDIA_VIRTIO;
    case UIOX_FW_STOR_SD:         return UIOX_MEDIA_SDMMC;
    case UIOX_FW_STOR_EMMC:       return UIOX_MEDIA_SDMMC;
    case UIOX_FW_STOR_NVME:       return UIOX_MEDIA_NVME;
    case UIOX_FW_STOR_IDE:        return UIOX_MEDIA_AHCI;
    default:                      return UIOX_MEDIA_NONE;
    }
}

static int fwstor_present(void)
{
    for (uint32_t i = 0; i < uiox_fw_stor_count(); i++) {
        uiox_fw_stor_dev_t *d = uiox_fw_stor_get(i);
        if (d && d->present && d->read) {
            s_dev  = d;
            s_kind = kind_of(d->type);
            return 1;
        }
    }
    return 0;
}

static uiox_boot_err_t fwstor_init(void)
{ return s_dev ? UIOX_BOOT_OK : UIOX_BOOT_ERR_NOTFOUND; }

static uiox_boot_err_t fwstor_read(uint32_t blk, uint32_t nblocks, void *buf)
{
    uint64_t lba  = (uint64_t)blk * UIOX_MEDIA_SECTORS_PER_BLOCK;
    uint32_t nsec = nblocks * UIOX_MEDIA_SECTORS_PER_BLOCK;
    return uiox_fw_stor_read(s_dev, lba, (uint8_t *)buf, nsec) == UIOX_FW_OK
           ? UIOX_BOOT_OK : UIOX_BOOT_ERR_IO;
}

static const uiox_boot_media_ops_t fwstor_ops = {
    .name = "fw-stor",
    .kind = UIOX_MEDIA_NONE,      /* set dynamically after select */
    .present = fwstor_present,
    .init = fwstor_init,
    .read_block = fwstor_read,
};

const uiox_boot_media_ops_t *uiox_boot_media_fwstor(void)
{ return &fwstor_ops; }
