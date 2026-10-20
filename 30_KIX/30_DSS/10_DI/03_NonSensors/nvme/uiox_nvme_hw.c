/**
 * @file  uiox_nvme_hw.c
 * @brief UIOX NVMe HAL implementation.
 *
 * Every public function is a guard plus a forward through OPS(hw) — the
 * shape uiox_emmc_hw.c uses.  No hardware knowledge lives here: the
 * vtable below is 02_FwHal's, bound by uiox_nvme_fwops.c.
 */
#include "uiox_nvme_hw.h"

#define OPS(hw) ((const uiox_nvme_hw_ops_t *)(hw)->ops)

int uiox_nvme_hw_init(uiox_nvme_hw_t *hw, const uiox_nvme_hw_ops_t *ops)
{
    if (!hw || !ops || !ops->init) return -EINVAL;
    hw->ops         = (void *)ops;
    hw->caps        = 0u;
    hw->nsid        = 0u;
    hw->queue_depth = 0u;
    hw->enabled     = false;
    hw->dev_ready   = false;
    hw->pending_irq = 0u;
    memset(&hw->ident, 0, sizeof(hw->ident));
    return ops->init(hw);
}

void uiox_nvme_hw_deinit(uiox_nvme_hw_t *hw)
{
    if (!hw || !hw->ops) return;
    if (OPS(hw)->deinit) OPS(hw)->deinit(hw);
    hw->ops = NULL;
}

int uiox_nvme_hw_power_on(uiox_nvme_hw_t *hw)
{
    if (!hw || !hw->ops) return -EINVAL;
    if (!OPS(hw)->power_on) return -ENOSYS;
    return OPS(hw)->power_on(hw);
}

void uiox_nvme_hw_power_off(uiox_nvme_hw_t *hw)
{
    if (!hw || !hw->ops) return;
    if (OPS(hw)->power_off) OPS(hw)->power_off(hw);
}

uint32_t uiox_nvme_hw_reg_read(uiox_nvme_hw_t *hw, uint32_t offset)
{
    if (!hw || !hw->ops || !OPS(hw)->reg_read) return 0u;
    return OPS(hw)->reg_read(hw, offset);
}

void uiox_nvme_hw_reg_write(uiox_nvme_hw_t *hw, uint32_t offset, uint32_t val)
{
    if (!hw || !hw->ops || !OPS(hw)->reg_write) return;
    OPS(hw)->reg_write(hw, offset, val);
}

int uiox_nvme_hw_enable(uiox_nvme_hw_t *hw, bool on)
{
    if (!hw || !hw->ops) return -EINVAL;
    if (!OPS(hw)->enable) return -ENOSYS;
    int rc = OPS(hw)->enable(hw, on);
    if (rc == 0) hw->enabled = on;
    return rc;
}

int uiox_nvme_hw_submit(uiox_nvme_hw_t *hw, uint8_t opcode, uint64_t lba,
                        uint8_t *buf, uint32_t sectors)
{
    if (!hw || !hw->ops) return -EINVAL;
    if (!OPS(hw)->submit) return -ENOSYS;
    return OPS(hw)->submit(hw, opcode, lba, buf, sectors);
}

int uiox_nvme_hw_read_blocks(uiox_nvme_hw_t *hw, uint64_t lba,
                             uint8_t *buf, uint32_t count)
{
    if (!hw || !buf || !count) return -EINVAL;
    if (!hw->ops || !OPS(hw)->read_blocks) return -ENOSYS;
    return OPS(hw)->read_blocks(hw, lba, buf, count);
}

int uiox_nvme_hw_write_blocks(uiox_nvme_hw_t *hw, uint64_t lba,
                              const uint8_t *buf, uint32_t count)
{
    if (!hw || !buf || !count) return -EINVAL;
    if (!hw->ops || !OPS(hw)->write_blocks) return -ENOSYS;
    return OPS(hw)->write_blocks(hw, lba, buf, count);
}

int uiox_nvme_hw_identify(uiox_nvme_hw_t *hw)
{
    if (!hw || !hw->ops) return -EINVAL;
    if (!OPS(hw)->identify) return -ENOSYS;
    return OPS(hw)->identify(hw);
}

int uiox_nvme_hw_flush(uiox_nvme_hw_t *hw)
{
    if (!hw || !hw->ops) return -EINVAL;
    if (!OPS(hw)->flush) return 0;   /* no VWC — nothing to flush */
    return OPS(hw)->flush(hw);
}

void uiox_nvme_hw_isr(uiox_nvme_hw_t *hw)
{
    if (!hw || !hw->ops) return;
    if (OPS(hw)->isr) OPS(hw)->isr(hw);
}
