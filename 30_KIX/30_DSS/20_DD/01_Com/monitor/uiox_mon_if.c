/**
 * @file    uiox_mon_if.c
 * @brief   UIOX Monitor interface driver implementation.
 * @date    2026-05-27
 */

 #include "uiox_mon_if.h"
 #include "uiox_klibc.h"
 #include "uiox_devclass.h"
 #include "uiox_fw_i2c.h"
 #include "uiox_mon_buf.h"     /* uiox_mon_fb_t — the flip argument */
 
 int uiox_mon_if_config(uiox_mon_if_t        *mif,
                         uiox_mon_hw_t        *hw,
                         uiox_mon_if_type_t    type,
                         const uiox_mon_timing_t *timing,
                         uiox_mon_pixfmt_t     pixfmt)
 {
     if (!mif || !hw || !timing) return -EINVAL;
     memset(mif, 0, sizeof(*mif));
     mif->hw     = hw;
     mif->type   = type;
     mif->pixfmt = pixfmt;
     memcpy(&mif->timing, timing, sizeof(*timing));
 
     /* Program pixel clock and timing into HAL */
     int rc = uiox_mon_hw_set_timing(hw, timing);
     if (rc < 0) return rc;
 
     /* Set pixel format */
     const uiox_mon_hw_ops_t *ops = (const uiox_mon_hw_ops_t *)hw->ops;
     if (ops && ops->set_pixfmt) {
         rc = ops->set_pixfmt(hw, pixfmt);
         if (rc < 0) return rc;
     }
 
     /* Interface-specific init */
     switch (type) {
     case UIOX_MON_IF_DP:
         mif->dp_lanes     = 4u;
         mif->dp_link_rate_hz = 2700000000u; /* HBR1 */
         mif->dp_link      = UIOX_DP_LINK_IDLE;
         break;
     case UIOX_MON_IF_MIPI_DSI:
         mif->dsi_lanes      = hw->mipi_lanes;
         mif->dsi_video_mode = true;
         break;
     default:
         break;
     }
 
     /* Initialise framebuffer pool */
     uint8_t bpp = (pixfmt == UIOX_MON_FMT_RGB565) ? 2u : 4u;
     uint32_t stride = (uint32_t)timing->h_active * bpp;
     uiox_mon_buf_init(timing->h_active, timing->v_active, stride, pixfmt);
 
     return 0;
 }
 
 int uiox_mon_if_enable(uiox_mon_if_t *mif)
 {
     if (!mif || !mif->hw) return -EINVAL;
     int rc = uiox_mon_hw_enable(mif->hw);
     if (rc == 0) mif->enabled = true;
     return rc;
 }
 
 void uiox_mon_if_disable(uiox_mon_if_t *mif)
 {
     if (!mif || !mif->hw) return;
     uiox_mon_hw_disable(mif->hw);
     mif->enabled = false;
 }
 
 int uiox_mon_if_flip(uiox_mon_if_t *mif, uiox_mon_fb_t *fb)
 {
     if (!mif || !fb) return -EINVAL;
     if (!mif->enabled)  return -ENETDOWN;
 
     fb->state = UIOX_MON_BUF_PENDING;
     int rc = uiox_mon_hw_flip(mif->hw, fb->paddr, fb->stride);
     if (rc < 0) {
         fb->state = UIOX_MON_BUF_RENDERING;
         return rc;
     }
 
     mif->stats.frame_count++;
     mif->stats.bytes_transferred += (uint64_t)fb->stride * fb->height;
     return 0;
 }
 
 int uiox_mon_if_vsync(uiox_mon_if_t *mif, uint32_t timeout_ms)
 {
     if (!mif || !mif->hw) return -EINVAL;
     int rc = uiox_mon_hw_wait_vblank(mif->hw, timeout_ms);
     if (rc == 0) mif->stats.vblank_count++;
     return rc;
 }
 
 void uiox_mon_if_stats_get(const uiox_mon_if_t *mif,
                             uiox_mon_if_stats_t *out)
 {
     if (!mif || !out) return;
     memcpy(out, &mif->stats, sizeof(*out));
 }
 
 void uiox_mon_if_stats_reset(uiox_mon_if_t *mif)
 {
     if (!mif) return;
     memset(&mif->stats, 0, sizeof(mif->stats));
 }
 /* ═══════════════════════════ INSERTED — registry layer ══════════════ */

static int mon_class_init(uiox_dev_dev_t *dev)
{
    uiox_mon_if_t *mif = (uiox_mon_if_t *)dev->drv_priv;
    if (!mif || !mif->hw) return -EINVAL;
    return uiox_mon_if_enable(mif);
}

static void mon_class_deinit(uiox_dev_dev_t *dev)
{
    uiox_mon_if_t *mif = (uiox_mon_if_t *)dev->drv_priv;
    if (mif) uiox_mon_if_disable(mif);
}

static int mon_class_ioctl(uiox_dev_dev_t *dev, uint32_t cmd, void *arg)
{
    uiox_mon_if_t *mif = (uiox_mon_if_t *)dev->drv_priv;
    if (!mif || !mif->hw) return -EINVAL;

    switch (cmd) {
    case UIOX_MON_IOC_ENABLE:    return uiox_mon_if_enable(mif);
    case UIOX_MON_IOC_DISABLE:
        uiox_mon_if_disable(mif);
        return 0;
    case UIOX_MON_IOC_FLIP: {
        if (!arg) return -EINVAL;
        const uiox_mon_flip_t *f = (const uiox_mon_flip_t *)arg;
        return uiox_mon_if_flip(mif, f->fb);
    }
    case UIOX_MON_IOC_VSYNC:
        if (!arg) return -EINVAL;
        return uiox_mon_if_vsync(mif, *(const uint32_t *)arg);
    case UIOX_MON_IOC_GET_INFO: {
        if (!arg) return -EINVAL;
        uiox_mon_info_t *o = (uiox_mon_info_t *)arg;
        o->base_addr   = mif->hw->base_addr;
        o->irq_vblank  = mif->hw->irq_vblank;
        o->irq_hotplug = mif->hw->irq_hotplug;
        o->type        = mif->type;
        o->dp_link     = mif->dp_link;
        o->dp_lanes    = mif->dp_lanes;
        o->dsi_lanes   = mif->dsi_lanes;
        return 0;
    }

    case UIOX_MON_IOC_GET_STATS:
        if (!arg) return -EINVAL;
        uiox_mon_if_stats_get(mif, (uiox_mon_if_stats_t *)arg);
        return 0;
    default:
        return -ENOSYS;
    }
}

const uiox_mon_ops_t uiox_mon_class_ops = {
    .head = { .name = "mon0", .probe = NULL,
              .init = mon_class_init, .deinit = mon_class_deinit,
              .ioctl = mon_class_ioctl },
};

static uiox_mon_hw_t       s_mon_hw;
static uiox_mon_if_t       s_mon_if;
static uiox_dev_dev_t      s_mon_dev;
static struct uiox_i2c_dev s_mon_i2c;

int uiox_mon_probe(void)
{
    if (uiox_mon_fwbind(&s_mon_hw, &s_mon_i2c) < 0)  return -1;

    /* if_type / timing / pixfmt come off the HAL — no defaults invented
     * here (uiox_mon_hw.h:139 and the timing/pixfmt enums above it). */
    if (uiox_mon_if_config(&s_mon_if, &s_mon_hw,
                           s_mon_hw.if_type,
                           &s_mon_hw.timing,
                           s_mon_hw.pixfmt) < 0)
        return -1;

    s_mon_dev.cls  = UIOX_DEVCLASS_MONITOR;
    s_mon_dev.unit = 0u;
    s_mon_dev.bus  = UIOX_BUS_MMIO;
    s_mon_dev.base = s_mon_hw.base_addr;
    s_mon_dev.addr = 0u;
    //s_mon_dev.irq  = s_mon_hw.irq;
    s_mon_dev.irq  = s_mon_hw.irq_vblank;   /* the per-frame line */
    s_mon_dev.name = "mon0";
    s_mon_dev.ops  = &uiox_mon_class_ops;
    s_mon_dev.drv_priv = &s_mon_if;

    return uiox_dev_bind(&s_mon_dev);
}
/* ═══════════════════════════ END INSERT ═════════════════════════ */
