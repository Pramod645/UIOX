/**
 * @file    uiox_spk_fwops.c
 * @brief   Adapter: 02_FwHal implementation -> layer-20 uiox_spk_hw_ops_t.
 *
 * PATH   30_KIX/34_DSS/20_DriverInterfaces/03_NonSensors/speaker/uiox_spk_fwops.c
 * LAYER  20_DriverInterfaces -- hardware-control interface.
 *
 * BOUNDARY
 *   Above: 30_DeviceDrivers/uiox_spk_if.c calls through hw->ops and never
 *          sees a firmware symbol.
 *   Below: 02_FwHal builds to libuiox_fw_<arch>.a; this file calls its flat
 *          uiox_fw_spk_*() API and never includes its sources.
 *   This file is the joint.
 *
 * REGENERATED -- every stub carries the exact signature of its vtable slot
 * (15 slots).  Bodies return -ENOSYS: an honest "not yet
 * bound", never a silent success.  Every slot parameter is explicitly cast
 * to void so the strict -Werror set stays intact for the driver units that
 * do carry logic.
 */

#include "uiox_spk_hw.h"
#include "uiox_fw_rc.h"
#include "uiox_fw_speaker.h"
#include "uiox_fw_i2c.h"

#define UIOX_SPK_FWBIND_MAX   2u

typedef struct {
    uiox_spk_dev_t  fw;
    uiox_i2c_dev_t     i2c;
    uiox_spk_hw_t             *hw;
    bool                in_use;
} uiox_spk_fwbind_t;

static uiox_spk_fwbind_t s_spk_bind[UIOX_SPK_FWBIND_MAX];

static uiox_spk_fwbind_t *bind_of(uiox_spk_hw_t *hw)
{
    if (!hw) return NULL;
    for (uint32_t i = 0u; i < UIOX_SPK_FWBIND_MAX; i++)
        if (s_spk_bind[i].in_use && s_spk_bind[i].hw == hw)
            return &s_spk_bind[i];
    return NULL;
}

/* ---- vtable slots ---- */
static int fw_spk_init(uiox_spk_hw_t *hw)
{
    (void)hw;
    uiox_spk_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

/* deinit: IRQ/void slot -- the firmware layer owns the IRQ controller
 * but does not route device interrupts into class handlers
 * (10_BSP/10_Arch/[*]/arch_runtime.c). */
static void fw_spk_deinit(uiox_spk_hw_t *hw) { (void)hw; }
static int fw_spk_start(uiox_spk_hw_t *hw)
{
    (void)hw;
    uiox_spk_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

/* stop: IRQ/void slot -- the firmware layer owns the IRQ controller
 * but does not route device interrupts into class handlers
 * (10_BSP/10_Arch/[*]/arch_runtime.c). */
static void fw_spk_stop(uiox_spk_hw_t *hw) { (void)hw; }
static int fw_spk_set_format(uiox_spk_hw_t *hw, const uiox_spk_audio_fmt_t *fmt)
{
    (void)hw; (void)fmt;
    uiox_spk_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_spk_set_volume(uiox_spk_hw_t *hw, uint8_t vol_pct)
{
    (void)hw; (void)vol_pct;
    uiox_spk_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_spk_set_mute(uiox_spk_hw_t *hw, bool mute)
{
    (void)hw; (void)mute;
    uiox_spk_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_spk_dma_submit(uiox_spk_hw_t *hw, uintptr_t phys, uint32_t bytes, bool last)
{
    (void)hw; (void)phys; (void)bytes; (void)last;
    uiox_spk_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_spk_dma_flush(uiox_spk_hw_t *hw)
{
    (void)hw;
    uiox_spk_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_spk_i2c_read(uiox_spk_hw_t *hw, uint8_t dev_addr, uint8_t reg, uint8_t *buf, uint16_t len)
{
    (void)hw; (void)dev_addr; (void)reg; (void)buf; (void)len;
    uiox_spk_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_spk_i2c_write(uiox_spk_hw_t *hw, uint8_t dev_addr, uint8_t reg, const uint8_t *buf, uint16_t len)
{
    (void)hw; (void)dev_addr; (void)reg; (void)buf; (void)len;
    uiox_spk_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

/* gpio_set: IRQ/void slot -- the firmware layer owns the IRQ controller
 * but does not route device interrupts into class handlers
 * (10_BSP/10_Arch/[*]/arch_runtime.c). */
static void fw_spk_gpio_set(uiox_spk_hw_t *hw, uint8_t pin, bool val) { (void)hw; (void)pin; (void)val; }

/* isr_dma_half: IRQ/void slot -- the firmware layer owns the IRQ controller
 * but does not route device interrupts into class handlers
 * (10_BSP/10_Arch/[*]/arch_runtime.c). */
static void fw_spk_isr_dma_half(uiox_spk_hw_t *hw) { (void)hw; }

/* isr_dma_done: IRQ/void slot -- the firmware layer owns the IRQ controller
 * but does not route device interrupts into class handlers
 * (10_BSP/10_Arch/[*]/arch_runtime.c). */
static void fw_spk_isr_dma_done(uiox_spk_hw_t *hw) { (void)hw; }

/* isr_fault: IRQ/void slot -- the firmware layer owns the IRQ controller
 * but does not route device interrupts into class handlers
 * (10_BSP/10_Arch/[*]/arch_runtime.c). */
static void fw_spk_isr_fault(uiox_spk_hw_t *hw) { (void)hw; }

static const uiox_spk_hw_ops_t s_spk_fwops = {
    .init           = fw_spk_init,
    .deinit         = fw_spk_deinit,
    .start          = fw_spk_start,
    .stop           = fw_spk_stop,
    .set_format     = fw_spk_set_format,
    .set_volume     = fw_spk_set_volume,
    .set_mute       = fw_spk_set_mute,
    .dma_submit     = fw_spk_dma_submit,
    .dma_flush      = fw_spk_dma_flush,
    .i2c_read       = fw_spk_i2c_read,
    .i2c_write      = fw_spk_i2c_write,
    .gpio_set       = fw_spk_gpio_set,
    .isr_dma_half   = fw_spk_isr_dma_half,
    .isr_dma_done   = fw_spk_isr_dma_done,
    .isr_fault      = fw_spk_isr_fault,
};

/* ---- binding entry point ---- */
int uiox_spk_fwbind(uiox_spk_hw_t *hw, struct uiox_i2c_dev *i2c)
{
    if (!hw || !i2c) return -EINVAL;
    uiox_spk_fwbind_t *b = NULL;
    for (uint32_t i = 0u; i < UIOX_SPK_FWBIND_MAX; i++)
        if (!s_spk_bind[i].in_use) { b = &s_spk_bind[i]; break; }
    if (!b) return -ENOSPC;
    b->in_use = true;
    b->i2c = *i2c;
    b->hw  = hw;

    int rc = uiox_spk_hw_init(hw, &s_spk_fwops);
    if (rc < 0) { b->in_use = false; return rc; }

    /* TODO: 02_FwHal has no mic class entry point. */
    b->in_use = false;
    return -ENOSYS;
}

