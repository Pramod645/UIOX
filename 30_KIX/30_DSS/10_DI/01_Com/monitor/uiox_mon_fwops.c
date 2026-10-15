/**
 * @file    uiox_mon_fwops.c
 * @brief   Adapter: 02_FwHal implementation -> layer-20 uiox_mon_hw_ops_t.
 *
 * PATH   30_KIX/34_DSS/20_DriverInterfaces/01_Com/monitor/uiox_mon_fwops.c
 * LAYER  20_DriverInterfaces -- hardware-control interface.
 *
 * BOUNDARY
 *   Above: 30_DeviceDrivers/uiox_mon_if.c calls through hw->ops and never
 *          sees a firmware symbol.
 *   Below: 02_FwHal builds to libuiox_fw_<arch>.a; this file calls its flat
 *          uiox_fw_hdmi_*() API and never includes its sources.
 *   This file is the joint.
 *
 * REGENERATED -- every stub carries the exact signature of its vtable slot
 * (15 slots).  Bodies return -ENOSYS: an honest "not yet
 * bound", never a silent success.  Every slot parameter is explicitly cast
 * to void so the strict -Werror set stays intact for the driver units that
 * do carry logic.
 */

#include "uiox_mon_hw.h"
#include "uiox_fw_rc.h"
#include "uiox_fw_hdmi.h"
#include "uiox_fw_i2c.h"

#define UIOX_MON_FWBIND_MAX   2u

typedef struct {
    uiox_mon_dev_t  fw;
    uiox_i2c_dev_t     i2c;
    uiox_mon_hw_t             *hw;
    bool                in_use;
} uiox_mon_fwbind_t;

static uiox_mon_fwbind_t s_mon_bind[UIOX_MON_FWBIND_MAX];

static uiox_mon_fwbind_t *bind_of(uiox_mon_hw_t *hw)
{
    if (!hw) return NULL;
    for (uint32_t i = 0u; i < UIOX_MON_FWBIND_MAX; i++)
        if (s_mon_bind[i].in_use && s_mon_bind[i].hw == hw)
            return &s_mon_bind[i];
    return NULL;
}

/* ---- vtable slots ---- */
static int fw_mon_init(uiox_mon_hw_t *hw)
{
    (void)hw;
    uiox_mon_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

/* deinit: IRQ/void slot -- the firmware layer owns the IRQ controller
 * but does not route device interrupts into class handlers
 * (10_BSP/10_Arch/[*]/arch_runtime.c). */
static void fw_mon_deinit(uiox_mon_hw_t *hw) { (void)hw; }
static int fw_mon_enable(uiox_mon_hw_t *hw)
{
    (void)hw;
    uiox_mon_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

/* disable: IRQ/void slot -- the firmware layer owns the IRQ controller
 * but does not route device interrupts into class handlers
 * (10_BSP/10_Arch/[*]/arch_runtime.c). */
static void fw_mon_disable(uiox_mon_hw_t *hw) { (void)hw; }
static int fw_mon_set_timing(uiox_mon_hw_t *hw, const uiox_mon_timing_t *t)
{
    (void)hw; (void)t;
    uiox_mon_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_mon_set_pixfmt(uiox_mon_hw_t *hw, uiox_mon_pixfmt_t fmt)
{
    (void)hw; (void)fmt;
    uiox_mon_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_mon_flip(uiox_mon_hw_t *hw, uintptr_t phys, uint32_t stride)
{
    (void)hw; (void)phys; (void)stride;
    uiox_mon_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_mon_wait_vblank(uiox_mon_hw_t *hw, uint32_t timeout_ms)
{
    (void)hw; (void)timeout_ms;
    uiox_mon_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_mon_read_edid(uiox_mon_hw_t *hw, uint8_t *buf)
{
    (void)hw; (void)buf;
    uiox_mon_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_mon_set_dpms(uiox_mon_hw_t *hw, uiox_mon_dpms_t dpms)
{
    (void)hw; (void)dpms;
    uiox_mon_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_mon_set_backlight(uiox_mon_hw_t *hw, uint8_t level)
{
    (void)hw; (void)level;
    uiox_mon_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_mon_set_gamma(uiox_mon_hw_t *hw, const uint16_t *r, const uint16_t *g, const uint16_t *b)
{
    (void)hw; (void)r; (void)g; (void)b;
    uiox_mon_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static bool fw_mon_hotplug_state(uiox_mon_hw_t *hw)
{
    (void)hw;
    uiox_mon_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

/* isr_vblank: IRQ/void slot -- the firmware layer owns the IRQ controller
 * but does not route device interrupts into class handlers
 * (10_BSP/10_Arch/[*]/arch_runtime.c). */
static void fw_mon_isr_vblank(uiox_mon_hw_t *hw) { (void)hw; }

/* isr_hotplug: IRQ/void slot -- the firmware layer owns the IRQ controller
 * but does not route device interrupts into class handlers
 * (10_BSP/10_Arch/[*]/arch_runtime.c). */
static void fw_mon_isr_hotplug(uiox_mon_hw_t *hw) { (void)hw; }

static const uiox_mon_hw_ops_t s_mon_fwops = {
    .init           = fw_mon_init,
    .deinit         = fw_mon_deinit,
    .enable         = fw_mon_enable,
    .disable        = fw_mon_disable,
    .set_timing     = fw_mon_set_timing,
    .set_pixfmt     = fw_mon_set_pixfmt,
    .flip           = fw_mon_flip,
    .wait_vblank    = fw_mon_wait_vblank,
    .read_edid      = fw_mon_read_edid,
    .set_dpms       = fw_mon_set_dpms,
    .set_backlight  = fw_mon_set_backlight,
    .set_gamma      = fw_mon_set_gamma,
    .hotplug_state  = fw_mon_hotplug_state,
    .isr_vblank     = fw_mon_isr_vblank,
    .isr_hotplug    = fw_mon_isr_hotplug,
};

/* ---- binding entry point ---- */
int uiox_mon_fwbind(uiox_mon_hw_t *hw, struct uiox_i2c_dev *i2c)
{
    if (!hw || !i2c) return -EINVAL;
    uiox_mon_fwbind_t *b = NULL;
    for (uint32_t i = 0u; i < UIOX_MON_FWBIND_MAX; i++)
        if (!s_mon_bind[i].in_use) { b = &s_mon_bind[i]; break; }
    if (!b) return -ENOSPC;
    b->in_use = true;
    b->i2c = *i2c;
    b->hw  = hw;
    uiox_fw_err_t frc = uiox_fw_hdmi_init(&b->fw, i2c);
    if (frc != UIOX_FW_OK) { b->in_use = false; return uiox_fw_rc(frc); }
    int rc = uiox_mon_hw_init(hw, &s_mon_fwops);
    if (rc < 0) { b->in_use = false; return rc; }
    return 0;
}
