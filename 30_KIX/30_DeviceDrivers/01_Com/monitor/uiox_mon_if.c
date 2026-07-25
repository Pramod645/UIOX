/**
 * @file    uiox_mon_if.c
 * @brief   UIOX Monitor interface driver implementation.
 * @date    2026-05-27
 */

 #include "uiox_mon_if.h"
 #include "uiox_klibc.h"
 
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
     const uiox_mon_hw_ops_t *ops = (const uiox_mon_hw_ops_t *)hw->priv;
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
 