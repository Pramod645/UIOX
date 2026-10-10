/**
 * @file    uiox_cam_fwops.c
 * @brief   Adapter: 02_FwHal implementation -> layer-20 uiox_cam_hw_ops_t.
 * @date    2026-10-10
 *
 * PATH   30_KIX/34_DSS/20_DriverInterfaces/02_Sensors/camera/uiox_cam_fwops.c
 * LAYER  20_DriverInterfaces — hardware-control interface.
 *
 * BOUNDARY
 *   Above: 30_DeviceDrivers/uiox_cam_if.c calls through hw->ops and never
 *          sees a firmware symbol.
 *   Below: 02_FwHal builds to libuioxfw<arch>.a; this file calls its flat
 *          uiox_fw_cam_*() API and never includes its sources.
 *   This file is the joint: it includes the FwHal public header for the
 *   declarations, and the archive supplies the definitions at link time.
 *
 * GENERATED — every slot in uiox_cam_hw_ops_t has an entry below.
 * Slots marked ENOSYS are the interrupt/GPIO entry points: the firmware
 * layer owns the IRQ controller but does not route device interrupts into
 * class handlers (that is 10_BSP/10_Arch/*/arch_runtime.c).  Reporting
 * unsupported is deliberate — silently accepting would swallow an IRQ
 * nothing services.
 */

#include "uiox_cam_hw.h"
#include "uiox_fw_rc.h"
#include "uiox_fw_cam.h"
#include "uiox_fw_i2c.h"

#define UIOX_CAM_FWBIND_MAX   2u

typedef struct {
    uiox_fw_cam_dev_t  fw;
    uiox_i2c_dev_t                     i2c;
    uiox_cam_hw_t                  *hw;
    bool                               in_use;
} uiox_cam_fwbind_t;

static uiox_cam_fwbind_t s_cam_bind[UIOX_CAM_FWBIND_MAX];

static uiox_cam_fwbind_t *bind_of(uiox_cam_hw_t *hw)
{
    if (!hw || !hw->base) return NULL;
    uiox_cam_fwbind_t *b = (uiox_cam_fwbind_t *)hw->base;
    return (b->in_use && b->hw == hw) ? b : NULL;
}

/* ---- vtable slots ---- */

static int fw_cam_init(uiox_cam_hw_t *hw)
{
    uiox_cam_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static void fw_cam_deinit(uiox_cam_hw_t *hw)
{
    uiox_cam_fwbind_t *b = bind_of(hw);
    if (!b) return;
}

static int fw_cam_start(uiox_cam_hw_t *hw)
{
    uiox_cam_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static void fw_cam_stop(uiox_cam_hw_t *hw)
{
    uiox_cam_fwbind_t *b = bind_of(hw);
    if (!b) return;
}

static int fw_cam_set_csi(uiox_cam_hw_t *hw)
{
    uiox_cam_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_cam_set_format(uiox_cam_hw_t *hw)
{
    uiox_cam_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_cam_dma_queue(uiox_cam_hw_t *hw)
{
    uiox_cam_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_cam_dma_complete(uiox_cam_hw_t *hw)
{
    uiox_cam_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static void fw_cam_isr(uiox_cam_hw_t *hw)
{ (void)hw; }

static const uiox_cam_hw_ops_t s_cam_fwops = {
    .init          = fw_cam_init,
    .deinit        = fw_cam_deinit,
    .start         = fw_cam_start,
    .stop          = fw_cam_stop,
    .set_csi       = fw_cam_set_csi,
    .set_format    = fw_cam_set_format,
    .dma_queue     = fw_cam_dma_queue,
    .dma_complete  = fw_cam_dma_complete,
    .isr           = fw_cam_isr,
};

/* ---- binding entry points (one per 02_FwHal chip helper) ---- */
int uiox_cam_fwbind   (uiox_cam_hw_t *hw, uiox_i2c_dev_t *i2c)
{
    if (!hw || !i2c) return -EINVAL;
    uiox_cam_fwbind_t *b = NULL;
    for (uint32_t i = 0u; i < UIOX_CAM_FWBIND_MAX; i++)
        if (!s_cam_bind[i].in_use) { b = &s_cam_bind[i]; break; }
    if (!b) return -ENOSPC;
    b->in_use = true;
    b->i2c = *i2c;
    b->hw  = hw;
    uiox_fw_err_t frc = uiox_fw_cam_init(&b->fw, i2c);
    if (frc != UIOX_FW_OK) { b->in_use = false; return uiox_fw_rc(frc); }
    hw->base = (uintptr_t)b;
    int rc = uiox_cam_hw_init(hw, &s_cam_fwops);
    if (rc < 0) { b->in_use = false; return rc; }
    return 0;
}
