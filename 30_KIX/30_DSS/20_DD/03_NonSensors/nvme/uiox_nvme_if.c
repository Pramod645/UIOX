/**
 * @file  uiox_nvme_if.c
 * @brief UIOX NVMe interface driver implementation.
 * @date  2026-10-19
 */
#include "uiox_nvme_if.h"

int uiox_nvme_if_config(uiox_nvme_if_t *nif, uiox_nvme_hw_t *hw)
{
    if (!nif || !hw) return -EINVAL;
    memset(nif, 0, sizeof(*nif));
    nif->hw     = hw;
    nif->primed = true;
    return 0;
}

int uiox_nvme_if_start(uiox_nvme_if_t *nif)
{
    if (!nif || !nif->primed) return -EINVAL;

    int rc = uiox_nvme_hw_power_on(nif->hw);
    if (rc < 0) return rc;

    /* Controller enable is the CC.EN ↔ CSTS.RDY handshake, including the
     * CSTS.CFS abort — one call because the sequence is indivisible. */
    rc = uiox_nvme_hw_enable(nif->hw, true);
    if (rc < 0) return rc;

    /* Learn the geometry: model/serial/fw_rev, nsid, capacity.  Without
     * this hw->ident is zeroed and num_blocks() reports 0 — "unknown",
     * which bcache reads as "skip the bound check". */
    rc = uiox_nvme_if_identify(nif);
    if (rc < 0) return rc;

    nif->hw->dev_ready = true;
    return 0;
}

void uiox_nvme_if_stop(uiox_nvme_if_t *nif)
{
    if (!nif) return;
    (void)uiox_nvme_hw_enable(nif->hw, false);
    uiox_nvme_hw_power_off(nif->hw);
    nif->hw->dev_ready = false;
}

int uiox_nvme_if_read(uiox_nvme_if_t *nif, uint64_t lba,
                      uint8_t *buf, uint32_t count)
{
    if (!nif || !buf || !count) return -EINVAL;
    nif->stats.cmds_submitted++;
    int rc = uiox_nvme_hw_read_blocks(nif->hw, lba, buf, count);
    if (rc == 0) {
        nif->stats.blocks_read += count;
        nif->stats.bytes_read  += (uint64_t)count * UIOX_NVME_BLOCK_SIZE;
    } else {
        nif->stats.errors++;
    }
    return rc;
}

int uiox_nvme_if_write(uiox_nvme_if_t *nif, uint64_t lba,
                       const uint8_t *buf, uint32_t count)
{
    if (!nif || !buf || !count) return -EINVAL;
    nif->stats.cmds_submitted++;
    int rc = uiox_nvme_hw_write_blocks(nif->hw, lba, buf, count);
    if (rc == 0) {
        nif->stats.blocks_written += count;
        nif->stats.bytes_written  += (uint64_t)count * UIOX_NVME_BLOCK_SIZE;
    } else {
        nif->stats.errors++;
    }
    return rc;
}

int uiox_nvme_if_identify(uiox_nvme_if_t *nif)
{
    if (!nif) return -EINVAL;
    return uiox_nvme_hw_identify(nif->hw);
}

int uiox_nvme_if_flush(uiox_nvme_if_t *nif)
{
    if (!nif) return -EINVAL;
    int rc = uiox_nvme_hw_flush(nif->hw);
    if (rc == 0) nif->stats.flush_count++;
    else         nif->stats.errors++;
    return rc;
}

void uiox_nvme_if_stats_get(const uiox_nvme_if_t *nif,
                            uiox_nvme_if_stats_t *out)
{ if (!nif || !out) return; memcpy(out, &nif->stats, sizeof(*out)); }
