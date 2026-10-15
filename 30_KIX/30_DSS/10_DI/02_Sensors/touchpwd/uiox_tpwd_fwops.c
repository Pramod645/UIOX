/**
 * @file    uiox_tpwd_fwops.c
 * @brief   Adapter: 02_FwHal implementation -> layer-20 uiox_tpwd_hw_ops_t.
 *
 * PATH   30_KIX/34_DSS/20_DriverInterfaces/02_Sensors/touchpwd/uiox_tpwd_fwops.c
 * LAYER  20_DriverInterfaces -- hardware-control interface.
 *
 * BOUNDARY
 *   Above: 30_DeviceDrivers/uiox_tpwd_if.c calls through hw->ops and never
 *          sees a firmware symbol.
 *   Below: 02_FwHal builds to libuiox_fw_<arch>.a; this file calls its flat
 *          uiox_fw_touchpwd_*() API and never includes its sources.
 *   This file is the joint.
 *
 * REGENERATED -- every stub carries the exact signature of its vtable slot
 * (11 slots).  Bodies return -ENOSYS: an honest "not yet
 * bound", never a silent success.  Every slot parameter is explicitly cast
 * to void so the strict -Werror set stays intact for the driver units that
 * do carry logic.
 */

#include "uiox_tpwd_hw.h"
#include "uiox_fw_rc.h"
#include "uiox_fw_touchpwd.h"
#include "uiox_fw_i2c.h"

#define UIOX_TPWD_FWBIND_MAX   2u

typedef struct {
    uiox_fp_dev_t  fw;
    uiox_i2c_dev_t     i2c;
    uiox_tpwd_hw_t             *hw;
    bool                in_use;
} uiox_tpwd_fwbind_t;

static uiox_tpwd_fwbind_t s_tpwd_bind[UIOX_TPWD_FWBIND_MAX];

static uiox_tpwd_fwbind_t *bind_of(uiox_tpwd_hw_t *hw)
{
    if (!hw) return NULL;
    for (uint32_t i = 0u; i < UIOX_TPWD_FWBIND_MAX; i++)
        if (s_tpwd_bind[i].in_use && s_tpwd_bind[i].hw == hw)
            return &s_tpwd_bind[i];
    return NULL;
}

/* ---- vtable slots ---- */
static int fw_tpwd_init(uiox_tpwd_hw_t *hw)
{
    (void)hw;
    uiox_tpwd_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

/* deinit: IRQ/void slot -- the firmware layer owns the IRQ controller
 * but does not route device interrupts into class handlers
 * (10_BSP/10_Arch/[*]/arch_runtime.c). */
static void fw_tpwd_deinit(uiox_tpwd_hw_t *hw) { (void)hw; }
static int fw_tpwd_power(uiox_tpwd_hw_t *hw, bool on)
{
    (void)hw; (void)on;
    uiox_tpwd_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_tpwd_reset(uiox_tpwd_hw_t *hw)
{
    (void)hw;
    uiox_tpwd_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_tpwd_i2c_read(uiox_tpwd_hw_t *hw, uint8_t reg, uint8_t *buf, uint16_t len)
{
    (void)hw; (void)reg; (void)buf; (void)len;
    uiox_tpwd_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_tpwd_i2c_write(uiox_tpwd_hw_t *hw, uint8_t reg, const uint8_t *buf, uint16_t len)
{
    (void)hw; (void)reg; (void)buf; (void)len;
    uiox_tpwd_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_tpwd_read_touch(uiox_tpwd_hw_t *hw, uiox_tpwd_raw_evt_t *evt)
{
    (void)hw; (void)evt;
    uiox_tpwd_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_tpwd_set_sensitivity(uiox_tpwd_hw_t *hw, uint8_t level)
{
    (void)hw; (void)level;
    uiox_tpwd_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_tpwd_set_backlight(uiox_tpwd_hw_t *hw, uint8_t level)
{
    (void)hw; (void)level;
    uiox_tpwd_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

/* delay_us: IRQ/void slot -- the firmware layer owns the IRQ controller
 * but does not route device interrupts into class handlers
 * (10_BSP/10_Arch/[*]/arch_runtime.c). */
static void fw_tpwd_delay_us(uiox_tpwd_hw_t *hw, uint32_t us) { (void)hw; (void)us; }

/* isr: IRQ/void slot -- the firmware layer owns the IRQ controller
 * but does not route device interrupts into class handlers
 * (10_BSP/10_Arch/[*]/arch_runtime.c). */
static void fw_tpwd_isr(uiox_tpwd_hw_t *hw) { (void)hw; }

static const uiox_tpwd_hw_ops_t s_tpwd_fwops = {
    .init           = fw_tpwd_init,
    .deinit         = fw_tpwd_deinit,
    .power          = fw_tpwd_power,
    .reset          = fw_tpwd_reset,
    .i2c_read       = fw_tpwd_i2c_read,
    .i2c_write      = fw_tpwd_i2c_write,
    .read_touch     = fw_tpwd_read_touch,
    .set_sensitivity= fw_tpwd_set_sensitivity,
    .set_backlight  = fw_tpwd_set_backlight,
    .delay_us       = fw_tpwd_delay_us,
    .isr            = fw_tpwd_isr,
};

/* ---- binding entry point ---- */
int uiox_tpwd_fwbind(uiox_tpwd_hw_t *hw, struct uiox_i2c_dev *i2c)
{
    if (!hw || !i2c) return -EINVAL;
    uiox_tpwd_fwbind_t *b = NULL;
    for (uint32_t i = 0u; i < UIOX_TPWD_FWBIND_MAX; i++)
        if (!s_tpwd_bind[i].in_use) { b = &s_tpwd_bind[i]; break; }
    if (!b) return -ENOSPC;
    b->in_use = true;
    b->i2c = *i2c;
    b->hw  = hw;
    uiox_fw_err_t frc = uiox_fw_fp_init_fpc1020(&b->fw, i2c, hw->rst_pin, hw->int_pin);
    if (frc != UIOX_FW_OK) { b->in_use = false; return uiox_fw_rc(frc); }
    int rc = uiox_tpwd_hw_init(hw, &s_tpwd_fwops);
    if (rc < 0) { b->in_use = false; return rc; }
    return 0;
}
