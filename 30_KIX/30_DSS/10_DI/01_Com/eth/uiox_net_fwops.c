/**
 * @file    uiox_net_fwops.c
 * @brief   Adapter: 02_FwHal implementation -> layer-20 uiox_net_hw_ops_t.
 *
 * PATH   30_KIX/30_DSS/10_DI/01_Com/eth/uiox_net_fwops.c
 * LAYER  10_DI -- hardware-control interface.
 *
 * BOUNDARY
 *   Above: 20_DD/uiox_net_if.c calls through hw->ops and never
 *          sees a firmware symbol.
 *   Below: 02_FwHal builds to libuiox_fw_<arch>.a; this file calls its flat
 *          uiox_fw_eth_*() API and never includes its sources.
 *   This file is the joint.
 *
 * REGENERATED -- every stub carries the exact signature of its vtable slot
 * (11 slots).  Bodies return -ENOSYS: an honest "not yet
 * bound", never a silent success.  Every slot parameter is explicitly cast
 * to void so the strict -Werror set stays intact for the driver units that
 * do carry logic.
 */

#include "uiox_net_hw.h"
#include "uiox_fw_rc.h"
#include "uiox_fw_eth.h"
#include "uiox_fw_i2c.h"

#define UIOX_NET_FWBIND_MAX   2u

typedef struct {
    uiox_eth_dev_t  fw;
    uiox_i2c_dev_t     i2c;
    uiox_net_dev_t             *hw;
    bool                in_use;
} uiox_net_fwbind_t;

static uiox_net_fwbind_t s_net_bind[UIOX_NET_FWBIND_MAX];

static uiox_net_fwbind_t *bind_of(uiox_net_dev_t *hw)
{
    if (!hw) return NULL;
    for (uint32_t i = 0u; i < UIOX_NET_FWBIND_MAX; i++)
        if (s_net_bind[i].in_use && s_net_bind[i].hw == hw)
            return &s_net_bind[i];
    return NULL;
}

/* ---- vtable slots ---- */
static int fw_net_init(uiox_net_dev_t *hw)
{
    (void)hw;
    uiox_net_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

/* deinit: IRQ/void slot -- the firmware layer owns the IRQ controller
 * but does not route device interrupts into class handlers
 * (10_BSP/10_Arch/[*]/arch_runtime.c). */
static void fw_net_deinit(uiox_net_dev_t *hw) { (void)hw; }
static int fw_net_start(uiox_net_dev_t *hw)
{
    (void)hw;
    uiox_net_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

/* stop: IRQ/void slot -- the firmware layer owns the IRQ controller
 * but does not route device interrupts into class handlers
 * (10_BSP/10_Arch/[*]/arch_runtime.c). */
static void fw_net_stop(uiox_net_dev_t *hw) { (void)hw; }
static int fw_net_phy_autoneg(uiox_net_dev_t *hw)
{
    (void)hw;
    uiox_net_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_net_tx_submit(uiox_net_dev_t *hw, uintptr_t buf, uint16_t len)
{
    (void)hw; (void)buf; (void)len;
    uiox_net_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

/* tx_reclaim: IRQ/void slot -- the firmware layer owns the IRQ controller
 * but does not route device interrupts into class handlers
 * (10_BSP/10_Arch/[*]/arch_runtime.c). */
static void fw_net_tx_reclaim(uiox_net_dev_t *hw) { (void)hw; }
static int fw_net_rx_poll(uiox_net_dev_t *hw, void *buf, uint16_t maxlen)
{
    (void)hw; (void)buf; (void)maxlen;
    uiox_net_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

/* isr: IRQ/void slot -- the firmware layer owns the IRQ controller
 * but does not route device interrupts into class handlers
 * (10_BSP/10_Arch/[*]/arch_runtime.c). */
static void fw_net_isr(uiox_net_dev_t *hw) { (void)hw; }
static uint16_t fw_net_mdio_read(uiox_net_dev_t *hw, uint8_t phy, uint8_t reg)
{
    (void)hw; (void)phy; (void)reg;
    uiox_net_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

/* mdio_write: IRQ/void slot -- the firmware layer owns the IRQ controller
 * but does not route device interrupts into class handlers
 * (10_BSP/10_Arch/[*]/arch_runtime.c). */
static void fw_net_mdio_write(uiox_net_dev_t *hw, uint8_t phy, uint8_t reg, uint16_t val) { (void)hw; (void)phy; (void)reg; (void)val; }

static const uiox_net_hw_ops_t s_net_fwops = {
    .init           = fw_net_init,
    .deinit         = fw_net_deinit,
    .start          = fw_net_start,
    .stop           = fw_net_stop,
    .phy_autoneg    = fw_net_phy_autoneg,
    .tx_submit      = fw_net_tx_submit,
    .tx_reclaim     = fw_net_tx_reclaim,
    .rx_poll        = fw_net_rx_poll,
    .isr            = fw_net_isr,
    .mdio_read      = fw_net_mdio_read,
    .mdio_write     = fw_net_mdio_write,
};

/* ---- binding entry point ---- */
int uiox_net_fwbind(uiox_net_dev_t *hw, struct uiox_i2c_dev *i2c)
{
    if (!hw || !i2c) return -EINVAL;
    uiox_net_fwbind_t *b = NULL;
    for (uint32_t i = 0u; i < UIOX_NET_FWBIND_MAX; i++)
        if (!s_net_bind[i].in_use) { b = &s_net_bind[i]; break; }
    if (!b) return -ENOSPC;
    b->in_use = true;
    b->i2c = *i2c;
    b->hw  = hw;
    uiox_fw_err_t frc = uiox_fw_eth_init_virtio(&b->fw, hw->base_addr, hw->irq);
    if (frc != UIOX_FW_OK) { b->in_use = false; return uiox_fw_rc(frc); }
    int rc = uiox_net_hw_init(hw, &s_net_fwops);
    if (rc < 0) { b->in_use = false; return rc; }
    return 0;
}
