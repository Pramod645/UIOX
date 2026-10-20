/**
 * @file    uiox_nvme_blk.c
 * @brief   The BLOCK-family vtable for NVMe — the seam where the
 *          filesystem's dispatch meets the driver.
 *
 * Five functions, each a cast and a forward, exactly as
 * uiox_emmc_blk.c is for eMMC.  Nothing above this file can name
 * uiox_nvme_if_t: bcache holds a uint8_t dev, the filesystem holds a
 * (cls, unit) pair, the registry returns a uiox_dev_dev_t.  This file
 * unwraps dev->drv_priv back to the typed pointer.
 *
 * bcache counts in BCACHE_SECTOR_SIZE units; NVMe's LBA is 512-byte, so
 * the values pass through — the translation is named here, not assumed.
 */
#include "uiox_nvme_if.h"
#include "uiox_nvme_hw.h"
#include "uiox_devclass.h"
#include "uiox_devops.h"
#include "uiox_nvme_ioctl.h"
#include "uiox_klibc.h"

/* dev->ops is written once at bind time and drv_priv only by the driver,
 * so this cast is safe: nothing else puts a different pointer there.
 * NULL rather than a dereference, so every entry point tests once. */
static uiox_nvme_if_t *nif_of(uiox_dev_dev_t *dev)
{
    if (!dev || !dev->drv_priv) return NULL;
    return (uiox_nvme_if_t *)dev->drv_priv;
}

static int nvme_blk_read(uiox_dev_dev_t *dev, uint32_t blkno,
                         uint32_t count, uint8_t *buf)
{
    uiox_nvme_if_t *nif = nif_of(dev);
    if (!nif || !nif->primed) return -ENODEV;
    return uiox_nvme_if_read(nif, (uint64_t)blkno, buf, count);
}

static int nvme_blk_write(uiox_dev_dev_t *dev, uint32_t blkno,
                          uint32_t count, const uint8_t *buf)
{
    uiox_nvme_if_t *nif = nif_of(dev);
    if (!nif || !nif->primed) return -ENODEV;
    return uiox_nvme_if_write(nif, (uint64_t)blkno, buf, count);
}

/* A device with no volatile write cache has no write-back state to lose,
 * so 0 is correct rather than a silent lie.  uiox_nvme_hw_flush()
 * returns 0 when the caps have no VWC. */
static int nvme_blk_flush(uiox_dev_dev_t *dev)
{
    uiox_nvme_if_t *nif = nif_of(dev);
    if (!nif || !nif->primed) return -ENODEV;
    return uiox_nvme_if_flush(nif);
}

/* capacity_lba is in 512-byte units — the same unit BCACHE_SECTOR_SIZE
 * counts — so no arithmetic is needed.  0 means UNKNOWN: bcache then
 * skips its bound check rather than treating every block as past the
 * end.  The uint32_t return truncates above ~2 TiB; a caller needing the
 * full value uses UIOX_NVME_IOC_GET_BYTES. */
static uint32_t nvme_blk_num_blocks(uiox_dev_dev_t *dev)
{
    uiox_nvme_if_t *nif = nif_of(dev);
    if (!nif || !nif->hw) return 0u;
    return (uint32_t)nif->hw->ident.capacity_lba;
}

/* Reached from uiox_kix_scfs_ioctl.c after uiox_dev_lookup(cls, unit).
 * -ENOSYS for an unknown command is the tree's "not implemented", not
 * "invalid" — a caller that gets it asked for something real this class
 * does not do yet. */
static int nvme_blk_ioctl(uiox_dev_dev_t *dev, unsigned long cmd, void *arg)
{
    uiox_nvme_if_t *nif = nif_of(dev);
    if (!nif || !nif->primed) return -ENODEV;

    switch ((uint32_t)cmd) {

    case UIOX_NVME_IOC_GET_IDENT:
        if (!arg) return -EINVAL;
        /* Copy, do not hand out a pointer into hw — the driver owns it. */
        *(uiox_nvme_ident_t *)arg = nif->hw->ident;
        return 0;

    case UIOX_NVME_IOC_GET_BYTES:
        if (!arg) return -EINVAL;
        *(uint64_t *)arg = nif->hw->ident.capacity_bytes;
        return 0;

    case UIOX_NVME_IOC_GET_BLOCKS:
        if (!arg) return -EINVAL;
        *(uint32_t *)arg = (uint32_t)nif->hw->ident.capacity_lba;
        return 0;

    case UIOX_NVME_IOC_GET_NS:
        if (!arg) return -EINVAL;
        *(uint32_t *)arg = nif->hw->ident.nsid;
        return 0;

    case UIOX_NVME_IOC_GET_STATS:
        if (!arg) return -EINVAL;
        uiox_nvme_if_stats_get(nif, (uiox_nvme_if_stats_t *)arg);
        return 0;

    case UIOX_NVME_IOC_FLUSH:
        return uiox_nvme_if_flush(nif);

    case UIOX_NVME_IOC_REFRESH:
        return uiox_nvme_if_identify(nif);

    default:
        return -ENOSYS;
    }
}

/* const because it is written once and never again — the convention
 * uiox_dev_dev_t.ops states.  Not static: the probe needs its address. */
const uiox_blk_ops_t s_nvme_blk_ops = {
    .read       = nvme_blk_read,
    .write      = nvme_blk_write,
    .flush      = nvme_blk_flush,
    .num_blocks = nvme_blk_num_blocks,
    .ioctl      = nvme_blk_ioctl,
};
