/**
 * @file    uiox_mic_fwops.c
 * @brief   Adapter: 02_FwHal implementation -> layer-20 uiox_mic_hw_ops_t.
 *
 * PATH   30_KIX/34_DSS/20_DriverInterfaces/02_Sensors/mic/uiox_mic_fwops.c
 * LAYER  20_DriverInterfaces -- hardware-control interface.
 *
 * BOUNDARY
 *   Above: 30_DeviceDrivers/uiox_mic_if.c calls through hw->ops and never
 *          sees a firmware symbol.
 *   Below: 02_FwHal builds to libuiox_fw_<arch>.a; this file calls its flat
 *          uiox_fw_mic_*() API and never includes its sources.
 *   This file is the joint.
 *
 * REGENERATED -- every stub carries the exact signature of its vtable slot
 * (14 slots).  Bodies return -ENOSYS: an honest "not yet
 * bound", never a silent success.  Every slot parameter is explicitly cast
 * to void so the strict -Werror set stays intact for the driver units that
 * do carry logic.
 */

#include "uiox_mic_hw.h"
#include "uiox_fw_rc.h"
#include "uiox_fw_mic.h"
#include "uiox_fw_i2c.h"

#define UIOX_MIC_FWBIND_MAX   2u

typedef struct {
    uiox_mic_dev_t  fw;
    uiox_i2c_dev_t     i2c;
    uiox_mic_hw_t             *hw;
    bool                in_use;
} uiox_mic_fwbind_t;

static uiox_mic_fwbind_t s_mic_bind[UIOX_MIC_FWBIND_MAX];

static uiox_mic_fwbind_t *bind_of(uiox_mic_hw_t *hw)
{
    if (!hw) return NULL;
    for (uint32_t i = 0u; i < UIOX_MIC_FWBIND_MAX; i++)
        if (s_mic_bind[i].in_use && s_mic_bind[i].hw == hw)
            return &s_mic_bind[i];
    return NULL;
}

/* ---- vtable slots ---- */
static int fw_mic_init(uiox_mic_hw_t *hw)
{
    (void)hw;
    uiox_mic_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

/* deinit: IRQ/void slot -- the firmware layer owns the IRQ controller
 * but does not route device interrupts into class handlers
 * (10_BSP/10_Arch/[*]/arch_runtime.c). */
static void fw_mic_deinit(uiox_mic_hw_t *hw) { (void)hw; }
static int fw_mic_start(uiox_mic_hw_t *hw)
{
    (void)hw;
    uiox_mic_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

/* stop: IRQ/void slot -- the firmware layer owns the IRQ controller
 * but does not route device interrupts into class handlers
 * (10_BSP/10_Arch/[*]/arch_runtime.c). */
static void fw_mic_stop(uiox_mic_hw_t *hw) { (void)hw; }
static int fw_mic_set_format(uiox_mic_hw_t *hw, const uiox_mic_audio_fmt_t *fmt)
{
    (void)hw; (void)fmt;
    uiox_mic_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_mic_set_gain(uiox_mic_hw_t *hw, uint8_t gain_db)
{
    (void)hw; (void)gain_db;
    uiox_mic_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_mic_set_mute(uiox_mic_hw_t *hw, bool mute)
{
    (void)hw; (void)mute;
    uiox_mic_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_mic_dma_submit(uiox_mic_hw_t *hw, uintptr_t phys, uint32_t bytes, bool last)
{
    (void)hw; (void)phys; (void)bytes; (void)last;
    uiox_mic_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_mic_i2c_read(uiox_mic_hw_t *hw, uint8_t dev_addr, uint8_t reg, uint8_t *buf, uint16_t len)
{
    (void)hw; (void)dev_addr; (void)reg; (void)buf; (void)len;
    uiox_mic_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_mic_i2c_write(uiox_mic_hw_t *hw, uint8_t dev_addr, uint8_t reg, const uint8_t *buf, uint16_t len)
{
    (void)hw; (void)dev_addr; (void)reg; (void)buf; (void)len;
    uiox_mic_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

/* gpio_set: IRQ/void slot -- the firmware layer owns the IRQ controller
 * but does not route device interrupts into class handlers
 * (10_BSP/10_Arch/[*]/arch_runtime.c). */
static void fw_mic_gpio_set(uiox_mic_hw_t *hw, uint8_t pin, bool val) { (void)hw; (void)pin; (void)val; }

/* isr_dma_half: IRQ/void slot -- the firmware layer owns the IRQ controller
 * but does not route device interrupts into class handlers
 * (10_BSP/10_Arch/[*]/arch_runtime.c). */
static void fw_mic_isr_dma_half(uiox_mic_hw_t *hw) { (void)hw; }

/* isr_dma_done: IRQ/void slot -- the firmware layer owns the IRQ controller
 * but does not route device interrupts into class handlers
 * (10_BSP/10_Arch/[*]/arch_runtime.c). */
static void fw_mic_isr_dma_done(uiox_mic_hw_t *hw) { (void)hw; }

/* isr_overrun: IRQ/void slot -- the firmware layer owns the IRQ controller
 * but does not route device interrupts into class handlers
 * (10_BSP/10_Arch/[*]/arch_runtime.c). */
static void fw_mic_isr_overrun(uiox_mic_hw_t *hw) { (void)hw; }

static const uiox_mic_hw_ops_t s_mic_fwops = {
    .init           = fw_mic_init,
    .deinit         = fw_mic_deinit,
    .start          = fw_mic_start,
    .stop           = fw_mic_stop,
    .set_format     = fw_mic_set_format,
    .set_gain       = fw_mic_set_gain,
    .set_mute       = fw_mic_set_mute,
    .dma_submit     = fw_mic_dma_submit,
    .i2c_read       = fw_mic_i2c_read,
    .i2c_write      = fw_mic_i2c_write,
    .gpio_set       = fw_mic_gpio_set,
    .isr_dma_half   = fw_mic_isr_dma_half,
    .isr_dma_done   = fw_mic_isr_dma_done,
    .isr_overrun    = fw_mic_isr_overrun,
};

/* ---- binding entry point ---- */
int uiox_mic_fwbind(uiox_mic_hw_t *hw, struct uiox_i2c_dev *i2c)
{
    if (!hw || !i2c) return -EINVAL;
    uiox_mic_fwbind_t *b = NULL;
    for (uint32_t i = 0u; i < UIOX_MIC_FWBIND_MAX; i++)
        if (!s_mic_bind[i].in_use) { b = &s_mic_bind[i]; break; }
    if (!b) return -ENOSPC;
    b->in_use = true;
    b->i2c = *i2c;
    b->hw  = hw;
    int rc = uiox_mic_hw_init(hw, &s_mic_fwops);
    if (rc < 0) { b->in_use = false; return rc; }

    /* TODO: 02_FwHal has no mic class entry point — uiox_fw_mic_init needs
     * an ops vtable plus audio format, and no _<bus> variant exists. */
    b->in_use = false;
    return -ENOSYS;
}

