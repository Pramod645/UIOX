/**
 * @file  uiox_sdmmc_hw.c
 * @brief UIOX SD/SDHC/SDXC HAL implementation.
 *
 * Every public function is a guard plus a forward through OPS(hw) — the
 * shape uiox_emmc_hw.c uses.  No hardware knowledge lives here: the
 * vtable is 02_FwHal's, bound by uiox_sdmmc_fwops.c.
 */
#include "uiox_sdmmc_hw.h"

#define OPS(hw) ((const uiox_sdmmc_hw_ops_t *)(hw)->ops)

int uiox_sdmmc_hw_init(uiox_sdmmc_hw_t *hw, const uiox_sdmmc_hw_ops_t *ops)
{
    if (!hw || !ops || !ops->init) return -EINVAL;
    hw->ops          = (void *)ops;
    hw->speed        = UIOX_SDMMC_SPEED_IDENT;
    hw->bus_width    = 1u;
    hw->clk_hz       = 400000u;
    hw->dev_ready    = false;
    hw->card_present = false;
    hw->write_protect= false;
    hw->pending_irq  = 0u;
    memset(&hw->ident, 0, sizeof(hw->ident));
    return ops->init(hw);
}

void uiox_sdmmc_hw_deinit(uiox_sdmmc_hw_t *hw)
{
    if (!hw || !hw->ops) return;
    if (OPS(hw)->deinit) OPS(hw)->deinit(hw);
    hw->ops = NULL;
}

int uiox_sdmmc_hw_power_on(uiox_sdmmc_hw_t *hw)
{
    if (!hw || !hw->ops) return -EINVAL;
    if (!OPS(hw)->power_on) return -ENOSYS;
    return OPS(hw)->power_on(hw);
}

void uiox_sdmmc_hw_power_off(uiox_sdmmc_hw_t *hw)
{
    if (!hw || !hw->ops) return;
    if (OPS(hw)->power_off) OPS(hw)->power_off(hw);
}

uint32_t uiox_sdmmc_hw_reg_read(uiox_sdmmc_hw_t *hw, uint32_t offset)
{
    if (!hw || !hw->ops || !OPS(hw)->reg_read) return 0u;
    return OPS(hw)->reg_read(hw, offset);
}

void uiox_sdmmc_hw_reg_write(uiox_sdmmc_hw_t *hw, uint32_t offset, uint32_t val)
{
    if (!hw || !hw->ops || !OPS(hw)->reg_write) return;
    OPS(hw)->reg_write(hw, offset, val);
}

int uiox_sdmmc_hw_set_clock(uiox_sdmmc_hw_t *hw, uint32_t hz)
{
    if (!hw || !hw->ops) return -EINVAL;
    if (!OPS(hw)->set_clock) return -ENOSYS;
    int rc = OPS(hw)->set_clock(hw, hz);
    if (rc == 0) hw->clk_hz = hz;
    return rc;
}

int uiox_sdmmc_hw_set_bus_width(uiox_sdmmc_hw_t *hw, uint8_t width)
{
    if (!hw || !hw->ops) return -EINVAL;
    if (!OPS(hw)->set_bus_width) return -ENOSYS;
    int rc = OPS(hw)->set_bus_width(hw, width);
    if (rc == 0) hw->bus_width = width;
    return rc;
}

int uiox_sdmmc_hw_set_speed(uiox_sdmmc_hw_t *hw, uiox_sdmmc_hw_speed_t speed)
{
    if (!hw || !hw->ops) return -EINVAL;
    if (!OPS(hw)->set_speed_mode) return -ENOSYS;
    int rc = OPS(hw)->set_speed_mode(hw, speed);
    if (rc == 0) hw->speed = speed;
    return rc;
}

/* Chains to the firmware's detect(), which sets card_present and
 * write_protect on the FwHal descriptor; this copies them across. */
int uiox_sdmmc_hw_detect(uiox_sdmmc_hw_t *hw)
{
    if (!hw || !hw->ops) return -EINVAL;
    if (!OPS(hw)->detect) return -ENOSYS;
    return OPS(hw)->detect(hw);
}

int uiox_sdmmc_hw_card_init(uiox_sdmmc_hw_t *hw)
{
    if (!hw || !hw->ops) return -EINVAL;
    if (!OPS(hw)->card_init) return -ENOSYS;
    return OPS(hw)->card_init(hw);
}

int uiox_sdmmc_hw_send_cmd(uiox_sdmmc_hw_t *hw, uint8_t cmd,
                           uint32_t arg, uint32_t *resp)
{
    if (!hw || !hw->ops) return -EINVAL;
    if (!OPS(hw)->send_cmd) return -ENOSYS;
    return OPS(hw)->send_cmd(hw, cmd, arg, resp);
}

int uiox_sdmmc_hw_read_blocks(uiox_sdmmc_hw_t *hw, uint32_t lba,
                              uint8_t *buf, uint32_t count)
{
    if (!hw || !buf || !count) return -EINVAL;
    if (!hw->ops || !OPS(hw)->read_blocks) return -ENOSYS;
    return OPS(hw)->read_blocks(hw, lba, buf, count);
}

int uiox_sdmmc_hw_write_blocks(uiox_sdmmc_hw_t *hw, uint32_t lba,
                               const uint8_t *buf, uint32_t count)
{
    if (!hw || !buf || !count) return -EINVAL;
    if (!hw->ops || !OPS(hw)->write_blocks) return -ENOSYS;
    return OPS(hw)->write_blocks(hw, lba, buf, count);
}

void uiox_sdmmc_hw_isr(uiox_sdmmc_hw_t *hw)
{
    if (!hw || !hw->ops) return;
    if (OPS(hw)->isr) OPS(hw)->isr(hw);
}
