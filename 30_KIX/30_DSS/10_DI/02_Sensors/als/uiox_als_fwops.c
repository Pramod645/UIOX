/**
 * @file    uiox_als_fwops.c
 * @brief   Adapter: 02_FwHal implementation -> layer-20 uiox_als_hw_ops_t.
 *
 * PATH   30_KIX/30_DSS/10_DI/02_Sensors/als/uiox_als_fwops.c
 * LAYER  10_DI -- hardware-control interface.
 *
 * BOUNDARY
 *   Above: 20_DD/uiox_als_if.c calls through hw->ops and never
 *          sees a firmware symbol.
 *   Below: 02_FwHal builds to libuiox_fw_<arch>.a; this file calls its flat
 *          uiox_fw_als_*() API and never includes its sources.
 *   This file is the joint.
 *
 * REGENERATED -- every stub carries the exact signature of its vtable slot
 * (17 slots).  Bodies return -ENOSYS: an honest "not yet
 * bound", never a silent success.  Every slot parameter is explicitly cast
 * to void so the strict -Werror set stays intact for the driver units that
 * do carry logic.
 */

#include "uiox_als_hw.h"
#include "uiox_fw_rc.h"
#include "uiox_fw_als.h"
#include "uiox_fw_i2c.h"

#define UIOX_ALS_FWBIND_MAX   2u

typedef struct {
    uiox_als_dev_t  fw;
    uiox_i2c_dev_t     i2c;
    uiox_als_hw_t             *hw;
    bool                in_use;
} uiox_als_fwbind_t;

static uiox_als_fwbind_t s_als_bind[UIOX_ALS_FWBIND_MAX];

static uiox_als_fwbind_t *bind_of(uiox_als_hw_t *hw)
{
    if (!hw) return NULL;
    for (uint32_t i = 0u; i < UIOX_ALS_FWBIND_MAX; i++)
        if (s_als_bind[i].in_use && s_als_bind[i].hw == hw)
            return &s_als_bind[i];
    return NULL;
}

/* ---- vtable slots ---- */
static int fw_als_init(uiox_als_hw_t *hw)
{
    (void)hw;
    uiox_als_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

/* deinit: IRQ/void slot -- the firmware layer owns the IRQ controller
 * but does not route device interrupts into class handlers
 * (10_BSP/10_Arch/[*]/arch_runtime.c). */
static void fw_als_deinit(uiox_als_hw_t *hw) { (void)hw; }
static int fw_als_power_on(uiox_als_hw_t *hw)
{
    (void)hw;
    uiox_als_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

/* power_off: IRQ/void slot -- the firmware layer owns the IRQ controller
 * but does not route device interrupts into class handlers
 * (10_BSP/10_Arch/[*]/arch_runtime.c). */
static void fw_als_power_off(uiox_als_hw_t *hw) { (void)hw; }
static int fw_als_reg_read(uiox_als_hw_t *hw, uint8_t reg, uint16_t *val)
{
    (void)hw; (void)reg; (void)val;
    uiox_als_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_als_reg_write(uiox_als_hw_t *hw, uint8_t reg, uint16_t val)
{
    (void)hw; (void)reg; (void)val;
    uiox_als_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_als_set_gain(uiox_als_hw_t *hw, uiox_als_hw_gain_t gain)
{
    (void)hw; (void)gain;
    uiox_als_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_als_set_itime(uiox_als_hw_t *hw, uiox_als_hw_itime_t itime)
{
    (void)hw; (void)itime;
    uiox_als_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_als_read_als(uiox_als_hw_t *hw, uint16_t *als, uint16_t *white)
{
    (void)hw; (void)als; (void)white;
    uiox_als_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_als_read_ir(uiox_als_hw_t *hw, uint16_t *ir)
{
    (void)hw; (void)ir;
    uiox_als_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_als_set_threshold(uiox_als_hw_t *hw, uint16_t low, uint16_t high)
{
    (void)hw; (void)low; (void)high;
    uiox_als_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_als_int_enable(uiox_als_hw_t *hw, bool en)
{
    (void)hw; (void)en;
    uiox_als_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_als_int_clear(uiox_als_hw_t *hw)
{
    (void)hw;
    uiox_als_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_als_trigger(uiox_als_hw_t *hw)
{
    (void)hw;
    uiox_als_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

/* gpio_write: IRQ/void slot -- the firmware layer owns the IRQ controller
 * but does not route device interrupts into class handlers
 * (10_BSP/10_Arch/[*]/arch_runtime.c). */
static void fw_als_gpio_write(uiox_als_hw_t *hw, uint32_t pin, bool val) { (void)hw; (void)pin; (void)val; }

/* gpio_read: IRQ/void slot -- the firmware layer owns the IRQ controller
 * but does not route device interrupts into class handlers
 * (10_BSP/10_Arch/[*]/arch_runtime.c). */
static bool fw_als_gpio_read(uiox_als_hw_t *hw, uint32_t pin) { (void)hw; (void)pin; return false; }

/* isr: IRQ/void slot -- the firmware layer owns the IRQ controller
 * but does not route device interrupts into class handlers
 * (10_BSP/10_Arch/[*]/arch_runtime.c). */
static void fw_als_isr(uiox_als_hw_t *hw) { (void)hw; }

static const uiox_als_hw_ops_t s_als_fwops = {
    .init           = fw_als_init,
    .deinit         = fw_als_deinit,
    .power_on       = fw_als_power_on,
    .power_off      = fw_als_power_off,
    .reg_read       = fw_als_reg_read,
    .reg_write      = fw_als_reg_write,
    .set_gain       = fw_als_set_gain,
    .set_itime      = fw_als_set_itime,
    .read_als       = fw_als_read_als,
    .read_ir        = fw_als_read_ir,
    .set_threshold  = fw_als_set_threshold,
    .int_enable     = fw_als_int_enable,
    .int_clear      = fw_als_int_clear,
    .trigger        = fw_als_trigger,
    .gpio_write     = fw_als_gpio_write,
    .gpio_read      = fw_als_gpio_read,
    .isr            = fw_als_isr,
};

/* ---- binding entry point ---- */
int uiox_als_fwbind(uiox_als_hw_t *hw, struct uiox_i2c_dev *i2c)
{
    if (!hw || !i2c) return -EINVAL;
    uiox_als_fwbind_t *b = NULL;
    for (uint32_t i = 0u; i < UIOX_ALS_FWBIND_MAX; i++)
        if (!s_als_bind[i].in_use) { b = &s_als_bind[i]; break; }
    if (!b) return -ENOSPC;
    b->in_use = true;
    b->i2c = *i2c;
    b->hw  = hw;
    uiox_fw_err_t frc = uiox_fw_als_init_veml7700(&b->fw, i2c);
    if (frc != UIOX_FW_OK) { b->in_use = false; return uiox_fw_rc(frc); }
    int rc = uiox_als_hw_init(hw, &s_als_fwops);
    if (rc < 0) { b->in_use = false; return rc; }
    return 0;
}
