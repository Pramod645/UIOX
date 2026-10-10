/**
 * @file    uiox_spk_fwops.c
 * @brief   Adapter: 02_FwHal implementation -> layer-20 uiox_spk_hw_ops_t.
 * @date    2026-10-10
 *
 * PATH   30_KIX/34_DSS/20_DriverInterfaces/03_NonSensors/speaker/uiox_spk_fwops.c
 * LAYER  20_DriverInterfaces — hardware-control interface.
 *
 * BOUNDARY
 *   Above: 30_DeviceDrivers/uiox_spk_if.c calls through hw->ops and never
 *          sees a firmware symbol.
 *   Below: 02_FwHal builds to libuioxfw<arch>.a; this file calls its flat
 *          uiox_fw_spk_*() API and never includes its sources.
 *   This file is the joint: it includes the FwHal public header for the
 *   declarations, and the archive supplies the definitions at link time.
 *
 * GENERATED — every slot in uiox_spk_hw_ops_t has an entry below.
 * Slots marked ENOSYS are the interrupt/GPIO entry points: the firmware
 * layer owns the IRQ controller but does not route device interrupts into
 * class handlers (that is 10_BSP/10_Arch/*/arch_runtime.c).  Reporting
 * unsupported is deliberate — silently accepting would swallow an IRQ
 * nothing services.
 */

#include "uiox_spk_hw.h"
#include "uiox_fw_rc.h"
#include "uiox_fw_spk.h"
#include "uiox_fw_i2c.h"

#define UIOX_SPK_FWBIND_MAX   2u

typedef struct {
    uiox_fw_spk_dev_t  fw;
    uiox_i2c_dev_t                     i2c;
    uiox_spk_hw_t                  *hw;
    bool                               in_use;
} uiox_spk_fwbind_t;

static uiox_spk_fwbind_t s_spk_bind[UIOX_SPK_FWBIND_MAX];

static uiox_spk_fwbind_t *bind_of(uiox_spk_hw_t *hw)
{
    if (!hw || !hw->base) return NULL;
    uiox_spk_fwbind_t *b = (uiox_spk_fwbind_t *)hw->base;
    return (b->in_use && b->hw == hw) ? b : NULL;
}

/* ---- vtable slots ---- */

static int fw_spk_init(uiox_spk_hw_t *hw)
{
    uiox_spk_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static void fw_spk_deinit(uiox_spk_hw_t *hw)
{
    uiox_spk_fwbind_t *b = bind_of(hw);
    if (!b) return;
}

static int fw_spk_start(uiox_spk_hw_t *hw)
{
    uiox_spk_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static void fw_spk_stop(uiox_spk_hw_t *hw)
{
    uiox_spk_fwbind_t *b = bind_of(hw);
    if (!b) return;
}

static int fw_spk_set_format(uiox_spk_hw_t *hw)
{
    uiox_spk_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_spk_set_volume(uiox_spk_hw_t *hw)
{
    uiox_spk_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_spk_set_mute(uiox_spk_hw_t *hw)
{
    uiox_spk_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_spk_dma_submit(uiox_spk_hw_t *hw)
{
    uiox_spk_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_spk_dma_flush(uiox_spk_hw_t *hw)
{
    uiox_spk_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_spk_i2c_read(uiox_spk_hw_t *hw)
{
    uiox_spk_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_spk_i2c_write(uiox_spk_hw_t *hw)
{
    uiox_spk_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static void fw_spk_gpio_set(uiox_spk_hw_t *hw, uint32_t pin, bool v)
{ (void)hw; (void)pin; (void)v; }

static void fw_spk_isr_dma_half(uiox_spk_hw_t *hw)
{ (void)hw; }

static void fw_spk_isr_dma_done(uiox_spk_hw_t *hw)
{ (void)hw; }

static void fw_spk_isr_fault(uiox_spk_hw_t *hw)
{ (void)hw; }

static const uiox_spk_hw_ops_t s_spk_fwops = {
    .init          = fw_spk_init,
    .deinit        = fw_spk_deinit,
    .start         = fw_spk_start,
    .stop          = fw_spk_stop,
    .set_format    = fw_spk_set_format,
    .set_volume    = fw_spk_set_volume,
    .set_mute      = fw_spk_set_mute,
    .dma_submit    = fw_spk_dma_submit,
    .dma_flush     = fw_spk_dma_flush,
    .i2c_read      = fw_spk_i2c_read,
    .i2c_write     = fw_spk_i2c_write,
    .gpio_set      = fw_spk_gpio_set,
    .isr_dma_half  = fw_spk_isr_dma_half,
    .isr_dma_done  = fw_spk_isr_dma_done,
    .isr_fault     = fw_spk_isr_fault,
};

/* ---- binding entry points (one per 02_FwHal chip helper) ---- */
int uiox_spk_fwbind   (uiox_spk_hw_t *hw, uiox_i2c_dev_t *i2c)
{
    if (!hw || !i2c) return -EINVAL;
    uiox_spk_fwbind_t *b = NULL;
    for (uint32_t i = 0u; i < UIOX_SPK_FWBIND_MAX; i++)
        if (!s_spk_bind[i].in_use) { b = &s_spk_bind[i]; break; }
    if (!b) return -ENOSPC;
    b->in_use = true;
    b->i2c = *i2c;
    b->hw  = hw;
    uiox_fw_err_t frc = uiox_fw_spk_init(&b->fw, i2c);
    if (frc != UIOX_FW_OK) { b->in_use = false; return uiox_fw_rc(frc); }
    hw->base = (uintptr_t)b;
    int rc = uiox_spk_hw_init(hw, &s_spk_fwops);
    if (rc < 0) { b->in_use = false; return rc; }
    return 0;
}
