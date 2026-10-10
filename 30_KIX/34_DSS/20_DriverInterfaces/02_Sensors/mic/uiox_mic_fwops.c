/**
 * @file    uiox_mic_fwops.c
 * @brief   Adapter: 02_FwHal implementation -> layer-20 uiox_mic_hw_ops_t.
 * @date    2026-10-10
 *
 * PATH   30_KIX/34_DSS/20_DriverInterfaces/02_Sensors/mic/uiox_mic_fwops.c
 * LAYER  20_DriverInterfaces — hardware-control interface.
 *
 * BOUNDARY
 *   Above: 30_DeviceDrivers/uiox_mic_if.c calls through hw->ops and never
 *          sees a firmware symbol.
 *   Below: 02_FwHal builds to libuioxfw<arch>.a; this file calls its flat
 *          uiox_fw_mic_*() API and never includes its sources.
 *   This file is the joint: it includes the FwHal public header for the
 *   declarations, and the archive supplies the definitions at link time.
 *
 * GENERATED — every slot in uiox_mic_hw_ops_t has an entry below.
 * Slots marked ENOSYS are the interrupt/GPIO entry points: the firmware
 * layer owns the IRQ controller but does not route device interrupts into
 * class handlers (that is 10_BSP/10_Arch/*/arch_runtime.c).  Reporting
 * unsupported is deliberate — silently accepting would swallow an IRQ
 * nothing services.
 */

#include "uiox_mic_hw.h"
#include "uiox_fw_rc.h"
#include "uiox_fw_mic.h"
#include "uiox_fw_i2c.h"

#define UIOX_MIC_FWBIND_MAX   2u

typedef struct {
    uiox_fw_mic_dev_t  fw;
    uiox_i2c_dev_t                     i2c;
    uiox_mic_hw_t                  *hw;
    bool                               in_use;
} uiox_mic_fwbind_t;

static uiox_mic_fwbind_t s_mic_bind[UIOX_MIC_FWBIND_MAX];

static uiox_mic_fwbind_t *bind_of(uiox_mic_hw_t *hw)
{
    if (!hw || !hw->base) return NULL;
    uiox_mic_fwbind_t *b = (uiox_mic_fwbind_t *)hw->base;
    return (b->in_use && b->hw == hw) ? b : NULL;
}

/* ---- vtable slots ---- */

static int fw_mic_init(uiox_mic_hw_t *hw)
{
    uiox_mic_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static void fw_mic_deinit(uiox_mic_hw_t *hw)
{
    uiox_mic_fwbind_t *b = bind_of(hw);
    if (!b) return;
}

static int fw_mic_start(uiox_mic_hw_t *hw)
{
    uiox_mic_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static void fw_mic_stop(uiox_mic_hw_t *hw)
{
    uiox_mic_fwbind_t *b = bind_of(hw);
    if (!b) return;
}

static int fw_mic_set_format(uiox_mic_hw_t *hw)
{
    uiox_mic_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_mic_set_gain(uiox_mic_hw_t *hw)
{
    uiox_mic_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_mic_set_mute(uiox_mic_hw_t *hw)
{
    uiox_mic_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_mic_dma_submit(uiox_mic_hw_t *hw)
{
    uiox_mic_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_mic_i2c_read(uiox_mic_hw_t *hw)
{
    uiox_mic_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_mic_i2c_write(uiox_mic_hw_t *hw)
{
    uiox_mic_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static void fw_mic_gpio_set(uiox_mic_hw_t *hw, uint32_t pin, bool v)
{ (void)hw; (void)pin; (void)v; }

static void fw_mic_isr_dma_half(uiox_mic_hw_t *hw)
{ (void)hw; }

static void fw_mic_isr_dma_done(uiox_mic_hw_t *hw)
{ (void)hw; }

static void fw_mic_isr_overrun(uiox_mic_hw_t *hw)
{ (void)hw; }

static const uiox_mic_hw_ops_t s_mic_fwops = {
    .init          = fw_mic_init,
    .deinit        = fw_mic_deinit,
    .start         = fw_mic_start,
    .stop          = fw_mic_stop,
    .set_format    = fw_mic_set_format,
    .set_gain      = fw_mic_set_gain,
    .set_mute      = fw_mic_set_mute,
    .dma_submit    = fw_mic_dma_submit,
    .i2c_read      = fw_mic_i2c_read,
    .i2c_write     = fw_mic_i2c_write,
    .gpio_set      = fw_mic_gpio_set,
    .isr_dma_half  = fw_mic_isr_dma_half,
    .isr_dma_done  = fw_mic_isr_dma_done,
    .isr_overrun   = fw_mic_isr_overrun,
};

/* ---- binding entry points (one per 02_FwHal chip helper) ---- */
int uiox_mic_fwbind   (uiox_mic_hw_t *hw, uiox_i2c_dev_t *i2c)
{
    if (!hw || !i2c) return -EINVAL;
    uiox_mic_fwbind_t *b = NULL;
    for (uint32_t i = 0u; i < UIOX_MIC_FWBIND_MAX; i++)
        if (!s_mic_bind[i].in_use) { b = &s_mic_bind[i]; break; }
    if (!b) return -ENOSPC;
    b->in_use = true;
    b->i2c = *i2c;
    b->hw  = hw;
    uiox_fw_err_t frc = uiox_fw_mic_init(&b->fw, i2c);
    if (frc != UIOX_FW_OK) { b->in_use = false; return uiox_fw_rc(frc); }
    hw->base = (uintptr_t)b;
    int rc = uiox_mic_hw_init(hw, &s_mic_fwops);
    if (rc < 0) { b->in_use = false; return rc; }
    return 0;
}
