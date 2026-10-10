/**
 * @file    uiox_emmc_fwops.c
 * @brief   Adapter: 02_FwHal implementation -> layer-20 uiox_emmc_hw_ops_t.
 * @date    2026-10-10
 *
 * PATH   30_KIX/34_DSS/20_DriverInterfaces/03_NonSensors/emmc/uiox_emmc_fwops.c
 * LAYER  20_DriverInterfaces — hardware-control interface.
 *
 * BOUNDARY
 *   Above: 30_DeviceDrivers/uiox_emmc_if.c calls through hw->ops and never
 *          sees a firmware symbol.
 *   Below: 02_FwHal builds to libuioxfw<arch>.a; this file calls its flat
 *          uiox_fw_emmc_*() API and never includes its sources.
 *   This file is the joint: it includes the FwHal public header for the
 *   declarations, and the archive supplies the definitions at link time.
 *
 * GENERATED — every slot in uiox_emmc_hw_ops_t has an entry below.
 * Slots marked ENOSYS are the interrupt/GPIO entry points: the firmware
 * layer owns the IRQ controller but does not route device interrupts into
 * class handlers (that is 10_BSP/10_Arch/*/arch_runtime.c).  Reporting
 * unsupported is deliberate — silently accepting would swallow an IRQ
 * nothing services.
 */

#include "uiox_emmc_hw.h"
#include "uiox_fw_rc.h"
#include "uiox_fw_emmc.h"
#include "uiox_fw_i2c.h"

#define UIOX_EMMC_FWBIND_MAX   2u

typedef struct {
    uiox_fw_emmc_dev_t  fw;
    uiox_i2c_dev_t                     i2c;
    uiox_emmc_hw_t                  *hw;
    bool                               in_use;
} uiox_emmc_fwbind_t;

static uiox_emmc_fwbind_t s_emmc_bind[UIOX_EMMC_FWBIND_MAX];

static uiox_emmc_fwbind_t *bind_of(uiox_emmc_hw_t *hw)
{
    if (!hw || !hw->base) return NULL;
    uiox_emmc_fwbind_t *b = (uiox_emmc_fwbind_t *)hw->base;
    return (b->in_use && b->hw == hw) ? b : NULL;
}

/* ---- vtable slots ---- */

static int fw_emmc_init(uiox_emmc_hw_t *hw)
{
    uiox_emmc_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static void fw_emmc_deinit(uiox_emmc_hw_t *hw)
{
    uiox_emmc_fwbind_t *b = bind_of(hw);
    if (!b) return;
}

static int fw_emmc_power_on(uiox_emmc_hw_t *hw)
{
    uiox_emmc_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static void fw_emmc_power_off(uiox_emmc_hw_t *hw)
{
    uiox_emmc_fwbind_t *b = bind_of(hw);
    if (!b) return;
}

static int fw_emmc_reg_read(uiox_emmc_hw_t *hw, uint8_t reg, uint16_t *val)
{
    uiox_emmc_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    if (!val) return -EINVAL;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_emmc_reg_write(uiox_emmc_hw_t *hw, uint8_t reg, uint16_t val)
{
    uiox_emmc_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_emmc_set_clock(uiox_emmc_hw_t *hw)
{
    uiox_emmc_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_emmc_set_bus_width(uiox_emmc_hw_t *hw)
{
    uiox_emmc_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_emmc_set_speed_mode(uiox_emmc_hw_t *hw)
{
    uiox_emmc_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_emmc_send_cmd(uiox_emmc_hw_t *hw)
{
    uiox_emmc_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_emmc_read_blocks(uiox_emmc_hw_t *hw)
{
    uiox_emmc_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_emmc_write_blocks(uiox_emmc_hw_t *hw)
{
    uiox_emmc_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_emmc_read_ext_csd(uiox_emmc_hw_t *hw)
{
    uiox_emmc_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_emmc_write_ext_csd(uiox_emmc_hw_t *hw)
{
    uiox_emmc_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_emmc_tuning(uiox_emmc_hw_t *hw)
{
    uiox_emmc_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_emmc_crc7(uiox_emmc_hw_t *hw)
{
    uiox_emmc_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_emmc_crc16(uiox_emmc_hw_t *hw)
{
    uiox_emmc_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static void fw_emmc_gpio_write(uiox_emmc_hw_t *hw, uint32_t pin, bool v)
{ (void)hw; (void)pin; (void)v; }

static bool fw_emmc_gpio_read(uiox_emmc_hw_t *hw, uint32_t pin)
{ (void)hw; (void)pin; return false; }

static void fw_emmc_isr(uiox_emmc_hw_t *hw)
{ (void)hw; }

static const uiox_emmc_hw_ops_t s_emmc_fwops = {
    .init          = fw_emmc_init,
    .deinit        = fw_emmc_deinit,
    .power_on      = fw_emmc_power_on,
    .power_off     = fw_emmc_power_off,
    .reg_read      = fw_emmc_reg_read,
    .reg_write     = fw_emmc_reg_write,
    .set_clock     = fw_emmc_set_clock,
    .set_bus_width = fw_emmc_set_bus_width,
    .set_speed_mode= fw_emmc_set_speed_mode,
    .send_cmd      = fw_emmc_send_cmd,
    .read_blocks   = fw_emmc_read_blocks,
    .write_blocks  = fw_emmc_write_blocks,
    .read_ext_csd  = fw_emmc_read_ext_csd,
    .write_ext_csd = fw_emmc_write_ext_csd,
    .tuning        = fw_emmc_tuning,
    .crc7          = fw_emmc_crc7,
    .crc16         = fw_emmc_crc16,
    .gpio_write    = fw_emmc_gpio_write,
    .gpio_read     = fw_emmc_gpio_read,
    .isr           = fw_emmc_isr,
};

/* ---- binding entry points (one per 02_FwHal chip helper) ---- */
int uiox_emmc_fwbind   (uiox_emmc_hw_t *hw, uiox_i2c_dev_t *i2c)
{
    if (!hw || !i2c) return -EINVAL;
    uiox_emmc_fwbind_t *b = NULL;
    for (uint32_t i = 0u; i < UIOX_EMMC_FWBIND_MAX; i++)
        if (!s_emmc_bind[i].in_use) { b = &s_emmc_bind[i]; break; }
    if (!b) return -ENOSPC;
    b->in_use = true;
    b->i2c = *i2c;
    b->hw  = hw;
    uiox_fw_err_t frc = uiox_fw_emmc_init(&b->fw, i2c);
    if (frc != UIOX_FW_OK) { b->in_use = false; return uiox_fw_rc(frc); }
    hw->base = (uintptr_t)b;
    int rc = uiox_emmc_hw_init(hw, &s_emmc_fwops);
    if (rc < 0) { b->in_use = false; return rc; }
    return 0;
}
