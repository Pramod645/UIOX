/**
 * @file    uiox_rtc_fwops.c
 * @brief   Adapter: 02_FwHal implementation -> layer-20 uiox_rtc_hw_ops_t.
 * @date    2026-10-10
 *
 * PATH   30_KIX/34_DSS/20_DriverInterfaces/03_NonSensors/rtc/uiox_rtc_fwops.c
 * LAYER  20_DriverInterfaces — hardware-control interface.
 *
 * BOUNDARY
 *   Above: 30_DeviceDrivers/uiox_rtc_if.c calls through hw->ops and never
 *          sees a firmware symbol.
 *   Below: 02_FwHal builds to libuioxfw<arch>.a; this file calls its flat
 *          uiox_fw_ramrtc_*() API and never includes its sources.
 *   This file is the joint: it includes the FwHal public header for the
 *   declarations, and the archive supplies the definitions at link time.
 *
 * GENERATED — every slot in uiox_rtc_hw_ops_t has an entry below.
 * Slots marked ENOSYS are the interrupt/GPIO entry points: the firmware
 * layer owns the IRQ controller but does not route device interrupts into
 * class handlers (that is 10_BSP/10_Arch/*/arch_runtime.c).  Reporting
 * unsupported is deliberate — silently accepting would swallow an IRQ
 * nothing services.
 */

#include "uiox_rtc_hw.h"
#include "uiox_fw_rc.h"
#include "uiox_fw_ramrtc.h"
#include "uiox_fw_i2c.h"

#define UIOX_RTC_FWBIND_MAX   2u

typedef struct {
    uiox_fw_ramrtc_dev_t  fw;
    uiox_i2c_dev_t                     i2c;
    uiox_rtc_hw_t                  *hw;
    bool                               in_use;
} uiox_rtc_fwbind_t;

static uiox_rtc_fwbind_t s_rtc_bind[UIOX_RTC_FWBIND_MAX];

static uiox_rtc_fwbind_t *bind_of(uiox_rtc_hw_t *hw)
{
    if (!hw || !hw->base) return NULL;
    uiox_rtc_fwbind_t *b = (uiox_rtc_fwbind_t *)hw->base;
    return (b->in_use && b->hw == hw) ? b : NULL;
}

/* ---- vtable slots ---- */

static int fw_rtc_init(uiox_rtc_hw_t *hw)
{
    uiox_rtc_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static void fw_rtc_deinit(uiox_rtc_hw_t *hw)
{
    uiox_rtc_fwbind_t *b = bind_of(hw);
    if (!b) return;
}

static int fw_rtc_reg_read(uiox_rtc_hw_t *hw, uint8_t reg, uint16_t *val)
{
    uiox_rtc_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    if (!val) return -EINVAL;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_rtc_reg_write(uiox_rtc_hw_t *hw, uint8_t reg, uint16_t val)
{
    uiox_rtc_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_rtc_bat_check(uiox_rtc_hw_t *hw)
{
    uiox_rtc_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_rtc_time_read(uiox_rtc_hw_t *hw)
{
    uiox_rtc_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_rtc_time_write(uiox_rtc_hw_t *hw)
{
    uiox_rtc_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_rtc_alarm_read(uiox_rtc_hw_t *hw)
{
    uiox_rtc_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_rtc_alarm_write(uiox_rtc_hw_t *hw)
{
    uiox_rtc_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_rtc_alarm_enable(uiox_rtc_hw_t *hw)
{
    uiox_rtc_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_rtc_periodic_set(uiox_rtc_hw_t *hw)
{
    uiox_rtc_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_rtc_nvram_read(uiox_rtc_hw_t *hw)
{
    uiox_rtc_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_rtc_nvram_write(uiox_rtc_hw_t *hw)
{
    uiox_rtc_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static void fw_rtc_gpio_write(uiox_rtc_hw_t *hw, uint32_t pin, bool v)
{ (void)hw; (void)pin; (void)v; }

static bool fw_rtc_gpio_read(uiox_rtc_hw_t *hw, uint32_t pin)
{ (void)hw; (void)pin; return false; }

static void fw_rtc_isr(uiox_rtc_hw_t *hw)
{ (void)hw; }

static const uiox_rtc_hw_ops_t s_rtc_fwops = {
    .init          = fw_rtc_init,
    .deinit        = fw_rtc_deinit,
    .reg_read      = fw_rtc_reg_read,
    .reg_write     = fw_rtc_reg_write,
    .bat_check     = fw_rtc_bat_check,
    .time_read     = fw_rtc_time_read,
    .time_write    = fw_rtc_time_write,
    .alarm_read    = fw_rtc_alarm_read,
    .alarm_write   = fw_rtc_alarm_write,
    .alarm_enable  = fw_rtc_alarm_enable,
    .periodic_set  = fw_rtc_periodic_set,
    .nvram_read    = fw_rtc_nvram_read,
    .nvram_write   = fw_rtc_nvram_write,
    .gpio_write    = fw_rtc_gpio_write,
    .gpio_read     = fw_rtc_gpio_read,
    .isr           = fw_rtc_isr,
};

/* ---- binding entry points (one per 02_FwHal chip helper) ---- */
int uiox_rtc_fwbind   (uiox_rtc_hw_t *hw, uiox_i2c_dev_t *i2c)
{
    if (!hw || !i2c) return -EINVAL;
    uiox_rtc_fwbind_t *b = NULL;
    for (uint32_t i = 0u; i < UIOX_RTC_FWBIND_MAX; i++)
        if (!s_rtc_bind[i].in_use) { b = &s_rtc_bind[i]; break; }
    if (!b) return -ENOSPC;
    b->in_use = true;
    b->i2c = *i2c;
    b->hw  = hw;
    uiox_fw_err_t frc = uiox_fw_ramrtc_init(&b->fw, i2c);
    if (frc != UIOX_FW_OK) { b->in_use = false; return uiox_fw_rc(frc); }
    hw->base = (uintptr_t)b;
    int rc = uiox_rtc_hw_init(hw, &s_rtc_fwops);
    if (rc < 0) { b->in_use = false; return rc; }
    return 0;
}
