/**
 * @file    uiox_sata_blk.c
 * @brief   The BLOCK-family vtable for SATA — the seam where the
 *          filesystem's dispatch meets the driver.
 *
 * Five functions, each a cast and a forward, exactly as
 * uiox_nvme_blk.c and uiox_emmc_blk.c are for their classes.  Nothing
 * above this file can name uiox_sata_if_t: bcache holds a uint8_t dev,
 * the filesystem holds a (cls, unit) pair, the registry returns a
 * uiox_dev_dev_t.  This file unwraps dev->drv_priv back to the typed
 * pointer.
 *
 * bcache counts in BCACHE_SECTOR_SIZE units; SATA's LBA is 512-byte, so
 * the values pass through — the translation is named here, not assumed.
 */
#include "uiox_sata_if.h"
#include "uiox_sata_hw.h"
#include "uiox_devclass.h"
#include "uiox_devops.h"
#include "uiox_sata_ioctl.h"
#include "uiox_klibc.h"

/* dev->ops is written once at bind time and drv_priv only by the driver,
 * so this cast is safe: nothing else puts a different pointer there.
 * NULL rather than a dereference, so every entry point tests once. */
static uiox_sata_if_t *sif_of(uiox_dev_dev_t *dev)
{
    if (!dev || !dev->drv_priv) return NULL;
    return (uiox_sata_if_t *)dev->drv_priv;
}

static int sata_blk_read(uiox_dev_dev_t *dev, uint32_t blkno,
                         uint32_t count, uint8_t *buf)
{
    uiox_sata_if_t *sif = sif_of(dev);
    if (!sif || !sif->primed) return -ENODEV;
    return uiox_sata_if_read(sif, (uint64_t)blkno, buf, count);
}

static int sata_blk_write(uiox_dev_dev_t *dev, uint32_t blkno,
                          uint32_t count, const uint8_t *buf)
{
    uiox_sata_if_t *sif = sif_of(dev);
    if (!sif || !sif->primed) return -ENODEV;
    return uiox_sata_if_write(sif, (uint64_t)blkno, buf, count);
}

/* uiox_sata_if.h declares NO flush entry point — the 50_UIX original had
 * uiox_sata_proto_flush(), which printed "FLUSH CACHE EXT" and returned
 * UIOX_FW_OK without issuing the command.  Neither was carried over, and
 * 02_FwHal's ahci_flush() is `{ (void)d; return UIOX_FW_OK; }`.
 *
 * So there is no path that flushes the drive's volatile write cache.
 * Returning 0 would tell the caller its data survived a power loss when
 * nothing was sent to the device — the same false success that
 * uiox_sdmmc_fwops.c refuses for sdh_read/sdh_write.  -ENOSYS is the
 * tree's "not implemented" and the honest answer here. */
static int sata_blk_flush(uiox_dev_dev_t *dev)
{
    uiox_sata_if_t *sif = sif_of(dev);
    if (!sif || !sif->primed) return -ENODEV;
    return -ENOSYS;
}

/* ident.lba48_sectors is in 512-byte units — the same unit
 * BCACHE_SECTOR_SIZE counts — so no arithmetic is needed.  0 means
 * UNKNOWN: bcache then skips its bound check rather than treating every
 * block as past the end.  The uint32_t return truncates above ~2 TiB; a
 * caller needing the full value uses UIOX_SATA_IOC_GET_BYTES. */
static uint32_t sata_blk_num_blocks(uiox_dev_dev_t *dev)
{
    uiox_sata_if_t *sif = sif_of(dev);
    if (!sif || !sif->hw) return 0u;
    return (uint32_t)sif->hw->ident.lba48_sectors;
}

/* Reached from uiox_kix_scfs_ioctl.c after uiox_dev_lookup(cls, unit).
 * -ENOSYS for an unknown command is the tree's "not implemented", not
 * "invalid" — a caller that gets it asked for something real this class
 * does not do yet. */
static int sata_blk_ioctl(uiox_dev_dev_t *dev, unsigned long cmd, void *arg)
{
    uiox_sata_if_t *sif = sif_of(dev);
    if (!sif || !sif->primed) return -ENODEV;

    switch ((uint32_t)cmd) {

    case UIOX_SATA_IOC_GET_IDENT:
        if (!arg) return -EINVAL;
        *((uiox_sata_ident_t *)arg) = sif->hw->ident;
        return 0;

    case UIOX_SATA_IOC_GET_BYTES:
        if (!arg) return -EINVAL;
        *((uint64_t *)arg) = sif->hw->ident.capacity_bytes;
        return 0;

    case UIOX_SATA_IOC_GET_BLOCKS:
        if (!arg) return -EINVAL;
        *((uint32_t *)arg) = (uint32_t)sif->hw->ident.lba48_sectors;
        return 0;

    case UIOX_SATA_IOC_GET_MEDIA:
        if (!arg) return -EINVAL;
        *((uint32_t *)arg) = sif->hw->ident.is_ssd ? 1u : 0u;
        return 0;

    case UIOX_SATA_IOC_GET_STATS:
        if (!arg) return -EINVAL;
        uiox_sata_if_stats_get(sif, (uiox_sata_if_stats_t *)arg);
        return 0;

    case UIOX_SATA_IOC_FLUSH:
        /* No flush path exists — see sata_blk_flush() above. */
        return -ENOSYS;

    case UIOX_SATA_IOC_REFRESH:
        return uiox_sata_if_start(sif);

    default:
        return -ENOSYS;
    }
}

/* Non-static: uiox_sata_probe.c takes its address for dev->ops. */
const uiox_blk_ops_t s_sata_blk_ops = {
    .read       = sata_blk_read,
    .write      = sata_blk_write,
    .flush      = sata_blk_flush,
    .num_blocks = sata_blk_num_blocks,
    .ioctl      = sata_blk_ioctl,
};
