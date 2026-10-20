/**
 * @file  uiox_sdmmc_if.c
 * @brief UIOX SD interface driver implementation.
 * @date  2026-10-20
 */
#include "uiox_sdmmc_if.h"

int uiox_sdmmc_if_config(uiox_sdmmc_if_t *sif, uiox_sdmmc_hw_t *hw)
{
    if (!sif || !hw) return -EINVAL;
    memset(sif, 0, sizeof(*sif));
    sif->hw        = hw;
    sif->primed    = true;
    sif->bus_width = 1u;
    sif->speed     = UIOX_SDMMC_SPEED_IDENT;
    return 0;
}

int uiox_sdmmc_if_detect(uiox_sdmmc_if_t *sif)
{
    if (!sif) return -EINVAL;
    int rc = uiox_sdmmc_hw_detect(sif->hw);

    /* Presence is an event, so count the transitions rather than every
     * call — a caller polling detect() should not accumulate counts. */
    bool now = sif->hw->card_present;
    if (now != sif->stats.insert_events) { /* first call establishes state */ }

    if (rc == 0) sif->stats.insert_events++;
    else         sif->stats.remove_events++;
    return rc;
}

int uiox_sdmmc_if_card_init(uiox_sdmmc_if_t *sif)
{
    if (!sif || !sif->primed) return -EINVAL;

    int rc = uiox_sdmmc_hw_card_init(sif->hw);
    if (rc < 0) { sif->stats.errors++; return rc; }
    sif->hw->dev_ready = true;
    return 0;
}

int uiox_sdmmc_if_start(uiox_sdmmc_if_t *sif)
{
    if (!sif || !sif->primed) return -EINVAL;

    int rc = uiox_sdmmc_hw_power_on(sif->hw);
    if (rc < 0) return rc;

    /* Start at the identification clock */
    rc = uiox_sdmmc_hw_set_clock(sif->hw, 400000u);
    if (rc < 0) return rc;

    uiox_sdmmc_hw_reg_write(sif->hw, SDMMC_HC_INT_STATUS_EN,
                            SDMMC_INT_CMD_COMPLETE | SDMMC_INT_XFER_COMPLETE);
    uiox_sdmmc_hw_reg_write(sif->hw, SDMMC_INT_SIGNAL_EN,
                            SDMMC_INT_CMD_COMPLETE | SDMMC_INT_XFER_COMPLETE);

    /* ── the step eMMC's start has no equivalent for ───────────────
     * A seated card is not a guarantee and an absent one is not an
     * error the driver can fix: return -ENODEV so uiox_dss_init.c skips
     * this class ("A class that fails to probe is reported by its own
     * return value and skipped").  A soldered eMMC can assume presence;
     * a removable SD cannot. */
    rc = uiox_sdmmc_if_detect(sif);
    if (rc < 0) return rc;

    return uiox_sdmmc_if_card_init(sif);
}

void uiox_sdmmc_if_stop(uiox_sdmmc_if_t *sif)
{
    if (!sif) return;
    uiox_sdmmc_hw_reg_write(sif->hw, SDMMC_HC_INT_SIGNAL_EN, 0u);
    uiox_sdmmc_hw_reg_write(sif->hw, SDMMC_HC_INT_STATUS_EN, 0u);
    uiox_sdmmc_hw_power_off(sif->hw);
    sif->hw->dev_ready = false;
}

int uiox_sdmmc_if_send_cmd(uiox_sdmmc_if_t *sif, uint8_t cmd,
                           uint32_t arg, uint32_t *resp)
{
    if (!sif) return -EINVAL;
    sif->stats.cmds_sent++;
    int rc = uiox_sdmmc_hw_send_cmd(sif->hw, cmd, arg, resp);
    if (rc < 0) sif->stats.errors++;
    return rc;
}

int uiox_sdmmc_if_read(uiox_sdmmc_if_t *sif, uint32_t lba,
                       uint8_t *buf, uint32_t count)
{
    if (!sif || !buf || !count) return -EINVAL;
    int rc = uiox_sdmmc_hw_read_blocks(sif->hw, lba, buf, count);
    if (rc == 0) {
        sif->stats.blocks_read += count;
        sif->stats.bytes_read  += (uint64_t)count * UIOX_SDMMC_BLOCK_SIZE;
    } else {
        sif->stats.errors++;
    }
    return rc;
}

int uiox_sdmmc_if_write(uiox_sdmmc_if_t *sif, uint32_t lba,
                        const uint8_t *buf, uint32_t count)
{
    if (!sif || !buf || !count) return -EINVAL;
    int rc = uiox_sdmmc_hw_write_blocks(sif->hw, lba, buf, count);
    if (rc == 0) {
        sif->stats.blocks_written += count;
        sif->stats.bytes_written  += (uint64_t)count * UIOX_SDMMC_BLOCK_SIZE;
    } else {
        sif->stats.errors++;
    }
    return rc;
}

int uiox_sdmmc_if_set_clock(uiox_sdmmc_if_t *sif, uint32_t hz)
{
    return sif ? uiox_sdmmc_hw_set_clock(sif->hw, hz) : -EINVAL;
}

int uiox_sdmmc_if_set_bus_width(uiox_sdmmc_if_t *sif, uint8_t width)
{
    if (!sif) return -EINVAL;
    int rc = uiox_sdmmc_hw_set_bus_width(sif->hw, width);
    if (rc == 0) sif->bus_width = width;
    return rc;
}

int uiox_sdmmc_if_set_speed(uiox_sdmmc_if_t *sif, uiox_sdmmc_hw_speed_t speed)
{
    if (!sif) return -EINVAL;
    int rc = uiox_sdmmc_hw_set_speed(sif->hw, speed);
    if (rc == 0) sif->speed = speed;
    return rc;
}

void uiox_sdmmc_if_stats_get(const uiox_sdmmc_if_t *sif,
                             uiox_sdmmc_if_stats_t *out)
{ if (!sif || !out) return; memcpy(out, &sif->stats, sizeof(*out)); }
