/**
 * @file    uiox_hdmi_if.c
 * @brief   UIOX HDMI interface driver implementation.
 * @date    2026-05-28
 */

 #include "uiox_hdmi_if.h"
 #include <string.h>
 #include <errno.h>
 
 /* =========================================================================
  * N/CTS computation (IEC 60958 / CEA-861)
  * For a given pixel clock and audio sample rate, N is chosen from the
  * standard table; CTS = (pixel_clk_khz * N) / (128 * sample_rate_hz/1000)
  * ====================================================================== */
 
 uiox_hdmi_acr_t uiox_hdmi_if_compute_acr(uint32_t pixel_clk_khz,
                                            uint32_t sample_rate_hz)
 {
     uiox_hdmi_acr_t acr = {0, 0};
 
     /* Standard N values per CEA-861 Table 7-1 */
     uint32_t N;
     switch (sample_rate_hz) {
     case 32000:  N = 4096u;  break;
     case 44100:  N = 6272u;  break;
     case 48000:  N = 6144u;  break;
     case 88200:  N = 12544u; break;
     case 96000:  N = 12288u; break;
     case 176400: N = 25088u; break;
     case 192000: N = 24576u; break;
     default:     N = 6144u;  break;
     }
 
     /* CTS = (pixel_clk_khz × 1000 × N) / (128 × sample_rate_hz)
      *     = (pixel_clk_khz × N) / (128 × sample_rate_hz / 1000)      */
     uint64_t cts_num = (uint64_t)pixel_clk_khz * 1000ULL * N;
     uint64_t cts_den = 128ULL * sample_rate_hz;
     acr.N   = N;
     acr.CTS = (uint32_t)(cts_num / cts_den);
     return acr;
 }
 
 int uiox_hdmi_if_config(uiox_hdmi_if_t        *hif,
                          uiox_hdmi_hw_t        *hw,
                          const uiox_hdmi_timing_t *timing,
                          uiox_hdmi_colorspace_t cs,
                          uiox_hdmi_bpc_t        bpc)
 {
     if (!hif || !hw || !timing) return -EINVAL;
     memset(hif, 0, sizeof(*hif));
     hif->hw  = hw;
     hif->cs  = cs;
     hif->bpc = bpc;
     memcpy(&hif->timing, timing, sizeof(*timing));
 
     /* Select TMDS or FRL based on pixel clock */
     if (timing->pixel_clk_khz > 600000u &&
         (hw->caps & UIOX_HDMI_CAP_FRL)) {
         hif->link     = UIOX_HDMI_LINK_FRL;
         hif->frl_rate = UIOX_HDMI_FRL_12G4L;
     } else {
         hif->link     = UIOX_HDMI_LINK_TMDS;
     }
 
     /* Enable scrambling for TMDS > 340 MHz (HDMI 2.0) */
     hif->scrambling = (timing->pixel_clk_khz > 340000u);
 
     /* Program pixel clock */
     const uiox_hdmi_hw_ops_t *ops = (const uiox_hdmi_hw_ops_t *)hw->priv;
     if (ops && ops->pll_set) {
         int rc = ops->pll_set(hw, timing->pixel_clk_khz);
         if (rc < 0) return rc;
     }
 
     /* Programme timing */
     int rc = uiox_hdmi_hw_set_timing(hw, timing);
     if (rc < 0) return rc;
 
     /* Colour space */
     if (ops && ops->set_colorspace) {
         rc = ops->set_colorspace(hw, cs, bpc);
         if (rc < 0) return rc;
     }
 
     /* Default audio: 2ch LPCM 48kHz 16-bit */
     hif->audio.fmt             = UIOX_HDMI_AUDIO_LPCM;
     hif->audio.sample_rate_hz  = 48000u;
     hif->audio.channels        = 2u;
     hif->audio.bits_per_sample = 16u;
     hif->acr = uiox_hdmi_if_compute_acr(timing->pixel_clk_khz, 48000u);
 
     /* Initialise framebuffer pool */
     uint8_t bpp = (bpc == UIOX_HDMI_BPC_8) ? 4u : 8u;
     uiox_hdmi_buf_init(timing->h_active, timing->v_active,
                         (uint32_t)timing->h_active * bpp, cs, bpc);
     return 0;
 }
 
 int uiox_hdmi_if_enable(uiox_hdmi_if_t *hif)
 {
     if (!hif || !hif->hw) return -EINVAL;
     const uiox_hdmi_hw_ops_t *ops =
         (const uiox_hdmi_hw_ops_t *)hif->hw->priv;
 
     /* PHY power on */
     if (ops && ops->phy_power) {
         int rc = ops->phy_power(hif->hw, true);
         if (rc < 0) return rc;
     }
 
     /* FRL training if required */
     if (hif->link == UIOX_HDMI_LINK_FRL && ops && ops->set_frl_rate) {
         int rc = ops->set_frl_rate(hif->hw, hif->frl_rate);
         if (rc < 0) return rc;
     }
 
     int rc = uiox_hdmi_hw_enable(hif->hw);
     if (rc == 0) hif->enabled = true;
     return rc;
 }
 
 void uiox_hdmi_if_disable(uiox_hdmi_if_t *hif)
 {
     if (!hif || !hif->hw) return;
     uiox_hdmi_hw_disable(hif->hw);
     const uiox_hdmi_hw_ops_t *ops =
         (const uiox_hdmi_hw_ops_t *)hif->hw->priv;
     if (ops && ops->phy_power) ops->phy_power(hif->hw, false);
     hif->enabled = false;
 }
 
 int uiox_hdmi_if_flip(uiox_hdmi_if_t *hif, uiox_hdmi_fb_t *fb)
 {
     if (!hif || !fb || !hif->enabled) return -EINVAL;
     fb->state = UIOX_HDMI_FB_PENDING;
     int rc = uiox_hdmi_hw_flip(hif->hw, fb->paddr, fb->stride);
     if (rc < 0) { fb->state = UIOX_HDMI_FB_RENDERING; return rc; }
     hif->stats.frame_count++;
     return 0;
 }
 
 int uiox_hdmi_if_vsync(uiox_hdmi_if_t *hif, uint32_t timeout_ms)
 {
     if (!hif) return -EINVAL;
     int rc = uiox_hdmi_hw_wait_vblank(hif->hw, timeout_ms);
     if (rc == 0) hif->stats.vblank_count++;
     return rc;
 }
 
 int uiox_hdmi_if_set_audio(uiox_hdmi_if_t             *hif,
                             const uiox_hdmi_audio_cfg_t *a)
 {
     if (!hif || !a) return -EINVAL;
     memcpy(&hif->audio, a, sizeof(*a));
     hif->acr = uiox_hdmi_if_compute_acr(hif->timing.pixel_clk_khz,
                                           a->sample_rate_hz);
     const uiox_hdmi_hw_ops_t *ops =
         (const uiox_hdmi_hw_ops_t *)hif->hw->priv;
     if (ops && ops->set_audio) return ops->set_audio(hif->hw, a);
     return 0;
 }
 
 int uiox_hdmi_if_audio_write(uiox_hdmi_if_t *hif,
                               const uint8_t *samples, uint32_t bytes)
 {
     if (!hif || !samples) return -EINVAL;
     const uiox_hdmi_hw_ops_t *ops =
         (const uiox_hdmi_hw_ops_t *)hif->hw->priv;
     if (!ops || !ops->audio_write) return -ENOSYS;
     return ops->audio_write(hif->hw, samples, bytes);
 }
 
 void uiox_hdmi_if_stats_get(const uiox_hdmi_if_t *hif,
                               uiox_hdmi_if_stats_t *out)
 {
     if (!hif || !out) return;
     memcpy(out, &hif->stats, sizeof(*out));
 }
 
 void uiox_hdmi_if_stats_reset(uiox_hdmi_if_t *hif)
 {
     if (!hif) return;
     memset(&hif->stats, 0, sizeof(hif->stats));
 }
 