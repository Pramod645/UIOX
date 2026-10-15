/**
 * @file    uiox_chg_fwops.c
 * @brief   Adapter: 02_FwHal implementation -> layer-20 uiox_chg_hw_ops_t.
 *
 * PATH   30_KIX/34_DSS/20_DriverInterfaces/03_NonSensors/chg/uiox_chg_fwops.c
 * LAYER  20_DriverInterfaces -- hardware-control interface.
 *
 * BOUNDARY
 *   Above: 30_DeviceDrivers/uiox_chg_if.c calls through hw->ops and never
 *          sees a firmware symbol.
 *   Below: 02_FwHal builds to libuiox_fw_<arch>.a; this file calls its flat
 *          uiox_fw_charger_*() API and never includes its sources.
 *   This file is the joint.
 *
 * REGENERATED -- every stub carries the exact signature of its vtable slot
 * (19 slots).  Bodies return -ENOSYS: an honest "not yet
 * bound", never a silent success.  Every slot parameter is explicitly cast
 * to void so the strict -Werror set stays intact for the driver units that
 * do carry logic.
 */

#include "uiox_chg_hw.h"
#include "uiox_fw_rc.h"
#include "uiox_fw_charger.h"
#include "uiox_fw_i2c.h"

#define UIOX_CHG_FWBIND_MAX   2u

typedef struct {
    uiox_chg_dev_t  fw;
    uiox_i2c_dev_t     i2c;
    uiox_chg_hw_t             *hw;
    bool                in_use;
} uiox_chg_fwbind_t;

static uiox_chg_fwbind_t s_chg_bind[UIOX_CHG_FWBIND_MAX];

static uiox_chg_fwbind_t *bind_of(uiox_chg_hw_t *hw)
{
    if (!hw) return NULL;
    for (uint32_t i = 0u; i < UIOX_CHG_FWBIND_MAX; i++)
        if (s_chg_bind[i].in_use && s_chg_bind[i].hw == hw)
            return &s_chg_bind[i];
    return NULL;
}

/* ---- vtable slots ---- */
static int fw_chg_init(uiox_chg_hw_t *hw)
{
    (void)hw;
    uiox_chg_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

/* deinit: IRQ/void slot -- the firmware layer owns the IRQ controller
 * but does not route device interrupts into class handlers
 * (10_BSP/10_Arch/[*]/arch_runtime.c). */
static void fw_chg_deinit(uiox_chg_hw_t *hw) { (void)hw; }
static int fw_chg_reg_read(uiox_chg_hw_t *hw, uint8_t reg, uint8_t *val)
{
    (void)hw; (void)reg; (void)val;
    uiox_chg_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_chg_reg_write(uiox_chg_hw_t *hw, uint8_t reg, uint8_t val)
{
    (void)hw; (void)reg; (void)val;
    uiox_chg_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_chg_reg_rmw(uiox_chg_hw_t *hw, uint8_t reg, uint8_t mask, uint8_t bits)
{
    (void)hw; (void)reg; (void)mask; (void)bits;
    uiox_chg_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_chg_adc_read(uiox_chg_hw_t *hw, uiox_chg_adc_ch_t ch, int32_t *val_mv)
{
    (void)hw; (void)ch; (void)val_mv;
    uiox_chg_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_chg_set_ichg(uiox_chg_hw_t *hw, uint32_t ma)
{
    (void)hw; (void)ma;
    uiox_chg_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_chg_set_vchg(uiox_chg_hw_t *hw, uint32_t mv)
{
    (void)hw; (void)mv;
    uiox_chg_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_chg_set_iin_lim(uiox_chg_hw_t *hw, uint32_t ma)
{
    (void)hw; (void)ma;
    uiox_chg_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_chg_set_vindpm(uiox_chg_hw_t *hw, uint32_t mv)
{
    (void)hw; (void)mv;
    uiox_chg_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_chg_charge_enable(uiox_chg_hw_t *hw, bool en)
{
    (void)hw; (void)en;
    uiox_chg_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_chg_otg_enable(uiox_chg_hw_t *hw, bool en)
{
    (void)hw; (void)en;
    uiox_chg_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_chg_get_status(uiox_chg_hw_t *hw, uiox_chg_chrg_t *chrg, uiox_chg_src_t *src, uint32_t *faults)
{
    (void)hw; (void)chrg; (void)src; (void)faults;
    uiox_chg_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_chg_wdog_reset(uiox_chg_hw_t *hw)
{
    (void)hw;
    uiox_chg_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

/* gpio_write: IRQ/void slot -- the firmware layer owns the IRQ controller
 * but does not route device interrupts into class handlers
 * (10_BSP/10_Arch/[*]/arch_runtime.c). */
static void fw_chg_gpio_write(uiox_chg_hw_t *hw, uint32_t pin, bool val) { (void)hw; (void)pin; (void)val; }

/* gpio_read: IRQ/void slot -- the firmware layer owns the IRQ controller
 * but does not route device interrupts into class handlers
 * (10_BSP/10_Arch/[*]/arch_runtime.c). */
static bool fw_chg_gpio_read(uiox_chg_hw_t *hw, uint32_t pin) { (void)hw; (void)pin; }
static int fw_chg_pd_tx_msg(uiox_chg_hw_t *hw, const uint8_t *buf, uint8_t len)
{
    (void)hw; (void)buf; (void)len;
    uiox_chg_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_chg_pd_rx_msg(uiox_chg_hw_t *hw, uint8_t *buf, uint8_t max_len)
{
    (void)hw; (void)buf; (void)max_len;
    uiox_chg_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

/* isr: IRQ/void slot -- the firmware layer owns the IRQ controller
 * but does not route device interrupts into class handlers
 * (10_BSP/10_Arch/[*]/arch_runtime.c). */
static void fw_chg_isr(uiox_chg_hw_t *hw) { (void)hw; }

static const uiox_chg_hw_ops_t s_chg_fwops = {
    .init           = fw_chg_init,
    .deinit         = fw_chg_deinit,
    .reg_read       = fw_chg_reg_read,
    .reg_write      = fw_chg_reg_write,
    .reg_rmw        = fw_chg_reg_rmw,
    .adc_read       = fw_chg_adc_read,
    .set_ichg       = fw_chg_set_ichg,
    .set_vchg       = fw_chg_set_vchg,
    .set_iin_lim    = fw_chg_set_iin_lim,
    .set_vindpm     = fw_chg_set_vindpm,
    .charge_enable  = fw_chg_charge_enable,
    .otg_enable     = fw_chg_otg_enable,
    .get_status     = fw_chg_get_status,
    .wdog_reset     = fw_chg_wdog_reset,
    .gpio_write     = fw_chg_gpio_write,
    .gpio_read      = fw_chg_gpio_read,
    .pd_tx_msg      = fw_chg_pd_tx_msg,
    .pd_rx_msg      = fw_chg_pd_rx_msg,
    .isr            = fw_chg_isr,
};

/* ---- binding entry point ---- */
int uiox_chg_fwbind(uiox_chg_hw_t *hw, struct uiox_i2c_dev *i2c)
{
    if (!hw || !i2c) return -EINVAL;
    uiox_chg_fwbind_t *b = NULL;
    for (uint32_t i = 0u; i < UIOX_CHG_FWBIND_MAX; i++)
        if (!s_chg_bind[i].in_use) { b = &s_chg_bind[i]; break; }
    if (!b) return -ENOSPC;
    b->in_use = true;
    b->i2c = *i2c;
    b->hw  = hw;
    uiox_fw_err_t frc = uiox_fw_charger_init(&b->fw, i2c);
    if (frc != UIOX_FW_OK) { b->in_use = false; return uiox_fw_rc(frc); }
    int rc = uiox_chg_hw_init(hw, &s_chg_fwops);
    if (rc < 0) { b->in_use = false; return rc; }
    return 0;
}
