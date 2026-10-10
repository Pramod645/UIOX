/**
 * @file    uiox_tb4_fwops.c
 * @brief   Adapter: 02_FwHal implementation -> layer-20 uiox_tb4_hw_ops_t.
 * @date    2026-10-10
 *
 * PATH   30_KIX/34_DSS/20_DriverInterfaces/01_Com/tb4/uiox_tb4_fwops.c
 * LAYER  20_DriverInterfaces — hardware-control interface.
 *
 * BOUNDARY
 *   Above: 30_DeviceDrivers/uiox_tb4_if.c calls through hw->ops and never
 *          sees a firmware symbol.
 *   Below: 02_FwHal builds to libuioxfw<arch>.a; this file calls its flat
 *          uiox_fw_tb4_*() API and never includes its sources.
 *   This file is the joint: it includes the FwHal public header for the
 *   declarations, and the archive supplies the definitions at link time.
 *
 * GENERATED — every slot in uiox_tb4_hw_ops_t has an entry below.
 * Slots marked ENOSYS are the interrupt/GPIO entry points: the firmware
 * layer owns the IRQ controller but does not route device interrupts into
 * class handlers (that is 10_BSP/10_Arch/*/arch_runtime.c).  Reporting
 * unsupported is deliberate — silently accepting would swallow an IRQ
 * nothing services.
 */

#include "uiox_tb4_hw.h"
#include "uiox_fw_rc.h"
#include "uiox_fw_tb4.h"
#include "uiox_fw_i2c.h"

#define UIOX_TB4_FWBIND_MAX   2u

typedef struct {
    uiox_fw_tb4_dev_t  fw;
    uiox_i2c_dev_t                     i2c;
    uiox_tb4_hw_t                  *hw;
    bool                               in_use;
} uiox_tb4_fwbind_t;

static uiox_tb4_fwbind_t s_tb4_bind[UIOX_TB4_FWBIND_MAX];

static uiox_tb4_fwbind_t *bind_of(uiox_tb4_hw_t *hw)
{
    if (!hw || !hw->base) return NULL;
    uiox_tb4_fwbind_t *b = (uiox_tb4_fwbind_t *)hw->base;
    return (b->in_use && b->hw == hw) ? b : NULL;
}

/* ---- vtable slots ---- */

static int fw_tb4_init(uiox_tb4_hw_t *hw)
{
    uiox_tb4_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static void fw_tb4_deinit(uiox_tb4_hw_t *hw)
{
    uiox_tb4_fwbind_t *b = bind_of(hw);
    if (!b) return;
}

static int fw_tb4_power_on(uiox_tb4_hw_t *hw)
{
    uiox_tb4_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static void fw_tb4_power_off(uiox_tb4_hw_t *hw)
{
    uiox_tb4_fwbind_t *b = bind_of(hw);
    if (!b) return;
}

static int fw_tb4_nhi_read(uiox_tb4_hw_t *hw)
{
    uiox_tb4_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_tb4_nhi_write(uiox_tb4_hw_t *hw)
{
    uiox_tb4_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_tb4_cfg_read(uiox_tb4_hw_t *hw)
{
    uiox_tb4_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_tb4_cfg_write(uiox_tb4_hw_t *hw)
{
    uiox_tb4_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_tb4_icm_send(uiox_tb4_hw_t *hw)
{
    uiox_tb4_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_tb4_icm_recv(uiox_tb4_hw_t *hw)
{
    uiox_tb4_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_tb4_tx_submit(uiox_tb4_hw_t *hw)
{
    uiox_tb4_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_tb4_rx_poll(uiox_tb4_hw_t *hw)
{
    uiox_tb4_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static void fw_tb4_gpio_write(uiox_tb4_hw_t *hw, uint32_t pin, bool v)
{ (void)hw; (void)pin; (void)v; }

static bool fw_tb4_gpio_read(uiox_tb4_hw_t *hw, uint32_t pin)
{ (void)hw; (void)pin; return false; }

static void fw_tb4_isr_ring(uiox_tb4_hw_t *hw)
{ (void)hw; }

static void fw_tb4_isr_hotplug(uiox_tb4_hw_t *hw)
{ (void)hw; }

static void fw_tb4_isr_icm(uiox_tb4_hw_t *hw)
{ (void)hw; }

static void fw_tb4_isr_error(uiox_tb4_hw_t *hw)
{ (void)hw; }

static const uiox_tb4_hw_ops_t s_tb4_fwops = {
    .init          = fw_tb4_init,
    .deinit        = fw_tb4_deinit,
    .power_on      = fw_tb4_power_on,
    .power_off     = fw_tb4_power_off,
    .nhi_read      = fw_tb4_nhi_read,
    .nhi_write     = fw_tb4_nhi_write,
    .cfg_read      = fw_tb4_cfg_read,
    .cfg_write     = fw_tb4_cfg_write,
    .icm_send      = fw_tb4_icm_send,
    .icm_recv      = fw_tb4_icm_recv,
    .tx_submit     = fw_tb4_tx_submit,
    .rx_poll       = fw_tb4_rx_poll,
    .gpio_write    = fw_tb4_gpio_write,
    .gpio_read     = fw_tb4_gpio_read,
    .isr_ring      = fw_tb4_isr_ring,
    .isr_hotplug   = fw_tb4_isr_hotplug,
    .isr_icm       = fw_tb4_isr_icm,
    .isr_error     = fw_tb4_isr_error,
};

/* ---- binding entry points (one per 02_FwHal chip helper) ---- */
int uiox_tb4_fwbind   (uiox_tb4_hw_t *hw, uiox_i2c_dev_t *i2c)
{
    if (!hw || !i2c) return -EINVAL;
    uiox_tb4_fwbind_t *b = NULL;
    for (uint32_t i = 0u; i < UIOX_TB4_FWBIND_MAX; i++)
        if (!s_tb4_bind[i].in_use) { b = &s_tb4_bind[i]; break; }
    if (!b) return -ENOSPC;
    b->in_use = true;
    b->i2c = *i2c;
    b->hw  = hw;
    uiox_fw_err_t frc = uiox_fw_tb4_init(&b->fw, i2c);
    if (frc != UIOX_FW_OK) { b->in_use = false; return uiox_fw_rc(frc); }
    hw->base = (uintptr_t)b;
    int rc = uiox_tb4_hw_init(hw, &s_tb4_fwops);
    if (rc < 0) { b->in_use = false; return rc; }
    return 0;
}
