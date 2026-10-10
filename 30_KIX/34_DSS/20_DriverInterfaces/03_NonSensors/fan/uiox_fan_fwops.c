/**
 * @file    uiox_fan_fwops.c
 * @brief   Adapter: 02_FwHal implementation -> layer-20 uiox_fan_hw_ops_t.
 *
 * PATH   30_KIX/34_DSS/20_DriverInterfaces/03_NonSensors/fan/uiox_fan_fwops.c
 * LAYER  20_DriverInterfaces -- hardware-control interface.
 *
 * BOUNDARY
 *   Above: 30_DeviceDrivers/uiox_fan_if.c calls through hw->ops and never
 *          sees a firmware symbol.
 *   Below: 02_FwHal builds to libuiox_fw_<arch>.a; this file calls its flat
 *          uiox_fw_fan_*() API and never includes its sources.
 *   This file is the joint.
 *
 * REGENERATED -- every stub carries the exact signature of its vtable slot
 * (18 slots).  Bodies return -ENOSYS: an honest "not yet
 * bound", never a silent success.  Every slot parameter is explicitly cast
 * to void so the strict -Werror set stays intact for the driver units that
 * do carry logic.
 */

#include "uiox_fan_hw.h"
#include "uiox_fw_rc.h"
#include "uiox_fw_fan.h"
#include "uiox_fw_i2c.h"

#define UIOX_FAN_FWBIND_MAX   2u

typedef struct {
    uiox_fan_dev_t  fw;
    uiox_i2c_dev_t     i2c;
    uiox_fan_hw_t             *hw;
    bool                in_use;
} uiox_fan_fwbind_t;

static uiox_fan_fwbind_t s_fan_bind[UIOX_FAN_FWBIND_MAX];

static uiox_fan_fwbind_t *bind_of(uiox_fan_hw_t *hw)
{
    if (!hw) return NULL;
    for (uint32_t i = 0u; i < UIOX_FAN_FWBIND_MAX; i++)
        if (s_fan_bind[i].in_use && s_fan_bind[i].hw == hw)
            return &s_fan_bind[i];
    return NULL;
}

/* ---- vtable slots ---- */
static int fw_fan_init(uiox_fan_hw_t *hw)
{
    (void)hw;
    uiox_fan_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

/* deinit: IRQ/void slot -- the firmware layer owns the IRQ controller
 * but does not route device interrupts into class handlers
 * (10_BSP/10_Arch/[*]/arch_runtime.c). */
static void fw_fan_deinit(uiox_fan_hw_t *hw) { (void)hw; }
static int fw_fan_reg_read(uiox_fan_hw_t *hw, uint8_t reg, uint8_t *val)
{
    (void)hw; (void)reg; (void)val;
    uiox_fan_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_fan_reg_write(uiox_fan_hw_t *hw, uint8_t reg, uint8_t val)
{
    (void)hw; (void)reg; (void)val;
    uiox_fan_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_fan_reg_read16(uiox_fan_hw_t *hw, uint8_t reg, uint16_t *val)
{
    (void)hw; (void)reg; (void)val;
    uiox_fan_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_fan_reg_write16(uiox_fan_hw_t *hw, uint8_t reg, uint16_t val)
{
    (void)hw; (void)reg; (void)val;
    uiox_fan_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_fan_set_pwm(uiox_fan_hw_t *hw, uint8_t ch, uint8_t duty)
{
    (void)hw; (void)ch; (void)duty;
    uiox_fan_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_fan_get_pwm(uiox_fan_hw_t *hw, uint8_t ch, uint8_t *duty)
{
    (void)hw; (void)ch; (void)duty;
    uiox_fan_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_fan_set_rpm_target(uiox_fan_hw_t *hw, uint8_t ch, uint16_t rpm)
{
    (void)hw; (void)ch; (void)rpm;
    uiox_fan_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_fan_read_rpm(uiox_fan_hw_t *hw, uint8_t ch, uint16_t *rpm)
{
    (void)hw; (void)ch; (void)rpm;
    uiox_fan_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_fan_read_temp(uiox_fan_hw_t *hw, uint8_t ch, int16_t *temp_dc)
{
    (void)hw; (void)ch; (void)temp_dc;
    uiox_fan_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_fan_fault_status(uiox_fan_hw_t *hw, uint32_t *flags)
{
    (void)hw; (void)flags;
    uiox_fan_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_fan_fault_clear(uiox_fan_hw_t *hw, uint32_t flags)
{
    (void)hw; (void)flags;
    uiox_fan_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_fan_chan_enable(uiox_fan_hw_t *hw, uint8_t ch, bool en)
{
    (void)hw; (void)ch; (void)en;
    uiox_fan_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

/* gpio_write: IRQ/void slot -- the firmware layer owns the IRQ controller
 * but does not route device interrupts into class handlers
 * (10_BSP/10_Arch/[*]/arch_runtime.c). */
static void fw_fan_gpio_write(uiox_fan_hw_t *hw, uint32_t pin, bool val) { (void)hw; (void)pin; (void)val; }

/* gpio_read: IRQ/void slot -- the firmware layer owns the IRQ controller
 * but does not route device interrupts into class handlers
 * (10_BSP/10_Arch/[*]/arch_runtime.c). */
static bool fw_fan_gpio_read(uiox_fan_hw_t *hw, uint32_t pin) { (void)hw; (void)pin; }
static int fw_fan_wdt_kick(uiox_fan_hw_t *hw)
{
    (void)hw;
    uiox_fan_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

/* isr: IRQ/void slot -- the firmware layer owns the IRQ controller
 * but does not route device interrupts into class handlers
 * (10_BSP/10_Arch/[*]/arch_runtime.c). */
static void fw_fan_isr(uiox_fan_hw_t *hw) { (void)hw; }

static const uiox_fan_hw_ops_t s_fan_fwops = {
    .init           = fw_fan_init,
    .deinit         = fw_fan_deinit,
    .reg_read       = fw_fan_reg_read,
    .reg_write      = fw_fan_reg_write,
    .reg_read16     = fw_fan_reg_read16,
    .reg_write16    = fw_fan_reg_write16,
    .set_pwm        = fw_fan_set_pwm,
    .get_pwm        = fw_fan_get_pwm,
    .set_rpm_target = fw_fan_set_rpm_target,
    .read_rpm       = fw_fan_read_rpm,
    .read_temp      = fw_fan_read_temp,
    .fault_status   = fw_fan_fault_status,
    .fault_clear    = fw_fan_fault_clear,
    .chan_enable    = fw_fan_chan_enable,
    .gpio_write     = fw_fan_gpio_write,
    .gpio_read      = fw_fan_gpio_read,
    .wdt_kick       = fw_fan_wdt_kick,
    .isr            = fw_fan_isr,
};

/* ---- binding entry point ---- */
int uiox_fan_fwbind(uiox_fan_hw_t *hw, struct uiox_i2c_dev *i2c)
{
    if (!hw || !i2c) return -EINVAL;
    uiox_fan_fwbind_t *b = NULL;
    for (uint32_t i = 0u; i < UIOX_FAN_FWBIND_MAX; i++)
        if (!s_fan_bind[i].in_use) { b = &s_fan_bind[i]; break; }
    if (!b) return -ENOSPC;
    b->in_use = true;
    b->i2c = *i2c;
    b->hw  = hw;
    uiox_fw_err_t frc = uiox_fw_fan_init(&b->fw, i2c);
    if (frc != UIOX_FW_OK) { b->in_use = false; return uiox_fw_rc(frc); }
    int rc = uiox_fan_hw_init(hw, &s_fan_fwops);
    if (rc < 0) { b->in_use = false; return rc; }
    return 0;
}
