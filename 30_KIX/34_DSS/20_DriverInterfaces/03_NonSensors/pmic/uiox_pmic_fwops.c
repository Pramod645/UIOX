/**
 * @file    uiox_pmic_fwops.c
 * @brief   Adapter: 02_FwHal implementation -> layer-20 uiox_pmic_hw_ops_t.
 * @date    2026-10-10
 *
 * PATH   30_KIX/34_DSS/20_DriverInterfaces/03_NonSensors/pmic/uiox_pmic_fwops.c
 * LAYER  20_DriverInterfaces — hardware-control interface.
 *
 * BOUNDARY
 *   Above: 30_DeviceDrivers/uiox_pmic_if.c calls through hw->ops and never
 *          sees a firmware symbol.
 *   Below: 02_FwHal builds to libuioxfw<arch>.a; this file calls its flat
 *          uiox_fw_pmic_*() API and never includes its sources.
 *   This file is the joint: it includes the FwHal public header for the
 *   declarations, and the archive supplies the definitions at link time.
 *
 * GENERATED — every slot in uiox_pmic_hw_ops_t has an entry below.
 * Slots marked ENOSYS are the interrupt/GPIO entry points: the firmware
 * layer owns the IRQ controller but does not route device interrupts into
 * class handlers (that is 10_BSP/10_Arch/*/arch_runtime.c).  Reporting
 * unsupported is deliberate — silently accepting would swallow an IRQ
 * nothing services.
 */

#include "uiox_pmic_hw.h"
#include "uiox_fw_rc.h"
#include "uiox_fw_pmic.h"
#include "uiox_fw_i2c.h"

#define UIOX_PMIC_FWBIND_MAX   2u

typedef struct {
    uiox_fw_pmic_dev_t  fw;
    uiox_i2c_dev_t                     i2c;
    uiox_pmic_hw_t                  *hw;
    bool                               in_use;
} uiox_pmic_fwbind_t;

static uiox_pmic_fwbind_t s_pmic_bind[UIOX_PMIC_FWBIND_MAX];

static uiox_pmic_fwbind_t *bind_of(uiox_pmic_hw_t *hw)
{
    if (!hw || !hw->base) return NULL;
    uiox_pmic_fwbind_t *b = (uiox_pmic_fwbind_t *)hw->base;
    return (b->in_use && b->hw == hw) ? b : NULL;
}

/* ---- vtable slots ---- */

static int fw_pmic_init(uiox_pmic_hw_t *hw)
{
    uiox_pmic_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static void fw_pmic_deinit(uiox_pmic_hw_t *hw)
{
    uiox_pmic_fwbind_t *b = bind_of(hw);
    if (!b) return;
}

static int fw_pmic_enable(uiox_pmic_hw_t *hw)
{
    uiox_pmic_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_pmic_reset(uiox_pmic_hw_t *hw)
{
    uiox_pmic_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_pmic_reg_read(uiox_pmic_hw_t *hw, uint8_t reg, uint16_t *val)
{
    uiox_pmic_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    if (!val) return -EINVAL;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_pmic_reg_write(uiox_pmic_hw_t *hw, uint8_t reg, uint16_t val)
{
    uiox_pmic_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_pmic_reg_update(uiox_pmic_hw_t *hw)
{
    uiox_pmic_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_pmic_bulk_read(uiox_pmic_hw_t *hw)
{
    uiox_pmic_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_pmic_bulk_write(uiox_pmic_hw_t *hw)
{
    uiox_pmic_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_pmic_adc_read(uiox_pmic_hw_t *hw)
{
    uiox_pmic_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_pmic_wdt_kick(uiox_pmic_hw_t *hw)
{
    uiox_pmic_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_pmic_wdt_enable(uiox_pmic_hw_t *hw)
{
    uiox_pmic_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_pmic_irq_status(uiox_pmic_hw_t *hw)
{
    uiox_pmic_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_pmic_irq_clear(uiox_pmic_hw_t *hw)
{
    uiox_pmic_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_pmic_irq_mask(uiox_pmic_hw_t *hw)
{
    uiox_pmic_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_pmic_irq_unmask(uiox_pmic_hw_t *hw)
{
    uiox_pmic_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static bool fw_pmic_gpio_read(uiox_pmic_hw_t *hw, uint32_t pin)
{ (void)hw; (void)pin; return false; }

static void fw_pmic_gpio_write(uiox_pmic_hw_t *hw, uint32_t pin, bool v)
{ (void)hw; (void)pin; (void)v; }

static void fw_pmic_isr(uiox_pmic_hw_t *hw)
{ (void)hw; }

static const uiox_pmic_hw_ops_t s_pmic_fwops = {
    .init          = fw_pmic_init,
    .deinit        = fw_pmic_deinit,
    .enable        = fw_pmic_enable,
    .reset         = fw_pmic_reset,
    .reg_read      = fw_pmic_reg_read,
    .reg_write     = fw_pmic_reg_write,
    .reg_update    = fw_pmic_reg_update,
    .bulk_read     = fw_pmic_bulk_read,
    .bulk_write    = fw_pmic_bulk_write,
    .adc_read      = fw_pmic_adc_read,
    .wdt_kick      = fw_pmic_wdt_kick,
    .wdt_enable    = fw_pmic_wdt_enable,
    .irq_status    = fw_pmic_irq_status,
    .irq_clear     = fw_pmic_irq_clear,
    .irq_mask      = fw_pmic_irq_mask,
    .irq_unmask    = fw_pmic_irq_unmask,
    .gpio_read     = fw_pmic_gpio_read,
    .gpio_write    = fw_pmic_gpio_write,
    .isr           = fw_pmic_isr,
};

/* ---- binding entry points (one per 02_FwHal chip helper) ---- */
int uiox_pmic_fwbind   (uiox_pmic_hw_t *hw, uiox_i2c_dev_t *i2c)
{
    if (!hw || !i2c) return -EINVAL;
    uiox_pmic_fwbind_t *b = NULL;
    for (uint32_t i = 0u; i < UIOX_PMIC_FWBIND_MAX; i++)
        if (!s_pmic_bind[i].in_use) { b = &s_pmic_bind[i]; break; }
    if (!b) return -ENOSPC;
    b->in_use = true;
    b->i2c = *i2c;
    b->hw  = hw;
    uiox_fw_err_t frc = uiox_fw_pmic_init(&b->fw, i2c);
    if (frc != UIOX_FW_OK) { b->in_use = false; return uiox_fw_rc(frc); }
    hw->base = (uintptr_t)b;
    int rc = uiox_pmic_hw_init(hw, &s_pmic_fwops);
    if (rc < 0) { b->in_use = false; return rc; }
    return 0;
}
