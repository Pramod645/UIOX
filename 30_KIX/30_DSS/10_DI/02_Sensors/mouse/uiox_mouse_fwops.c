/**
 * @file    uiox_mouse_fwops.c
 * @brief   Adapter: 02_FwHal implementation -> layer-20 uiox_mouse_hw_ops_t.
 *
 * PATH   30_KIX/30_DSS/10_DI/02_Sensors/mouse/uiox_mouse_fwops.c
 * LAYER  10_DI -- hardware-control interface.
 *
 * BOUNDARY
 *   Above: 20_DD/uiox_mouse_if.c calls through hw->ops and never
 *          sees a firmware symbol.
 *   Below: 02_FwHal builds to libuiox_fw_<arch>.a; this file calls its flat
 *          uiox_fw_mouse_*() API and never includes its sources.
 *   This file is the joint.
 *
 * REGENERATED -- every stub carries the exact signature of its vtable slot
 * (11 slots).  Bodies return -ENOSYS: an honest "not yet
 * bound", never a silent success.  Every slot parameter is explicitly cast
 * to void so the strict -Werror set stays intact for the driver units that
 * do carry logic.
 */

#include "uiox_mouse_hw.h"
#include "uiox_fw_rc.h"
#include "uiox_fw_mouse.h"
#include "uiox_fw_i2c.h"

#define UIOX_MOUSE_FWBIND_MAX   2u

typedef struct {
    uiox_mouse_dev_t  fw;
    uiox_i2c_dev_t     i2c;
    uiox_mouse_hw_t             *hw;
    bool                in_use;
} uiox_mouse_fwbind_t;

static uiox_mouse_fwbind_t s_mouse_bind[UIOX_MOUSE_FWBIND_MAX];

static uiox_mouse_fwbind_t *bind_of(uiox_mouse_hw_t *hw)
{
    if (!hw) return NULL;
    for (uint32_t i = 0u; i < UIOX_MOUSE_FWBIND_MAX; i++)
        if (s_mouse_bind[i].in_use && s_mouse_bind[i].hw == hw)
            return &s_mouse_bind[i];
    return NULL;
}

/* ---- vtable slots ---- */
static int fw_mouse_init(uiox_mouse_hw_t *hw)
{
    (void)hw;
    uiox_mouse_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

/* deinit: IRQ/void slot -- the firmware layer owns the IRQ controller
 * but does not route device interrupts into class handlers
 * (10_BSP/10_Arch/[*]/arch_runtime.c). */
static void fw_mouse_deinit(uiox_mouse_hw_t *hw) { (void)hw; }
static int fw_mouse_enable(uiox_mouse_hw_t *hw)
{
    (void)hw;
    uiox_mouse_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

/* disable: IRQ/void slot -- the firmware layer owns the IRQ controller
 * but does not route device interrupts into class handlers
 * (10_BSP/10_Arch/[*]/arch_runtime.c). */
static void fw_mouse_disable(uiox_mouse_hw_t *hw) { (void)hw; }
static int fw_mouse_read_report(uiox_mouse_hw_t *hw, uiox_mouse_raw_t *raw)
{
    (void)hw; (void)raw;
    uiox_mouse_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_mouse_set_rate(uiox_mouse_hw_t *hw, uint32_t rate_hz)
{
    (void)hw; (void)rate_hz;
    uiox_mouse_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_mouse_set_dpi(uiox_mouse_hw_t *hw, uint16_t dpi)
{
    (void)hw; (void)dpi;
    uiox_mouse_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_mouse_i2c_read(uiox_mouse_hw_t *hw, uint8_t reg, uint8_t *buf, uint16_t len)
{
    (void)hw; (void)reg; (void)buf; (void)len;
    uiox_mouse_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_mouse_i2c_write(uiox_mouse_hw_t *hw, uint8_t reg, const uint8_t *buf, uint16_t len)
{
    (void)hw; (void)reg; (void)buf; (void)len;
    uiox_mouse_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static bool fw_mouse_connected(uiox_mouse_hw_t *hw)
{
    (void)hw;
    uiox_mouse_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

/* isr: IRQ/void slot -- the firmware layer owns the IRQ controller
 * but does not route device interrupts into class handlers
 * (10_BSP/10_Arch/[*]/arch_runtime.c). */
static void fw_mouse_isr(uiox_mouse_hw_t *hw) { (void)hw; }

static const uiox_mouse_hw_ops_t s_mouse_fwops = {
    .init           = fw_mouse_init,
    .deinit         = fw_mouse_deinit,
    .enable         = fw_mouse_enable,
    .disable        = fw_mouse_disable,
    .read_report    = fw_mouse_read_report,
    .set_rate       = fw_mouse_set_rate,
    .set_dpi        = fw_mouse_set_dpi,
    .i2c_read       = fw_mouse_i2c_read,
    .i2c_write      = fw_mouse_i2c_write,
    .connected      = fw_mouse_connected,
    .isr            = fw_mouse_isr,
};

/* ---- binding entry point ---- */
int uiox_mouse_fwbind(uiox_mouse_hw_t *hw, struct uiox_i2c_dev *i2c)
{
    if (!hw || !i2c) return -EINVAL;
    uiox_mouse_fwbind_t *b = NULL;
    for (uint32_t i = 0u; i < UIOX_MOUSE_FWBIND_MAX; i++)
        if (!s_mouse_bind[i].in_use) { b = &s_mouse_bind[i]; break; }
    if (!b) return -ENOSPC;
    b->in_use = true;
    b->i2c = *i2c;
    b->hw  = hw;
    uiox_fw_err_t frc = uiox_fw_mouse_init_ps2(&b->fw, hw->irq);
    if (frc != UIOX_FW_OK) { b->in_use = false; return uiox_fw_rc(frc); }
    int rc = uiox_mouse_hw_init(hw, &s_mouse_fwops);
    if (rc < 0) { b->in_use = false; return rc; }
    return 0;
}
