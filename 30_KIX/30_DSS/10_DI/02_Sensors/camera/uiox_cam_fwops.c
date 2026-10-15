/**
 * @file    uiox_cam_fwops.c
 * @brief   Adapter: 02_FwHal implementation -> layer-20 uiox_cam_hw_ops_t.
 *
 * PATH   30_KIX/30_DSS/10_DI/02_Sensors/camera/uiox_cam_fwops.c
 * LAYER  10_DI -- hardware-control interface.
 *
 * BOUNDARY
 *   Above: 20_DD/uiox_cam_if.c calls through hw->ops and never
 *          sees a firmware symbol.
 *   Below: 02_FwHal builds to libuiox_fw_<arch>.a; this file calls its flat
 *          uiox_fw_camera_*() API and never includes its sources.
 *   This file is the joint.
 *
 * REGENERATED -- every stub carries the exact signature of its vtable slot
 * (9 slots).  Bodies return -ENOSYS: an honest "not yet
 * bound", never a silent success.  Every slot parameter is explicitly cast
 * to void so the strict -Werror set stays intact for the driver units that
 * do carry logic.
 */

#include "uiox_cam_hw.h"
#include "uiox_fw_rc.h"
#include "uiox_fw_camera.h"
#include "uiox_fw_i2c.h"

#define UIOX_CAM_FWBIND_MAX   2u

typedef struct {
    uiox_cam_dev_t  fw;
    uiox_i2c_dev_t     i2c;
    uiox_cam_hw_t             *hw;
    bool                in_use;
} uiox_cam_fwbind_t;

static uiox_cam_fwbind_t s_cam_bind[UIOX_CAM_FWBIND_MAX];

static uiox_cam_fwbind_t *bind_of(uiox_cam_hw_t *hw)
{
    if (!hw) return NULL;
    for (uint32_t i = 0u; i < UIOX_CAM_FWBIND_MAX; i++)
        if (s_cam_bind[i].in_use && s_cam_bind[i].hw == hw)
            return &s_cam_bind[i];
    return NULL;
}

/* ---- vtable slots ---- */
static int fw_cam_init(uiox_cam_hw_t *hw)
{
    (void)hw;
    uiox_cam_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

/* deinit: IRQ/void slot -- the firmware layer owns the IRQ controller
 * but does not route device interrupts into class handlers
 * (10_BSP/10_Arch/[*]/arch_runtime.c). */
static void fw_cam_deinit(uiox_cam_hw_t *hw) { (void)hw; }
static int fw_cam_start(uiox_cam_hw_t *hw)
{
    (void)hw;
    uiox_cam_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

/* stop: IRQ/void slot -- the firmware layer owns the IRQ controller
 * but does not route device interrupts into class handlers
 * (10_BSP/10_Arch/[*]/arch_runtime.c). */
static void fw_cam_stop(uiox_cam_hw_t *hw) { (void)hw; }
static int fw_cam_set_csi(uiox_cam_hw_t *hw, uint8_t lanes, uiox_csi_dt_t dt, uint8_t virt_chan)
{
    (void)hw; (void)lanes; (void)dt; (void)virt_chan;
    uiox_cam_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_cam_set_format(uiox_cam_hw_t *hw, uint16_t width, uint16_t height, uiox_cam_pixfmt_t fmt)
{
    (void)hw; (void)width; (void)height; (void)fmt;
    uiox_cam_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_cam_dma_queue(uiox_cam_hw_t *hw, uintptr_t phys, uint32_t length)
{
    (void)hw; (void)phys; (void)length;
    uiox_cam_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_cam_dma_complete(uiox_cam_hw_t *hw, uintptr_t *phys_out, uint32_t *bytes_out)
{
    (void)hw; (void)phys_out; (void)bytes_out;
    uiox_cam_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

/* isr: IRQ/void slot -- the firmware layer owns the IRQ controller
 * but does not route device interrupts into class handlers
 * (10_BSP/10_Arch/[*]/arch_runtime.c). */
static void fw_cam_isr(uiox_cam_hw_t *hw) { (void)hw; }

static const uiox_cam_hw_ops_t s_cam_fwops = {
    .init           = fw_cam_init,
    .deinit         = fw_cam_deinit,
    .start          = fw_cam_start,
    .stop           = fw_cam_stop,
    .set_csi        = fw_cam_set_csi,
    .set_format     = fw_cam_set_format,
    .dma_queue      = fw_cam_dma_queue,
    .dma_complete   = fw_cam_dma_complete,
    .isr            = fw_cam_isr,
};

/* ---- binding entry point ---- */
int uiox_cam_fwbind(uiox_cam_hw_t *hw, struct uiox_i2c_dev *i2c)
{
    if (!hw || !i2c) return -EINVAL;
    uiox_cam_fwbind_t *b = NULL;
    for (uint32_t i = 0u; i < UIOX_CAM_FWBIND_MAX; i++)
        if (!s_cam_bind[i].in_use) { b = &s_cam_bind[i]; break; }
    if (!b) return -ENOSPC;
    b->in_use = true;
    b->i2c = *i2c;
    b->hw  = hw;
    uiox_fw_err_t frc = uiox_fw_cam_init_ov5640(&b->fw, i2c, 0u, 0u);
    if (frc != UIOX_FW_OK) { b->in_use = false; return uiox_fw_rc(frc); }
    int rc = uiox_cam_hw_init(hw, &s_cam_fwops);
    if (rc < 0) { b->in_use = false; return rc; }
    return 0;
}
