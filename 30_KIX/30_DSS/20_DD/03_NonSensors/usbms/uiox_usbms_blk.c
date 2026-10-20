/**
 * @file    uiox_usbms_blk.c
 * @brief   The BLOCK-family vtable for USB mass storage — the seam
 *          where the filesystem's dispatch meets the driver.
 *
 * Five functions, each a cast and a forward, exactly as
 * uiox_emmc_blk.c is for eMMC.  Nothing above this file can name
 * uiox_usbms_if_t.
 *
 * num_blocks: 0 means UNKNOWN — a device whose geometry was never read
 * (an empty reader, or a stick pulled out) reports 0 and bcache skips
 * its bound check rather than treating every block as past the end.
 * The value is last_lba + 1 because READ CAPACITY(10) returns the last
 * addressable sector, not the count.
 */
#include "uiox_usbms_if.h"
#include "uiox_devclass.h"
#include "uiox_devops.h"
#include "uiox_klibc.h"

static uiox_usbms_if_t *bif_of(uiox_dev_dev_t *dev)
{
    if (!dev || !dev->drv_priv) return NULL;
    return (uiox_usbms_if_t *)dev->drv_priv;
}

static int usbms_blk_read(uiox_dev_dev_t *dev, uint32_t blkno,
                          uint32_t count, uint8_t *buf)
{
    uiox_usbms_if_t *bif = bif_of(dev);
    if (!bif || !bif->primed) return -ENODEV;
    return uiox_usbms_if_read(bif, blkno, buf, count);
}

static int usbms_blk_write(uiox_dev_dev_t *dev, uint32_t blkno,
                           uint32_t count, const uint8_t *buf)
{
    uiox_usbms_if_t *bif = bif_of(dev);
    if (!bif || !bif->primed) return -ENODEV;
    if (bif->scsi.geom.write_protected) return -EROFS;
    return uiox_usbms_if_write(bif, blkno, buf, count);
}

/* BOT has no write-back state above the transport: WRITE(10) completes
 * or fails at the CSW.  0 says "nothing to lose". */
static int usbms_blk_flush(uiox_dev_dev_t *dev)
{
    uiox_usbms_if_t *bif = bif_of(dev);
    if (!bif || !bif->primed) return -ENODEV;
    return 0;
}

static uint32_t usbms_blk_num_blocks(uiox_dev_dev_t *dev)
{
    uiox_usbms_if_t *bif = bif_of(dev);
    if (!bif || !bif->scsi.geom.ready) return 0u;   /* unknown */
    return bif->scsi.geom.last_lba + 1u;            /* CAP returns last */
}

static int usbms_blk_ioctl(uiox_dev_dev_t *dev, unsigned long cmd, void *arg)
{
    uiox_usbms_if_t *bif = bif_of(dev);
    if (!bif || !bif->primed) return -ENODEV;

    switch ((uint32_t)cmd) {
    case UIOX_USBMS_IOC_GET_BYTES:
        if (!arg) return -EINVAL;
        *(uint64_t *)arg = (uint64_t)(bif->scsi.geom.last_lba + 1u)
                         * bif->scsi.geom.block_size;
        return 0;
    case UIOX_USBMS_IOC_GET_BLOCKS:
        if (!arg) return -EINVAL;
        *(uint32_t *)arg = usbms_blk_num_blocks(dev);
        return 0;
    case UIOX_USBMS_IOC_GET_SENSE:
        if (!arg) return -EINVAL;
        *(uiox_usbms_sense_t *)arg = bif->scsi.sense;
        return 0;
    case UIOX_USBMS_IOC_GET_STATS:
        if (!arg) return -EINVAL;
        uiox_usbms_if_stats_get(bif, (uiox_usbms_if_stats_t *)arg);
        return 0;
    default:
        return -ENOSYS;
    }
}

const uiox_blk_ops_t s_usbms_blk_ops = {
    .read       = usbms_blk_read,
    .write      = usbms_blk_write,
    .flush      = usbms_blk_flush,
    .num_blocks = usbms_blk_num_blocks,
    .ioctl      = usbms_blk_ioctl,
};
