/**
 * @file    uiox_usb_fwops.c
 * @brief   Adapter: 02_FwHal implementation -> layer-20 uiox_usb_hw_ops_t.
 * @date    2026-10-10
 *
 * PATH   30_KIX/34_DSS/20_DriverInterfaces/01_Com/usb/uiox_usb_fwops.c
 * LAYER  20_DriverInterfaces — hardware-control interface.
 *
 * BOUNDARY
 *   Above: 30_DeviceDrivers/uiox_usb_if.c calls through hw->ops and never
 *          sees a firmware symbol.
 *   Below: 02_FwHal builds to libuioxfw<arch>.a; this file calls its flat
 *          uiox_fw_usb_*() API and never includes its sources.
 *   This file is the joint: it includes the FwHal public header for the
 *   declarations, and the archive supplies the definitions at link time.
 *
 * GENERATED — every slot in uiox_usb_hw_ops_t has an entry below.
 * Slots marked ENOSYS are the interrupt/GPIO entry points: the firmware
 * layer owns the IRQ controller but does not route device interrupts into
 * class handlers (that is 10_BSP/10_Arch/*/arch_runtime.c).  Reporting
 * unsupported is deliberate — silently accepting would swallow an IRQ
 * nothing services.
 */

#include "uiox_usb_hw.h"
#include "uiox_fw_rc.h"
#include "uiox_fw_usb.h"
#include "uiox_fw_i2c.h"

#define UIOX_USB_FWBIND_MAX   2u

typedef struct {
    uiox_fw_usb_dev_t  fw;
    uiox_i2c_dev_t                     i2c;
    uiox_usb_hw_t                  *hw;
    bool                               in_use;
} uiox_usb_fwbind_t;

static uiox_usb_fwbind_t s_usb_bind[UIOX_USB_FWBIND_MAX];

static uiox_usb_fwbind_t *bind_of(uiox_usb_hw_t *hw)
{
    if (!hw || !hw->base) return NULL;
    uiox_usb_fwbind_t *b = (uiox_usb_fwbind_t *)hw->base;
    return (b->in_use && b->hw == hw) ? b : NULL;
}

/* ---- vtable slots ---- */

static int fw_usb_init(uiox_usb_hw_t *hw)
{
    uiox_usb_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static void fw_usb_deinit(uiox_usb_hw_t *hw)
{
    uiox_usb_fwbind_t *b = bind_of(hw);
    if (!b) return;
}

static int fw_usb_start(uiox_usb_hw_t *hw)
{
    uiox_usb_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static void fw_usb_stop(uiox_usb_hw_t *hw)
{
    uiox_usb_fwbind_t *b = bind_of(hw);
    if (!b) return;
}

static int fw_usb_set_role(uiox_usb_hw_t *hw)
{
    uiox_usb_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_usb_set_address(uiox_usb_hw_t *hw)
{
    uiox_usb_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_usb_ep_config(uiox_usb_hw_t *hw)
{
    uiox_usb_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_usb_ep_disable(uiox_usb_hw_t *hw)
{
    uiox_usb_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_usb_ep_stall(uiox_usb_hw_t *hw)
{
    uiox_usb_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_usb_ep_flush(uiox_usb_hw_t *hw)
{
    uiox_usb_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_usb_tx_submit(uiox_usb_hw_t *hw)
{
    uiox_usb_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_usb_rx_submit(uiox_usb_hw_t *hw)
{
    uiox_usb_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_usb_tx_complete(uiox_usb_hw_t *hw)
{
    uiox_usb_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_usb_rx_complete(uiox_usb_hw_t *hw)
{
    uiox_usb_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_usb_vbus_sense(uiox_usb_hw_t *hw)
{
    uiox_usb_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_usb_remote_wakeup(uiox_usb_hw_t *hw)
{
    uiox_usb_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static void fw_usb_isr_sof(uiox_usb_hw_t *hw)
{ (void)hw; }

static void fw_usb_isr_reset(uiox_usb_hw_t *hw)
{ (void)hw; }

static void fw_usb_isr_suspend(uiox_usb_hw_t *hw)
{ (void)hw; }

static void fw_usb_isr_resume(uiox_usb_hw_t *hw)
{ (void)hw; }

static void fw_usb_isr_ep(uiox_usb_hw_t *hw)
{ (void)hw; }

static void fw_usb_isr_connect(uiox_usb_hw_t *hw)
{ (void)hw; }

static void fw_usb_isr_disconnect(uiox_usb_hw_t *hw)
{ (void)hw; }

static const uiox_usb_hw_ops_t s_usb_fwops = {
    .init          = fw_usb_init,
    .deinit        = fw_usb_deinit,
    .start         = fw_usb_start,
    .stop          = fw_usb_stop,
    .set_role      = fw_usb_set_role,
    .set_address   = fw_usb_set_address,
    .ep_config     = fw_usb_ep_config,
    .ep_disable    = fw_usb_ep_disable,
    .ep_stall      = fw_usb_ep_stall,
    .ep_flush      = fw_usb_ep_flush,
    .tx_submit     = fw_usb_tx_submit,
    .rx_submit     = fw_usb_rx_submit,
    .tx_complete   = fw_usb_tx_complete,
    .rx_complete   = fw_usb_rx_complete,
    .vbus_sense    = fw_usb_vbus_sense,
    .remote_wakeup = fw_usb_remote_wakeup,
    .isr_sof       = fw_usb_isr_sof,
    .isr_reset     = fw_usb_isr_reset,
    .isr_suspend   = fw_usb_isr_suspend,
    .isr_resume    = fw_usb_isr_resume,
    .isr_ep        = fw_usb_isr_ep,
    .isr_connect   = fw_usb_isr_connect,
    .isr_disconnect= fw_usb_isr_disconnect,
};

/* ---- binding entry points (one per 02_FwHal chip helper) ---- */
int uiox_usb_fwbind   (uiox_usb_hw_t *hw, uiox_i2c_dev_t *i2c)
{
    if (!hw || !i2c) return -EINVAL;
    uiox_usb_fwbind_t *b = NULL;
    for (uint32_t i = 0u; i < UIOX_USB_FWBIND_MAX; i++)
        if (!s_usb_bind[i].in_use) { b = &s_usb_bind[i]; break; }
    if (!b) return -ENOSPC;
    b->in_use = true;
    b->i2c = *i2c;
    b->hw  = hw;
    uiox_fw_err_t frc = uiox_fw_usb_init(&b->fw, i2c);
    if (frc != UIOX_FW_OK) { b->in_use = false; return uiox_fw_rc(frc); }
    hw->base = (uintptr_t)b;
    int rc = uiox_usb_hw_init(hw, &s_usb_fwops);
    if (rc < 0) { b->in_use = false; return rc; }
    return 0;
}
