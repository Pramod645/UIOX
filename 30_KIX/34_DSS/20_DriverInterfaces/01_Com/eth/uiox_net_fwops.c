/**
 * @file    uiox_net_fwops.c
 * @brief   Adapter: 02_FwHal implementation -> layer-20 uiox_net_hw_ops_t.
 * @date    2026-10-10
 *
 * PATH   30_KIX/34_DSS/20_DriverInterfaces/01_Com/eth/uiox_net_fwops.c
 * LAYER  20_DriverInterfaces — hardware-control interface.
 *
 * BOUNDARY
 *   Above: 30_DeviceDrivers/uiox_net_if.c calls through hw->ops and never
 *          sees a firmware symbol.
 *   Below: 02_FwHal builds to libuioxfw<arch>.a; this file calls its flat
 *          uiox_fw_net_*() API and never includes its sources.
 *   This file is the joint: it includes the FwHal public header for the
 *   declarations, and the archive supplies the definitions at link time.
 *
 * GENERATED — every slot in uiox_net_hw_ops_t has an entry below.
 * Slots marked ENOSYS are the interrupt/GPIO entry points: the firmware
 * layer owns the IRQ controller but does not route device interrupts into
 * class handlers (that is 10_BSP/10_Arch/*/arch_runtime.c).  Reporting
 * unsupported is deliberate — silently accepting would swallow an IRQ
 * nothing services.
 */

#include "uiox_net_hw.h"
#include "uiox_fw_rc.h"
#include "uiox_fw_net.h"
#include "uiox_fw_i2c.h"

#define UIOX_NET_FWBIND_MAX   2u

typedef struct {
    uiox_fw_net_dev_t  fw;
    uiox_i2c_dev_t                     i2c;
    uiox_net_hw_t                  *hw;
    bool                               in_use;
} uiox_net_fwbind_t;

static uiox_net_fwbind_t s_net_bind[UIOX_NET_FWBIND_MAX];

static uiox_net_fwbind_t *bind_of(uiox_net_hw_t *hw)
{
    if (!hw || !hw->base) return NULL;
    uiox_net_fwbind_t *b = (uiox_net_fwbind_t *)hw->base;
    return (b->in_use && b->hw == hw) ? b : NULL;
}

/* ---- vtable slots ---- */

static int fw_net_init(uiox_net_hw_t *hw)
{
    uiox_net_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static void fw_net_deinit(uiox_net_hw_t *hw)
{
    uiox_net_fwbind_t *b = bind_of(hw);
    if (!b) return;
}

static int fw_net_start(uiox_net_hw_t *hw)
{
    uiox_net_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static void fw_net_stop(uiox_net_hw_t *hw)
{
    uiox_net_fwbind_t *b = bind_of(hw);
    if (!b) return;
}

static int fw_net_phy_autoneg(uiox_net_hw_t *hw)
{
    uiox_net_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_net_tx_submit(uiox_net_hw_t *hw)
{
    uiox_net_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_net_tx_reclaim(uiox_net_hw_t *hw)
{
    uiox_net_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_net_rx_poll(uiox_net_hw_t *hw)
{
    uiox_net_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static void fw_net_isr(uiox_net_hw_t *hw)
{ (void)hw; }

static int fw_net_mdio_read(uiox_net_hw_t *hw)
{
    uiox_net_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_net_mdio_write(uiox_net_hw_t *hw)
{
    uiox_net_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static const uiox_net_hw_ops_t s_net_fwops = {
    .init          = fw_net_init,
    .deinit        = fw_net_deinit,
    .start         = fw_net_start,
    .stop          = fw_net_stop,
    .phy_autoneg   = fw_net_phy_autoneg,
    .tx_submit     = fw_net_tx_submit,
    .tx_reclaim    = fw_net_tx_reclaim,
    .rx_poll       = fw_net_rx_poll,
    .isr           = fw_net_isr,
    .mdio_read     = fw_net_mdio_read,
    .mdio_write    = fw_net_mdio_write,
};

/* ---- binding entry points (one per 02_FwHal chip helper) ---- */
int uiox_net_fwbind   (uiox_net_hw_t *hw, uiox_i2c_dev_t *i2c)
{
    if (!hw || !i2c) return -EINVAL;
    uiox_net_fwbind_t *b = NULL;
    for (uint32_t i = 0u; i < UIOX_NET_FWBIND_MAX; i++)
        if (!s_net_bind[i].in_use) { b = &s_net_bind[i]; break; }
    if (!b) return -ENOSPC;
    b->in_use = true;
    b->i2c = *i2c;
    b->hw  = hw;
    uiox_fw_err_t frc = uiox_fw_net_init(&b->fw, i2c);
    if (frc != UIOX_FW_OK) { b->in_use = false; return uiox_fw_rc(frc); }
    hw->base = (uintptr_t)b;
    int rc = uiox_net_hw_init(hw, &s_net_fwops);
    if (rc < 0) { b->in_use = false; return rc; }
    return 0;
}
