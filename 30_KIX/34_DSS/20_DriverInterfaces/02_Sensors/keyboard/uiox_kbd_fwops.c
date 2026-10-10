/**
 * @file    uiox_kbd_fwops.c
 * @brief   Adapter: 02_FwHal implementation -> layer-20 uiox_kbd_hw_ops_t.
 * @date    2026-10-10
 *
 * PATH   30_KIX/34_DSS/20_DriverInterfaces/02_Sensors/keyboard/uiox_kbd_fwops.c
 * LAYER  20_DriverInterfaces — hardware-control interface.
 *
 * BOUNDARY
 *   Above: 30_DeviceDrivers/uiox_kbd_if.c calls through hw->ops and never
 *          sees a firmware symbol.
 *   Below: 02_FwHal builds to libuioxfw<arch>.a; this file calls its flat
 *          uiox_fw_kbd_*() API and never includes its sources.
 *   This file is the joint: it includes the FwHal public header for the
 *   declarations, and the archive supplies the definitions at link time.
 *
 * GENERATED — every slot in uiox_kbd_hw_ops_t has an entry below.
 * Slots marked ENOSYS are the interrupt/GPIO entry points: the firmware
 * layer owns the IRQ controller but does not route device interrupts into
 * class handlers (that is 10_BSP/10_Arch/*/arch_runtime.c).  Reporting
 * unsupported is deliberate — silently accepting would swallow an IRQ
 * nothing services.
 */

#include "uiox_kbd_hw.h"
#include "uiox_fw_rc.h"
#include "uiox_fw_kbd.h"
#include "uiox_fw_i2c.h"

#define UIOX_KBD_FWBIND_MAX   2u

typedef struct {
    uiox_fw_kbd_dev_t  fw;
    uiox_i2c_dev_t                     i2c;
    uiox_kbd_hw_t                  *hw;
    bool                               in_use;
} uiox_kbd_fwbind_t;

static uiox_kbd_fwbind_t s_kbd_bind[UIOX_KBD_FWBIND_MAX];

static uiox_kbd_fwbind_t *bind_of(uiox_kbd_hw_t *hw)
{
    if (!hw || !hw->base) return NULL;
    uiox_kbd_fwbind_t *b = (uiox_kbd_fwbind_t *)hw->base;
    return (b->in_use && b->hw == hw) ? b : NULL;
}

/* ---- vtable slots ---- */

static int fw_kbd_init(uiox_kbd_hw_t *hw)
{
    uiox_kbd_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static void fw_kbd_deinit(uiox_kbd_hw_t *hw)
{
    uiox_kbd_fwbind_t *b = bind_of(hw);
    if (!b) return;
}

static int fw_kbd_scan_row(uiox_kbd_hw_t *hw)
{
    uiox_kbd_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_kbd_read_direct(uiox_kbd_hw_t *hw)
{
    uiox_kbd_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_kbd_i2c_read(uiox_kbd_hw_t *hw)
{
    uiox_kbd_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_kbd_i2c_write(uiox_kbd_hw_t *hw)
{
    uiox_kbd_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_kbd_spi_transfer(uiox_kbd_hw_t *hw)
{
    uiox_kbd_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_kbd_ps2_send(uiox_kbd_hw_t *hw)
{
    uiox_kbd_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_kbd_ps2_recv(uiox_kbd_hw_t *hw)
{
    uiox_kbd_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_kbd_set_leds(uiox_kbd_hw_t *hw)
{
    uiox_kbd_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_kbd_set_backlight(uiox_kbd_hw_t *hw)
{
    uiox_kbd_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_kbd_delay_us(uiox_kbd_hw_t *hw)
{
    uiox_kbd_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static void fw_kbd_isr(uiox_kbd_hw_t *hw)
{ (void)hw; }

static const uiox_kbd_hw_ops_t s_kbd_fwops = {
    .init          = fw_kbd_init,
    .deinit        = fw_kbd_deinit,
    .scan_row      = fw_kbd_scan_row,
    .read_direct   = fw_kbd_read_direct,
    .i2c_read      = fw_kbd_i2c_read,
    .i2c_write     = fw_kbd_i2c_write,
    .spi_transfer  = fw_kbd_spi_transfer,
    .ps2_send      = fw_kbd_ps2_send,
    .ps2_recv      = fw_kbd_ps2_recv,
    .set_leds      = fw_kbd_set_leds,
    .set_backlight = fw_kbd_set_backlight,
    .delay_us      = fw_kbd_delay_us,
    .isr           = fw_kbd_isr,
};

/* ---- binding entry points (one per 02_FwHal chip helper) ---- */
int uiox_kbd_fwbind   (uiox_kbd_hw_t *hw, uiox_i2c_dev_t *i2c)
{
    if (!hw || !i2c) return -EINVAL;
    uiox_kbd_fwbind_t *b = NULL;
    for (uint32_t i = 0u; i < UIOX_KBD_FWBIND_MAX; i++)
        if (!s_kbd_bind[i].in_use) { b = &s_kbd_bind[i]; break; }
    if (!b) return -ENOSPC;
    b->in_use = true;
    b->i2c = *i2c;
    b->hw  = hw;
    uiox_fw_err_t frc = uiox_fw_kbd_init(&b->fw, i2c);
    if (frc != UIOX_FW_OK) { b->in_use = false; return uiox_fw_rc(frc); }
    hw->base = (uintptr_t)b;
    int rc = uiox_kbd_hw_init(hw, &s_kbd_fwops);
    if (rc < 0) { b->in_use = false; return rc; }
    return 0;
}
