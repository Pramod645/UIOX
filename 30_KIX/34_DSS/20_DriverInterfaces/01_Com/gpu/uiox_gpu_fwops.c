/**
 * @file    uiox_gpu_fwops.c
 * @brief   Adapter: 02_FwHal implementation -> layer-20 uiox_gpu_hw_ops_t.
 * @date    2026-10-10
 *
 * PATH   30_KIX/34_DSS/20_DriverInterfaces/01_Com/gpu/uiox_gpu_fwops.c
 * LAYER  20_DriverInterfaces — hardware-control interface.
 *
 * BOUNDARY
 *   Above: 30_DeviceDrivers/uiox_gpu_if.c calls through hw->ops and never
 *          sees a firmware symbol.
 *   Below: 02_FwHal builds to libuioxfw<arch>.a; this file calls its flat
 *          uiox_fw_gpu_*() API and never includes its sources.
 *   This file is the joint: it includes the FwHal public header for the
 *   declarations, and the archive supplies the definitions at link time.
 *
 * GENERATED — every slot in uiox_gpu_hw_ops_t has an entry below.
 * Slots marked ENOSYS are the interrupt/GPIO entry points: the firmware
 * layer owns the IRQ controller but does not route device interrupts into
 * class handlers (that is 10_BSP/10_Arch/*/arch_runtime.c).  Reporting
 * unsupported is deliberate — silently accepting would swallow an IRQ
 * nothing services.
 */

#include "uiox_gpu_hw.h"
#include "uiox_fw_rc.h"
#include "uiox_fw_gpu.h"
#include "uiox_fw_i2c.h"

#define UIOX_GPU_FWBIND_MAX   2u

typedef struct {
    uiox_fw_gpu_dev_t  fw;
    uiox_i2c_dev_t                     i2c;
    uiox_gpu_hw_t                  *hw;
    bool                               in_use;
} uiox_gpu_fwbind_t;

static uiox_gpu_fwbind_t s_gpu_bind[UIOX_GPU_FWBIND_MAX];

static uiox_gpu_fwbind_t *bind_of(uiox_gpu_hw_t *hw)
{
    if (!hw || !hw->base) return NULL;
    uiox_gpu_fwbind_t *b = (uiox_gpu_fwbind_t *)hw->base;
    return (b->in_use && b->hw == hw) ? b : NULL;
}

/* ---- vtable slots ---- */

static int fw_gpu_init(uiox_gpu_hw_t *hw)
{
    uiox_gpu_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static void fw_gpu_deinit(uiox_gpu_hw_t *hw)
{
    uiox_gpu_fwbind_t *b = bind_of(hw);
    if (!b) return;
}

static int fw_gpu_power_on(uiox_gpu_hw_t *hw)
{
    uiox_gpu_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static void fw_gpu_power_off(uiox_gpu_hw_t *hw)
{
    uiox_gpu_fwbind_t *b = bind_of(hw);
    if (!b) return;
}

static int fw_gpu_set_freq(uiox_gpu_hw_t *hw)
{
    uiox_gpu_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_gpu_mmu_map(uiox_gpu_hw_t *hw)
{
    uiox_gpu_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_gpu_mmu_unmap(uiox_gpu_hw_t *hw)
{
    uiox_gpu_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_gpu_cmd_submit(uiox_gpu_hw_t *hw)
{
    uiox_gpu_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_gpu_fence_wait(uiox_gpu_hw_t *hw)
{
    uiox_gpu_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_gpu_shader_load(uiox_gpu_hw_t *hw)
{
    uiox_gpu_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_gpu_shader_unload(uiox_gpu_hw_t *hw)
{
    uiox_gpu_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_gpu_perf_read(uiox_gpu_hw_t *hw)
{
    uiox_gpu_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static void fw_gpu_isr_job(uiox_gpu_hw_t *hw)
{ (void)hw; }

static void fw_gpu_isr_mmu(uiox_gpu_hw_t *hw)
{ (void)hw; }

static void fw_gpu_isr_err(uiox_gpu_hw_t *hw)
{ (void)hw; }

static const uiox_gpu_hw_ops_t s_gpu_fwops = {
    .init          = fw_gpu_init,
    .deinit        = fw_gpu_deinit,
    .power_on      = fw_gpu_power_on,
    .power_off     = fw_gpu_power_off,
    .set_freq      = fw_gpu_set_freq,
    .mmu_map       = fw_gpu_mmu_map,
    .mmu_unmap     = fw_gpu_mmu_unmap,
    .cmd_submit    = fw_gpu_cmd_submit,
    .fence_wait    = fw_gpu_fence_wait,
    .shader_load   = fw_gpu_shader_load,
    .shader_unload = fw_gpu_shader_unload,
    .perf_read     = fw_gpu_perf_read,
    .isr_job       = fw_gpu_isr_job,
    .isr_mmu       = fw_gpu_isr_mmu,
    .isr_err       = fw_gpu_isr_err,
};

/* ---- binding entry points (one per 02_FwHal chip helper) ---- */
int uiox_gpu_fwbind   (uiox_gpu_hw_t *hw, uiox_i2c_dev_t *i2c)
{
    if (!hw || !i2c) return -EINVAL;
    uiox_gpu_fwbind_t *b = NULL;
    for (uint32_t i = 0u; i < UIOX_GPU_FWBIND_MAX; i++)
        if (!s_gpu_bind[i].in_use) { b = &s_gpu_bind[i]; break; }
    if (!b) return -ENOSPC;
    b->in_use = true;
    b->i2c = *i2c;
    b->hw  = hw;
    uiox_fw_err_t frc = uiox_fw_gpu_init(&b->fw, i2c);
    if (frc != UIOX_FW_OK) { b->in_use = false; return uiox_fw_rc(frc); }
    hw->base = (uintptr_t)b;
    int rc = uiox_gpu_hw_init(hw, &s_gpu_fwops);
    if (rc < 0) { b->in_use = false; return rc; }
    return 0;
}
