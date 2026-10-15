/**
 * @file    uiox_hdmi_fwops.c
 * @brief   Adapter: 02_FwHal implementation -> layer-20 uiox_hdmi_hw_ops_t.
 *
 * PATH   30_KIX/34_DSS/20_DriverInterfaces/01_Com/hdmi/uiox_hdmi_fwops.c
 * LAYER  20_DriverInterfaces -- hardware-control interface.
 *
 * BOUNDARY
 *   Above: 30_DeviceDrivers/uiox_hdmi_if.c calls through hw->ops and never
 *          sees a firmware symbol.
 *   Below: 02_FwHal builds to libuiox_fw_<arch>.a; this file calls its flat
 *          uiox_fw_hdmi_*() API and never includes its sources.
 *   This file is the joint.
 *
 * REGENERATED -- every stub carries the exact signature of its vtable slot
 * (26 slots).  Bodies return -ENOSYS: an honest "not yet
 * bound", never a silent success.  Every slot parameter is explicitly cast
 * to void so the strict -Werror set stays intact for the driver units that
 * do carry logic.
 */

#include "uiox_hdmi_hw.h"
#include "uiox_fw_rc.h"
#include "uiox_fw_hdmi.h"
#include "uiox_fw_i2c.h"

#define UIOX_HDMI_FWBIND_MAX   2u

typedef struct {
    uiox_hdmi_dev_t  fw;
    uiox_i2c_dev_t     i2c;
    uiox_hdmi_hw_t             *hw;
    bool                in_use;
} uiox_hdmi_fwbind_t;

static uiox_hdmi_fwbind_t s_hdmi_bind[UIOX_HDMI_FWBIND_MAX];

static uiox_hdmi_fwbind_t *bind_of(uiox_hdmi_hw_t *hw)
{
    if (!hw) return NULL;
    for (uint32_t i = 0u; i < UIOX_HDMI_FWBIND_MAX; i++)
        if (s_hdmi_bind[i].in_use && s_hdmi_bind[i].hw == hw)
            return &s_hdmi_bind[i];
    return NULL;
}

/* ---- vtable slots ---- */
static int fw_hdmi_init(uiox_hdmi_hw_t *hw)
{
    (void)hw;
    uiox_hdmi_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

/* deinit: IRQ/void slot -- the firmware layer owns the IRQ controller
 * but does not route device interrupts into class handlers
 * (10_BSP/10_Arch/[*]/arch_runtime.c). */
static void fw_hdmi_deinit(uiox_hdmi_hw_t *hw) { (void)hw; }
static int fw_hdmi_enable(uiox_hdmi_hw_t *hw)
{
    (void)hw;
    uiox_hdmi_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

/* disable: IRQ/void slot -- the firmware layer owns the IRQ controller
 * but does not route device interrupts into class handlers
 * (10_BSP/10_Arch/[*]/arch_runtime.c). */
static void fw_hdmi_disable(uiox_hdmi_hw_t *hw) { (void)hw; }
static int fw_hdmi_set_timing(uiox_hdmi_hw_t *hw, const uiox_hdmi_timing_t *t)
{
    (void)hw; (void)t;
    uiox_hdmi_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_hdmi_set_colorspace(uiox_hdmi_hw_t *hw, uiox_hdmi_colorspace_t cs, uiox_hdmi_bpc_t bpc)
{
    (void)hw; (void)cs; (void)bpc;
    uiox_hdmi_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_hdmi_set_audio(uiox_hdmi_hw_t *hw, const uiox_hdmi_audio_cfg_t *a)
{
    (void)hw; (void)a;
    uiox_hdmi_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_hdmi_set_frl_rate(uiox_hdmi_hw_t *hw, uiox_hdmi_frl_rate_t rate)
{
    (void)hw; (void)rate;
    uiox_hdmi_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_hdmi_phy_power(uiox_hdmi_hw_t *hw, bool on)
{
    (void)hw; (void)on;
    uiox_hdmi_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_hdmi_pll_set(uiox_hdmi_hw_t *hw, uint32_t pixel_clk_khz)
{
    (void)hw; (void)pixel_clk_khz;
    uiox_hdmi_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_hdmi_wait_vblank(uiox_hdmi_hw_t *hw, uint32_t timeout_ms)
{
    (void)hw; (void)timeout_ms;
    uiox_hdmi_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_hdmi_flip(uiox_hdmi_hw_t *hw, uintptr_t phys, uint32_t stride)
{
    (void)hw; (void)phys; (void)stride;
    uiox_hdmi_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_hdmi_ddc_read(uiox_hdmi_hw_t *hw, uint8_t dev_addr, uint8_t reg, uint8_t *buf, uint16_t len)
{
    (void)hw; (void)dev_addr; (void)reg; (void)buf; (void)len;
    uiox_hdmi_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_hdmi_ddc_write(uiox_hdmi_hw_t *hw, uint8_t dev_addr, uint8_t reg, const uint8_t *buf, uint16_t len)
{
    (void)hw; (void)dev_addr; (void)reg; (void)buf; (void)len;
    uiox_hdmi_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_hdmi_hdcp_start(uiox_hdmi_hw_t *hw, uint8_t version)
{
    (void)hw; (void)version;
    uiox_hdmi_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

/* hdcp_stop: IRQ/void slot -- the firmware layer owns the IRQ controller
 * but does not route device interrupts into class handlers
 * (10_BSP/10_Arch/[*]/arch_runtime.c). */
static void fw_hdmi_hdcp_stop(uiox_hdmi_hw_t *hw) { (void)hw; }
static int fw_hdmi_hdcp_status(uiox_hdmi_hw_t *hw, uiox_hdcp_state_t *state_out)
{
    (void)hw; (void)state_out;
    uiox_hdmi_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_hdmi_cec_send(uiox_hdmi_hw_t *hw, uint8_t dst_la, const uint8_t *msg, uint8_t len)
{
    (void)hw; (void)dst_la; (void)msg; (void)len;
    uiox_hdmi_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_hdmi_cec_recv(uiox_hdmi_hw_t *hw, uint8_t *src_la, uint8_t *msg, uint8_t *len)
{
    (void)hw; (void)src_la; (void)msg; (void)len;
    uiox_hdmi_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_hdmi_infoframe_send(uiox_hdmi_hw_t *hw, const uint8_t *packet, uint8_t len)
{
    (void)hw; (void)packet; (void)len;
    uiox_hdmi_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static int fw_hdmi_audio_write(uiox_hdmi_hw_t *hw, const uint8_t *samples, uint32_t bytes)
{
    (void)hw; (void)samples; (void)bytes;
    uiox_hdmi_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}
static bool fw_hdmi_hpd_state(uiox_hdmi_hw_t *hw)
{
    (void)hw;
    uiox_hdmi_fwbind_t *b = bind_of(hw);
    if (!b) return -ENODEV;
    return -ENOSYS;   /* TODO: bind 02_FwHal call */
}

/* isr_hpd: IRQ/void slot -- the firmware layer owns the IRQ controller
 * but does not route device interrupts into class handlers
 * (10_BSP/10_Arch/[*]/arch_runtime.c). */
static void fw_hdmi_isr_hpd(uiox_hdmi_hw_t *hw) { (void)hw; }

/* isr_hdcp: IRQ/void slot -- the firmware layer owns the IRQ controller
 * but does not route device interrupts into class handlers
 * (10_BSP/10_Arch/[*]/arch_runtime.c). */
static void fw_hdmi_isr_hdcp(uiox_hdmi_hw_t *hw) { (void)hw; }

/* isr_vblank: IRQ/void slot -- the firmware layer owns the IRQ controller
 * but does not route device interrupts into class handlers
 * (10_BSP/10_Arch/[*]/arch_runtime.c). */
static void fw_hdmi_isr_vblank(uiox_hdmi_hw_t *hw) { (void)hw; }

/* isr_audio: IRQ/void slot -- the firmware layer owns the IRQ controller
 * but does not route device interrupts into class handlers
 * (10_BSP/10_Arch/[*]/arch_runtime.c). */
static void fw_hdmi_isr_audio(uiox_hdmi_hw_t *hw) { (void)hw; }

static const uiox_hdmi_hw_ops_t s_hdmi_fwops = {
    .init           = fw_hdmi_init,
    .deinit         = fw_hdmi_deinit,
    .enable         = fw_hdmi_enable,
    .disable        = fw_hdmi_disable,
    .set_timing     = fw_hdmi_set_timing,
    .set_colorspace = fw_hdmi_set_colorspace,
    .set_audio      = fw_hdmi_set_audio,
    .set_frl_rate   = fw_hdmi_set_frl_rate,
    .phy_power      = fw_hdmi_phy_power,
    .pll_set        = fw_hdmi_pll_set,
    .wait_vblank    = fw_hdmi_wait_vblank,
    .flip           = fw_hdmi_flip,
    .ddc_read       = fw_hdmi_ddc_read,
    .ddc_write      = fw_hdmi_ddc_write,
    .hdcp_start     = fw_hdmi_hdcp_start,
    .hdcp_stop      = fw_hdmi_hdcp_stop,
    .hdcp_status    = fw_hdmi_hdcp_status,
    .cec_send       = fw_hdmi_cec_send,
    .cec_recv       = fw_hdmi_cec_recv,
    .infoframe_send = fw_hdmi_infoframe_send,
    .audio_write    = fw_hdmi_audio_write,
    .hpd_state      = fw_hdmi_hpd_state,
    .isr_hpd        = fw_hdmi_isr_hpd,
    .isr_hdcp       = fw_hdmi_isr_hdcp,
    .isr_vblank     = fw_hdmi_isr_vblank,
    .isr_audio      = fw_hdmi_isr_audio,
};

/* ---- binding entry point ---- */
int uiox_hdmi_fwbind(uiox_hdmi_hw_t *hw, struct uiox_i2c_dev *i2c)
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
    int rc = uiox_hdmi_hw_init(hw, &s_hdmi_fwops);
    if (rc < 0) { b->in_use = false; return rc; }
    return 0;
}
