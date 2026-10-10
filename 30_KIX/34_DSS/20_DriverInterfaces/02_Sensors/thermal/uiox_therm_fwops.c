/**
 * @file    uiox_therm_fwops.c
 * @brief   Adapter: 02_FwHal implementation -> layer-20 uiox_therm_hw_ops_t.
 * @date    2026-10-10
 *
 * PATH   30_KIX/34_DSS/20_DriverInterfaces/02_Sensors/thermal/uiox_therm_fwops.c
 * LAYER  20_DriverInterfaces — hardware-control interface.
 *
 * BOUNDARY
 *   Above: 30_DeviceDrivers/uiox_therm_if.c calls through hw->ops and never
 *          sees a firmware symbol.
 *   Below: 02_FwHal builds to libuioxfw<arch>.a; this file calls its flat
 *          uiox_fw_therm_*() API and never includes its sources.
 *   This file is the joint: it includes the FwHal public header for the
 *   declarations, and the archive supplies the definitions at link time.
 *
 * GENERATED — every slot in uiox_therm_hw_ops_t has an entry below.
 * Slots marked ENOSYS are the interrupt/GPIO entry points: the firmware
 * layer owns the IRQ controller but does not route device interrupts into
 * class handlers (that is 10_BSP/10_Arch/*/arch_runtime.c).  Reporting
 * unsupported is deliberate — silently accepting would swallow an IRQ
 * nothing services.
 */

#include "uiox_therm_hw.h"
#include "uiox_fw_rc.h"
#include "uiox_fw_therm.h"
#include "uiox_fw_i2c.h"

#define UIOX_THERM_FWBIND_MAX   2u

typedef struct {
    uiox_fw_therm_dev_t  fw;
    uiox_i2c_dev_t                     i2c;
    uiox_therm_hw_t                  *hw;
    bool                               in_use;
} uiox_therm_fwbind_t;

static uiox_therm_fwbind_t s_therm_bind[UIOX_THERM_FWBIND_MAX];

static uiox_therm_fwbind_t *bind_of(uiox_therm_hw_t *hw)
{
    if (!hw || !hw->base) return NULL;
    uiox_therm_fwbind_t *b = (uiox_therm_fwbind_t *)hw->base;
    return (b->in_use && b->hw == hw) ? b : NULL;
}

/* ---- vtable slots ---- */

static int fw_therm_init(uiox_therm_hw_t *hw)
{
    uiox_therm_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static void fw_therm_deinit(uiox_therm_hw_t *hw)
{
    uiox_therm_fwbind_t *b = bind_of(hw);
    if (!b) return;
}

static int fw_therm_reg_read(uiox_therm_hw_t *hw, uint8_t reg, uint16_t *val)
{
    uiox_therm_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    if (!val) return -EINVAL;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_therm_reg_write(uiox_therm_hw_t *hw, uint8_t reg, uint16_t val)
{
    uiox_therm_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_therm_reg_read16(uiox_therm_hw_t *hw, uint8_t reg, uint16_t *val)
{
    uiox_therm_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    if (!val) return -EINVAL;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_therm_reg_write16(uiox_therm_hw_t *hw, uint8_t reg, uint16_t val)
{
    uiox_therm_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_therm_read_temp(uiox_therm_hw_t *hw, uint8_t ch, int16_t *temp_dc)
{
    uiox_therm_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    if (!temp_dc) return -EINVAL;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_therm_set_t_high(uiox_therm_hw_t *hw, int16_t t_dc)
{
    uiox_therm_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_therm_set_t_hyst(uiox_therm_hw_t *hw, int16_t t_dc)
{
    uiox_therm_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_therm_set_t_crit(uiox_therm_hw_t *hw, int16_t t_dc)
{
    uiox_therm_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_therm_set_resolution(uiox_therm_hw_t *hw)
{
    uiox_therm_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_therm_set_mode(uiox_therm_hw_t *hw)
{
    uiox_therm_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_therm_oneshot(uiox_therm_hw_t *hw)
{
    uiox_therm_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_therm_alert_status(uiox_therm_hw_t *hw)
{
    uiox_therm_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_therm_alert_clear(uiox_therm_hw_t *hw)
{
    uiox_therm_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_therm_adc_read(uiox_therm_hw_t *hw)
{
    uiox_therm_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static bool fw_therm_gpio_read(uiox_therm_hw_t *hw, uint32_t pin)
{ (void)hw; (void)pin; return false; }

static void fw_therm_isr(uiox_therm_hw_t *hw)
{ (void)hw; }

static const uiox_therm_hw_ops_t s_therm_fwops = {
    .init          = fw_therm_init,
    .deinit        = fw_therm_deinit,
    .reg_read      = fw_therm_reg_read,
    .reg_write     = fw_therm_reg_write,
    .reg_read16    = fw_therm_reg_read16,
    .reg_write16   = fw_therm_reg_write16,
    .read_temp     = fw_therm_read_temp,
    .set_t_high    = fw_therm_set_t_high,
    .set_t_hyst    = fw_therm_set_t_hyst,
    .set_t_crit    = fw_therm_set_t_crit,
    .set_resolution= fw_therm_set_resolution,
    .set_mode      = fw_therm_set_mode,
    .oneshot       = fw_therm_oneshot,
    .alert_status  = fw_therm_alert_status,
    .alert_clear   = fw_therm_alert_clear,
    .adc_read      = fw_therm_adc_read,
    .gpio_read     = fw_therm_gpio_read,
    .isr           = fw_therm_isr,
};

/* ---- binding entry points (one per 02_FwHal chip helper) ---- */
int uiox_therm_fwbind   (uiox_therm_hw_t *hw, uiox_i2c_dev_t *i2c)
{
    if (!hw || !i2c) return -EINVAL;
    uiox_therm_fwbind_t *b = NULL;
    for (uint32_t i = 0u; i < UIOX_THERM_FWBIND_MAX; i++)
        if (!s_therm_bind[i].in_use) { b = &s_therm_bind[i]; break; }
    if (!b) return -ENOSPC;
    b->in_use = true;
    b->i2c = *i2c;
    b->hw  = hw;
    uiox_fw_err_t frc = uiox_fw_therm_init(&b->fw, i2c);
    if (frc != UIOX_FW_OK) { b->in_use = false; return uiox_fw_rc(frc); }
    hw->base = (uintptr_t)b;
    int rc = uiox_therm_hw_init(hw, &s_therm_fwops);
    if (rc < 0) { b->in_use = false; return rc; }
    return 0;
}
