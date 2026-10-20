/**
 * @file    uiox_sdmmc_blk.c
 * @brief   The BLOCK-family vtable for SD — the seam where the
 *          filesystem's dispatch meets the driver.
 *
 * Five functions, each a cast and a forward, exactly as
 * uiox_emmc_blk.c is for eMMC.  Nothing above this file can name
 * uiox_sdmmc_if_t.
 *
 * num_blocks returns hw->ident.capacity_sectors, which card_init()
 * filled — 0 means "card not seated or not yet initialised", and bcache
 * then skips its bound check rather than treating every block as past
 * the end.  That is the honest answer for removable media.
 */
#include "uiox_sdmmc_if.h"
#include "uiox_sdmmc_hw.h"
#include "uiox_devclass.h"
#include "uiox_devops.h"
#include "uiox_sdmmc_ioctl.h"
#include "uiox_klibc.h"

static uiox_sdmmc_if_t *sif_of(uiox_dev_dev_t *dev)
{
    if (!dev || !dev->drv_priv) return NULL;
    return (uiox_sdmmc_if_t *)dev->drv_priv;
}

static int sdmmc_blk_read(uiox_dev_dev_t *dev, uint32_t blkno,
                          uint32_t count, uint8_t *buf)
{
    uiox_sdmmc_if_t *sif = sif_of(dev);
    if (!sif || !sif->primed || !sif->hw->card_present) return -ENODEV;
    return uiox_sdmmc_if_read(sif, blkno, buf, count);
}

static int sdmmc_blk_write(uiox_dev_dev_t *dev, uint32_t blkno,
                           uint32_t count, const uint8_t *buf)
{
    uiox_sdmmc_if_t *sif = sif_of(dev);
    if (!sif || !sif->primed || !sif->hw->card_present) return -ENODEV;
    /* A write-protected card rejects writes at the driver, not silently
     * at the medium. */
    if (sif->hw->write_protect) return -EROFS;
    return uiox_sdmmc_if_write(sif, blkno, buf, count);
}

/* SD has no cache-flush command in the driver yet, and nothing above it
 * holds write-back state: writes go to the card synchronously.  0 says
 * "nothing to lose", not "flushed". */
static int sdmmc_blk_flush(uiox_dev_dev_t *dev)
{
    uiox_sdmmc_if_t *sif = sif_of(dev);
    if (!sif || !sif->primed) return -ENODEV;
    return 0;
}

static uint32_t sdmmc_blk_num_blocks(uiox_dev_dev_t *dev)
{
    uiox_sdmmc_if_t *sif = sif_of(dev);
    if (!sif || !sif->hw) return 0u;          /* unknown, not zero-sized */
    return (uint32_t)sif->hw->ident.capacity_sectors;
}

static int sdmmc_blk_ioctl(uiox_dev_dev_t *dev, unsigned long cmd, void *arg)
{
    uiox_sdmmc_if_t *sif = sif_of(dev);
    if (!sif || !sif->primed) return -ENODEV;

    switch ((uint32_t)cmd) {

    case UIOX_SDMMC_IOC_GET_IDENT:
        if (!arg) return -EINVAL;
        *(uiox_sdmmc_ident_t *)arg = sif->hw->ident;
        return 0;

    case UIOX_SDMMC_IOC_GET_BYTES:
        if (!arg) return -EINVAL;
        *(uint64_t *)arg = sif->hw->ident.capacity_bytes;
        return 0;

    case UIOX_SDMMC_IOC_GET_BLOCKS:
        if (!arg) return -EINVAL;
        *(uint32_t *)arg = (uint32_t)sif->hw->ident.capacity_sectors;
        return 0;

    case UIOX_SDMMC_IOC_GET_CARD:
        if (!arg) return -EINVAL;
        *(uint32_t *)arg = (uint32_t)sif->hw->ident.card;
        return 0;

    case UIOX_SDMMC_IOC_GET_STATS:
        if (!arg) return -EINVAL;
        uiox_sdmmc_if_stats_get(sif, (uiox_sdmmc_if_stats_t *)arg);
        return 0;

    case UIOX_SDMMC_IOC_RESCAN:
        return uiox_sdmmc_if_detect(sif);

    case UIOX_SDMMC_IOC_REINIT:
        return uiox_sdmmc_if_card_init(sif);

    case UIOX_SDMMC_IOC_SET_WIDTH:
        if (!arg) return -EINVAL;
        return uiox_sdmmc_if_set_bus_width(sif, *(uint8_t *)arg);

    case UIOX_SDMMC_IOC_SET_CLOCK:
        if (!arg) return -EINVAL;
        return uiox_sdmmc_if_set_clock(sif, *(uint32_t *)arg);

    default:
        return -ENOSYS;
    }
}

const uiox_blk_ops_t s_sdmmc_blk_ops = {
    .read       = sdmmc_blk_read,
    .write      = sdmmc_blk_write,
    .flush      = sdmmc_blk_flush,
    .num_blocks = sdmmc_blk_num_blocks,
    .ioctl      = sdmmc_blk_ioctl,
};
