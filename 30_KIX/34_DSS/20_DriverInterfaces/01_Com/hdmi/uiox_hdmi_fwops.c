/**
 * @file    uiox_hdmi_fwops.c
 * @brief   Adapter: 02_FwHal implementation -> layer-20 uiox_hdmi_hw_ops_t.
 * @date    2026-10-10
 *
 * PATH   30_KIX/34_DSS/20_DriverInterfaces/01_Com/hdmi/uiox_hdmi_fwops.c
 * LAYER  20_DriverInterfaces — hardware-control interface.
 *
 * BOUNDARY
 *   Above: 30_DeviceDrivers/uiox_hdmi_if.c calls through hw->ops and never
 *          sees a firmware symbol.
 *   Below: 02_FwHal builds to libuioxfw<arch>.a; this file calls its flat
 *          uiox_fw_hdmi_*() API and never includes its sources.
 *   This file is the joint: it includes the FwHal public header for the
 *   declarations, and the archive supplies the definitions at link time.
 *
 * GENERATED — every slot in uiox_hdmi_hw_ops_t has an entry below.
 * Slots marked ENOSYS are the interrupt/GPIO entry points: the firmware
 * layer owns the IRQ controller but does not route device interrupts into
 * class handlers (that is 10_BSP/10_Arch/*/arch_runtime.c).  Reporting
 * unsupported is deliberate — silently accepting would swallow an IRQ
 * nothing services.
 */

#include "uiox_hdmi_hw.h"
#include "uiox_fw_rc.h"
#include "uiox_fw_hdmi.h"
#include "uiox_fw_i2c.h"

#define UIOX_HDMI_FWBIND_MAX   2u

typedef struct {
    uiox_fw_hdmi_dev_t  fw;
    uiox_i2c_dev_t                     i2c;
    uiox_hdmi_hw_t                  *hw;
    bool                               in_use;
} uiox_hdmi_fwbind_t;

static uiox_hdmi_fwbind_t s_hdmi_bind[UIOX_HDMI_FWBIND_MAX];

static uiox_hdmi_fwbind_t *bind_of(uiox_hdmi_hw_t *hw)
{
    if (!hw || !hw->base) return NULL;
    uiox_hdmi_fwbind_t *b = (uiox_hdmi_fwbind_t *)hw->base;
    return (b->in_use && b->hw == hw) ? b : NULL;
}

/* ---- vtable slots ---- */

static int fw_hdmi_init(uiox_hdmi_hw_t *hw)
{
    uiox_hdmi_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static void fw_hdmi_deinit(uiox_hdmi_hw_t *hw)
{
    uiox_hdmi_fwbind_t *b = bind_of(hw);
    if (!b) return;
}

static int fw_hdmi_enable(uiox_hdmi_hw_t *hw)
{
    uiox_hdmi_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static void fw_hdmi_disable(uiox_hdmi_hw_t *hw)
{
    uiox_hdmi_fwbind_t *b = bind_of(hw);
    if (!b) return;
}

static int fw_hdmi_set_timing(uiox_hdmi_hw_t *hw)
{
    uiox_hdmi_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_hdmi_set_colorspace(uiox_hdmi_hw_t *hw)
{
    uiox_hdmi_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_hdmi_set_audio(uiox_hdmi_hw_t *hw)
{
    uiox_hdmi_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_hdmi_set_frl_rate(uiox_hdmi_hw_t *hw)
{
    uiox_hdmi_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_hdmi_phy_power(uiox_hdmi_hw_t *hw)
{
    uiox_hdmi_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_hdmi_pll_set(uiox_hdmi_hw_t *hw)
{
    uiox_hdmi_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_hdmi_wait_vblank(uiox_hdmi_hw_t *hw)
{
    uiox_hdmi_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_hdmi_flip(uiox_hdmi_hw_t *hw)
{
    uiox_hdmi_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_hdmi_ddc_read(uiox_hdmi_hw_t *hw)
{
    uiox_hdmi_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_hdmi_ddc_write(uiox_hdmi_hw_t *hw)
{
    uiox_hdmi_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_hdmi_hdcp_start(uiox_hdmi_hw_t *hw)
{
    uiox_hdmi_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_hdmi_hdcp_stop(uiox_hdmi_hw_t *hw)
{
    uiox_hdmi_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_hdmi_hdcp_status(uiox_hdmi_hw_t *hw)
{
    uiox_hdmi_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_hdmi_cec_send(uiox_hdmi_hw_t *hw)
{
    uiox_hdmi_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_hdmi_cec_recv(uiox_hdmi_hw_t *hw)
{
    uiox_hdmi_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_hdmi_infoframe_send(uiox_hdmi_hw_t *hw)
{
    uiox_hdmi_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_hdmi_audio_write(uiox_hdmi_hw_t *hw)
{
    uiox_hdmi_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static int fw_hdmi_hpd_state(uiox_hdmi_hw_t *hw)
{
    uiox_hdmi_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

static void fw_hdmi_isr_hpd(uiox_hdmi_hw_t *hw)
{ (void)hw; }

static void fw_hdmi_isr_hdcp(uiox_hdmi_hw_t *hw)
{ (void)hw; }

static void fw_hdmi_isr_vblank(uiox_hdmi_hw_t *hw)
{ (void)hw; }

static void fw_hdmi_isr_audio(uiox_hdmi_hw_t *hw)
{ (void)hw; }

static const uiox_hdmi_hw_ops_t s_hdmi_fwops = {
    .init          = fw_hdmi_init,
    .deinit        = fw_hdmi_deinit,
    .enable        = fw_hdmi_enable,
    .disable       = fw_hdmi_disable,
    .set_timing    = fw_hdmi_set_timing,
    .set_colorspace= fw_hdmi_set_colorspace,
    .set_audio     = fw_hdmi_set_audio,
    .set_frl_rate  = fw_hdmi_set_frl_rate,
    .phy_power     = fw_hdmi_phy_power,
    .pll_set       = fw_hdmi_pll_set,
    .wait_vblank   = fw_hdmi_wait_vblank,
    .flip          = fw_hdmi_flip,
    .ddc_read      = fw_hdmi_ddc_read,
    .ddc_write     = fw_hdmi_ddc_write,
    .hdcp_start    = fw_hdmi_hdcp_start,
    .hdcp_stop     = fw_hdmi_hdcp_stop,
    .hdcp_status   = fw_hdmi_hdcp_status,
    .cec_send      = fw_hdmi_cec_send,
    .cec_recv      = fw_hdmi_cec_recv,
    .infoframe_send= fw_hdmi_infoframe_send,
    .audio_write   = fw_hdmi_audio_write,
    .hpd_state     = fw_hdmi_hpd_state,
    .isr_hpd       = fw_hdmi_isr_hpd,
    .isr_hdcp      = fw_hdmi_isr_hdcp,
    .isr_vblank    = fw_hdmi_isr_vblank,
    .isr_audio     = fw_hdmi_isr_audio,
};

/* ---- binding entry points (one per 02_FwHal chip helper) ---- */
int uiox_hdmi_fwbind   (uiox_hdmi_hw_t *hw, uiox_i2c_dev_t *i2c)
{
    if (!hw || !i2c) return -EINVAL;
    uiox_hdmi_fwbind_t *b = NULL;
    for (uint32_t i = 0u; i < UIOX_HDMI_FWBIND_MAX; i++)
        if (!s_hdmi_bind[i].in_use) { b = &s_hdmi_bind[i]; break; }
    if (!b) return -ENOSPC;
    b->in_use = true;
    b->i2c = *i2c;
    b->hw  = hw;
    uiox_fw_err_t frc = uiox_fw_hdmi_init(&b->fw, i2c);
    if (frc != UIOX_FW_OK) { b->in_use = false; return uiox_fw_rc(frc); }
    hw->base = (uintptr_t)b;
    int rc = uiox_hdmi_hw_init(hw, &s_hdmi_fwops);
    if (rc < 0) { b->in_use = false; return rc; }
    return 0;
}
