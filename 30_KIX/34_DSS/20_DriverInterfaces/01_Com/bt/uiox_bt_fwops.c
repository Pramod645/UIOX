/**
 * @file    uiox_bt_fwops.c
 * @brief   Adapter: 02_FwHal implementation -> layer-20 uiox_bt_hw_ops_t.
 * @date    2026-10-10
 *
 * PATH   30_KIX/34_DSS/20_DriverInterfaces/01_Com/bt/uiox_bt_fwops.c
 * LAYER  20_DriverInterfaces — hardware-control interface.
 *
 * BOUNDARY
 *   Above: 30_DeviceDrivers/uiox_bt_if.c calls through hw->ops and never
 *          sees a firmware symbol.
 *   Below: 02_FwHal builds to libuioxfw<arch>.a; this file calls its flat
 *          uiox_fw_bt_*() API and never includes its sources.
 *   This file is the joint: it includes the FwHal public header for the
 *   declarations, and the archive supplies the definitions at link time.
 *
 * GENERATED — every slot in uiox_bt_hw_ops_t has an entry below.
 * Slots marked ENOSYS are the interrupt/GPIO entry points: the firmware
 * layer owns the IRQ controller but does not route device interrupts into
 * class handlers (that is 10_BSP/10_Arch/*/arch_runtime.c).  Reporting
 * unsupported is deliberate — silently accepting would swallow an IRQ
 * nothing services.
 */

#include "uiox_bt_hw.h"
#include "uiox_fw_rc.h"
#include "uiox_fw_bt.h"
#include "uiox_fw_i2c.h"

#define UIOX_BT_FWBIND_MAX   2u

typedef struct {
    uiox_fw_bt_dev_t  fw;
    uiox_i2c_dev_t                     i2c;
    uiox_bt_hw_t                  *hw;
    bool                               in_use;
} uiox_bt_fwbind_t;

static uiox_bt_fwbind_t s_bt_bind[UIOX_BT_FWBIND_MAX];

static uiox_bt_fwbind_t *bind_of(uiox_bt_hw_t *hw)
{
    if (!hw || !hw->base) return NULL;
    uiox_bt_fwbind_t *b = (uiox_bt_fwbind_t *)hw->base;
    return (b->in_use && b->hw == hw) ? b : NULL;
}

/* ---- vtable slots ---- */

static int fw_bt_init(uiox_bt_hw_t *hw)
{
    uiox_bt_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static void fw_bt_deinit(uiox_bt_hw_t *hw)
{
    uiox_bt_fwbind_t *b = bind_of(hw);
    if (!b) return;
}

static int fw_bt_power(uiox_bt_hw_t *hw)
{
    uiox_bt_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_bt_fw_download(uiox_bt_hw_t *hw)
{
    uiox_bt_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_bt_hci_write(uiox_bt_hw_t *hw)
{
    uiox_bt_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_bt_hci_read(uiox_bt_hw_t *hw)
{
    uiox_bt_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_bt_set_baud(uiox_bt_hw_t *hw)
{
    uiox_bt_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static void fw_bt_gpio_write(uiox_bt_hw_t *hw, uint32_t pin, bool v)
{ (void)hw; (void)pin; (void)v; }

static bool fw_bt_gpio_read(uiox_bt_hw_t *hw, uint32_t pin)
{ (void)hw; (void)pin; return false; }

static int fw_bt_delay_ms(uiox_bt_hw_t *hw)
{
    uiox_bt_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static void fw_bt_isr_host_wake(uiox_bt_hw_t *hw)
{ (void)hw; }

static void fw_bt_isr_rx(uiox_bt_hw_t *hw)
{ (void)hw; }

static const uiox_bt_hw_ops_t s_bt_fwops = {
    .init          = fw_bt_init,
    .deinit        = fw_bt_deinit,
    .power         = fw_bt_power,
    .fw_download   = fw_bt_fw_download,
    .hci_write     = fw_bt_hci_write,
    .hci_read      = fw_bt_hci_read,
    .set_baud      = fw_bt_set_baud,
    .gpio_write    = fw_bt_gpio_write,
    .gpio_read     = fw_bt_gpio_read,
    .delay_ms      = fw_bt_delay_ms,
    .isr_host_wake = fw_bt_isr_host_wake,
    .isr_rx        = fw_bt_isr_rx,
};

/* ---- binding entry points (one per 02_FwHal chip helper) ---- */
int uiox_bt_fwbind   (uiox_bt_hw_t *hw, uiox_i2c_dev_t *i2c)
{
    if (!hw || !i2c) return -EINVAL;
    uiox_bt_fwbind_t *b = NULL;
    for (uint32_t i = 0u; i < UIOX_BT_FWBIND_MAX; i++)
        if (!s_bt_bind[i].in_use) { b = &s_bt_bind[i]; break; }
    if (!b) return -ENOSPC;
    b->in_use = true;
    b->i2c = *i2c;
    b->hw  = hw;
    uiox_fw_err_t frc = uiox_fw_bt_init(&b->fw, i2c);
    if (frc != UIOX_FW_OK) { b->in_use = false; return uiox_fw_rc(frc); }
    hw->base = (uintptr_t)b;
    int rc = uiox_bt_hw_init(hw, &s_bt_fwops);
    if (rc < 0) { b->in_use = false; return rc; }
    return 0;
}
