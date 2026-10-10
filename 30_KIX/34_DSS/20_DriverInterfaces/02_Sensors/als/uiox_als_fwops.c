/**
 * @file    uiox_als_fwops.c
 * @brief   Adapter: 02_FwHal implementation -> layer-20 uiox_als_hw_ops_t.
 * @date    2026-10-10
 *
 * PATH   30_KIX/34_DSS/20_DriverInterfaces/02_Sensors/als/uiox_als_fwops.c
 * LAYER  20_DriverInterfaces — hardware-control interface.
 *
 * BOUNDARY
 *   Above: 30_DeviceDrivers/uiox_als_if.c calls through hw->ops and never
 *          sees a firmware symbol.
 *   Below: 02_FwHal builds to libuioxfw<arch>.a; this file calls its flat
 *          uiox_fw_als_*() API and never includes its sources.
 *   This file is the joint: it includes the FwHal public header for the
 *   declarations, and the archive supplies the definitions at link time.
 *
 * GENERATED — every slot in uiox_als_hw_ops_t has an entry below.
 * Slots marked ENOSYS are the interrupt/GPIO entry points: the firmware
 * layer owns the IRQ controller but does not route device interrupts into
 * class handlers (that is 10_BSP/10_Arch/*/arch_runtime.c).  Reporting
 * unsupported is deliberate — silently accepting would swallow an IRQ
 * nothing services.
 */

#include "uiox_als_hw.h"
#include "uiox_fw_rc.h"
#include "uiox_fw_als.h"
#include "uiox_fw_i2c.h"

#define UIOX_ALS_FWBIND_MAX   2u

typedef struct {
    uiox_fw_als_dev_t  fw;
    uiox_i2c_dev_t                     i2c;
    uiox_als_hw_t                  *hw;
    bool                               in_use;
} uiox_als_fwbind_t;

static uiox_als_fwbind_t s_als_bind[UIOX_ALS_FWBIND_MAX];

static uiox_als_fwbind_t *bind_of(uiox_als_hw_t *hw)
{
    if (!hw || !hw->base) return NULL;
    uiox_als_fwbind_t *b = (uiox_als_fwbind_t *)hw->base;
    return (b->in_use && b->hw == hw) ? b : NULL;
}

/* ---- vtable slots ---- */

static int fw_als_init(uiox_als_hw_t *hw)
{
    uiox_als_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static void fw_als_deinit(uiox_als_hw_t *hw)
{
    uiox_als_fwbind_t *b = bind_of(hw);
    if (!b) return;
}

static int fw_als_power_on(uiox_als_hw_t *hw)
{
    uiox_als_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static void fw_als_power_off(uiox_als_hw_t *hw)
{
    uiox_als_fwbind_t *b = bind_of(hw);
    if (!b) return;
}

static int fw_als_reg_read(uiox_als_hw_t *hw, uint8_t reg, uint16_t *val)
{
    uiox_als_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    if (!val) return -EINVAL;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_als_reg_write(uiox_als_hw_t *hw, uint8_t reg, uint16_t val)
{
    uiox_als_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_als_set_gain(uiox_als_hw_t *hw)
{
    uiox_als_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_als_set_itime(uiox_als_hw_t *hw)
{
    uiox_als_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_als_read_als(uiox_als_hw_t *hw)
{
    uiox_als_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_als_read_ir(uiox_als_hw_t *hw)
{
    uiox_als_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_als_set_threshold(uiox_als_hw_t *hw)
{
    uiox_als_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_als_int_enable(uiox_als_hw_t *hw)
{
    uiox_als_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_als_int_clear(uiox_als_hw_t *hw)
{
    uiox_als_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_als_trigger(uiox_als_hw_t *hw)
{
    uiox_als_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static void fw_als_gpio_write(uiox_als_hw_t *hw, uint32_t pin, bool v)
{ (void)hw; (void)pin; (void)v; }

static bool fw_als_gpio_read(uiox_als_hw_t *hw, uint32_t pin)
{ (void)hw; (void)pin; return false; }

static void fw_als_isr(uiox_als_hw_t *hw)
{ (void)hw; }

static const uiox_als_hw_ops_t s_als_fwops = {
    .init          = fw_als_init,
    .deinit        = fw_als_deinit,
    .power_on      = fw_als_power_on,
    .power_off     = fw_als_power_off,
    .reg_read      = fw_als_reg_read,
    .reg_write     = fw_als_reg_write,
    .set_gain      = fw_als_set_gain,
    .set_itime     = fw_als_set_itime,
    .read_als      = fw_als_read_als,
    .read_ir       = fw_als_read_ir,
    .set_threshold = fw_als_set_threshold,
    .int_enable    = fw_als_int_enable,
    .int_clear     = fw_als_int_clear,
    .trigger       = fw_als_trigger,
    .gpio_write    = fw_als_gpio_write,
    .gpio_read     = fw_als_gpio_read,
    .isr           = fw_als_isr,
};

/* ---- binding entry points (one per 02_FwHal chip helper) ---- */
int uiox_als_fwbind   (uiox_als_hw_t *hw, uiox_i2c_dev_t *i2c)
{
    if (!hw || !i2c) return -EINVAL;
    uiox_als_fwbind_t *b = NULL;
    for (uint32_t i = 0u; i < UIOX_ALS_FWBIND_MAX; i++)
        if (!s_als_bind[i].in_use) { b = &s_als_bind[i]; break; }
    if (!b) return -ENOSPC;
    b->in_use = true;
    b->i2c = *i2c;
    b->hw  = hw;
    uiox_fw_err_t frc = uiox_fw_als_init(&b->fw, i2c);
    if (frc != UIOX_FW_OK) { b->in_use = false; return uiox_fw_rc(frc); }
    hw->base = (uintptr_t)b;
    int rc = uiox_als_hw_init(hw, &s_als_fwops);
    if (rc < 0) { b->in_use = false; return rc; }
    return 0;
}
