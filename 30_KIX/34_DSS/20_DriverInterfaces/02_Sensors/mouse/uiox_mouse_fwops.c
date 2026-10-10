/**
 * @file    uiox_mouse_fwops.c
 * @brief   Adapter: 02_FwHal implementation -> layer-20 uiox_mouse_hw_ops_t.
 * @date    2026-10-10
 *
 * PATH   30_KIX/34_DSS/20_DriverInterfaces/02_Sensors/mouse/uiox_mouse_fwops.c
 * LAYER  20_DriverInterfaces — hardware-control interface.
 *
 * BOUNDARY
 *   Above: 30_DeviceDrivers/uiox_mouse_if.c calls through hw->ops and never
 *          sees a firmware symbol.
 *   Below: 02_FwHal builds to libuioxfw<arch>.a; this file calls its flat
 *          uiox_fw_mouse_*() API and never includes its sources.
 *   This file is the joint: it includes the FwHal public header for the
 *   declarations, and the archive supplies the definitions at link time.
 *
 * GENERATED — every slot in uiox_mouse_hw_ops_t has an entry below.
 * Slots marked ENOSYS are the interrupt/GPIO entry points: the firmware
 * layer owns the IRQ controller but does not route device interrupts into
 * class handlers (that is 10_BSP/10_Arch/*/arch_runtime.c).  Reporting
 * unsupported is deliberate — silently accepting would swallow an IRQ
 * nothing services.
 */

#include "uiox_mouse_hw.h"
#include "uiox_fw_rc.h"
#include "uiox_fw_mouse.h"
#include "uiox_fw_i2c.h"

#define UIOX_MOUSE_FWBIND_MAX   2u

typedef struct {
    uiox_fw_mouse_dev_t  fw;
    uiox_i2c_dev_t                     i2c;
    uiox_mouse_hw_t                  *hw;
    bool                               in_use;
} uiox_mouse_fwbind_t;

static uiox_mouse_fwbind_t s_mouse_bind[UIOX_MOUSE_FWBIND_MAX];

static uiox_mouse_fwbind_t *bind_of(uiox_mouse_hw_t *hw)
{
    if (!hw || !hw->base) return NULL;
    uiox_mouse_fwbind_t *b = (uiox_mouse_fwbind_t *)hw->base;
    return (b->in_use && b->hw == hw) ? b : NULL;
}

/* ---- vtable slots ---- */

static int fw_mouse_init(uiox_mouse_hw_t *hw)
{
    uiox_mouse_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static void fw_mouse_deinit(uiox_mouse_hw_t *hw)
{
    uiox_mouse_fwbind_t *b = bind_of(hw);
    if (!b) return;
}

static int fw_mouse_enable(uiox_mouse_hw_t *hw)
{
    uiox_mouse_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static void fw_mouse_disable(uiox_mouse_hw_t *hw)
{
    uiox_mouse_fwbind_t *b = bind_of(hw);
    if (!b) return;
}

static int fw_mouse_read_report(uiox_mouse_hw_t *hw)
{
    uiox_mouse_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_mouse_set_rate(uiox_mouse_hw_t *hw)
{
    uiox_mouse_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_mouse_set_dpi(uiox_mouse_hw_t *hw)
{
    uiox_mouse_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_mouse_i2c_read(uiox_mouse_hw_t *hw)
{
    uiox_mouse_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_mouse_i2c_write(uiox_mouse_hw_t *hw)
{
    uiox_mouse_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_mouse_connected(uiox_mouse_hw_t *hw)
{
    uiox_mouse_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static void fw_mouse_isr(uiox_mouse_hw_t *hw)
{ (void)hw; }

static const uiox_mouse_hw_ops_t s_mouse_fwops = {
    .init          = fw_mouse_init,
    .deinit        = fw_mouse_deinit,
    .enable        = fw_mouse_enable,
    .disable       = fw_mouse_disable,
    .read_report   = fw_mouse_read_report,
    .set_rate      = fw_mouse_set_rate,
    .set_dpi       = fw_mouse_set_dpi,
    .i2c_read      = fw_mouse_i2c_read,
    .i2c_write     = fw_mouse_i2c_write,
    .connected     = fw_mouse_connected,
    .isr           = fw_mouse_isr,
};

/* ---- binding entry points (one per 02_FwHal chip helper) ---- */
int uiox_mouse_fwbind   (uiox_mouse_hw_t *hw, uiox_i2c_dev_t *i2c)
{
    if (!hw || !i2c) return -EINVAL;
    uiox_mouse_fwbind_t *b = NULL;
    for (uint32_t i = 0u; i < UIOX_MOUSE_FWBIND_MAX; i++)
        if (!s_mouse_bind[i].in_use) { b = &s_mouse_bind[i]; break; }
    if (!b) return -ENOSPC;
    b->in_use = true;
    b->i2c = *i2c;
    b->hw  = hw;
    uiox_fw_err_t frc = uiox_fw_mouse_init(&b->fw, i2c);
    if (frc != UIOX_FW_OK) { b->in_use = false; return uiox_fw_rc(frc); }
    hw->base = (uintptr_t)b;
    int rc = uiox_mouse_hw_init(hw, &s_mouse_fwops);
    if (rc < 0) { b->in_use = false; return rc; }
    return 0;
}
