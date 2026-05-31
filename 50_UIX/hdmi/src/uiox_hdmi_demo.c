/**
 * @file    uiox_hdmi_demo.c
 * @brief   UIOX HDMI stack end-to-end demonstration.
 *
 * Demonstrates the complete flow:
 *   HAL init → HPD detect → EDID read → mode select →
 *   TMDS/FRL config → infoframe TX → HDR metadata →
 *   audio config → CEC → framebuffer render/flip → HDCP → teardown.
 *
 * Uses stub HAL ops — replace with real HDMI TX driver.
 * @date    2026-05-28
 */
//Demo Application
 #include "uiox_hdmi_device.h"
 #include <stdio.h>
 #include <string.h>
 #include <stdint.h>
 #include <math.h>
 #include <errno.h>
 
 /* =========================================================================
  * Fake EDID — simulates an HDMI 2.0 4K HDR10 sink
  * ====================================================================== */
 
 static uint8_t s_fake_edid[256];
 
 static void build_fake_edid(void)
 {
     memset(s_fake_edid, 0, sizeof(s_fake_edid));
 
     /* EDID header */
     uint8_t *b = s_fake_edid;
     static const uint8_t hdr[8] = {0,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0};
     memcpy(b, hdr, 8);
 
     /* Manufacturer: "UIO" = U(21) I(9) O(15) */
     uint16_t mfr = (uint16_t)(((21u & 0x1Fu) << 10u) |
                                ((9u  & 0x1Fu) <<  5u) |
                                 (15u & 0x1Fu));
     b[8]  = (uint8_t)(mfr >> 8u);
     b[9]  = (uint8_t)(mfr & 0xFFu);
     b[10] = 0x01u; b[11] = 0x00u;  /* Product code */
     b[18] = 1u;                     /* EDID version 1 */
     b[19] = 4u;                     /* Revision 4 */
     b[20] = 0xA5u;                  /* Digital input, 8 bpc */
     b[21] = 60u;                    /* H size 60 cm */
     b[22] = 34u;                    /* V size 34 cm */
     b[23] = 120u;                   /* Gamma 2.2 × 100 - 100 */
 
     /* DTD 0: 3840×2160@60Hz (pixel clock ≈ 594 MHz → 59400 × 10 kHz) */
     uint8_t *dtd = &b[54];
     uint16_t clk = 59400u;         /* ×10 kHz = 594 MHz */
     dtd[0] = (uint8_t)(clk & 0xFF);
     dtd[1] = (uint8_t)(clk >> 8u);
     /* h_active=3840, h_blank=560 */
     dtd[2] = (uint8_t)(3840u & 0xFFu);
     dtd[3] = (uint8_t)(560u  & 0xFFu);
     dtd[4] = (uint8_t)(((3840u >> 4u) & 0xF0u) | ((560u >> 8u) & 0x0Fu));
     /* v_active=2160, v_blank=90 */
     dtd[5] = (uint8_t)(2160u & 0xFFu);
     dtd[6] = (uint8_t)(90u   & 0xFFu);
     dtd[7] = (uint8_t)(((2160u >> 4u) & 0xF0u) | ((90u >> 8u) & 0x0Fu));
     /* Porches: h_fp=176, h_sync=88, v_fp=8, v_sync=10 */
     dtd[8]  = 176u; dtd[9]  = 88u;
     dtd[10] = (uint8_t)((8u << 4u) | 10u);
     dtd[11] = 0x00u;
     dtd[16] = 60u; dtd[17] = 0x1Eu; /* H/V size + flags: +H +V */
 
     /* Monitor name descriptor */
     uint8_t *nm = &b[72];
     nm[0]=0; nm[1]=0; nm[2]=0; nm[3]=0xFCu; nm[4]=0;
     memcpy(&nm[5], "UIOX 4K HDR\x0A  ", 13);
 
     /* Extension flag */
     b[126] = 1u;
 
     /* Fix block 0 checksum */
     uint8_t sum = 0;
     for (int i = 0; i < 127; i++) sum += b[i];
     b[127] = (uint8_t)(256u - sum);
 
     /* CEA-861 extension block */
     uint8_t *ext = &s_fake_edid[128];
     ext[0] = 0x02u;  /* CEA extension */
     ext[1] = 0x03u;  /* CEA revision 3 */
     ext[2] = 0x20u;  /* DTD offset = 32 (data blocks end at 31) */
     ext[3] = 0x70u;  /* YCbCr 4:4:4 + 4:2:2 + 2 native DTDs */
 
     /* Audio data block: 2ch LPCM 32/44.1/48 kHz 16/20/24-bit */
     ext[4]  = (uint8_t)(0x23u);  /* tag=1 (audio), len=3 */
     ext[5]  = 0x09u;             /* format=1(LPCM), ch=2 */
     ext[6]  = 0x07u;             /* 32k+44.1k+48k */
     ext[7]  = 0x07u;             /* 16+20+24 bit */
 
     /* Video data block: VIC 97 (4K@60) */
     ext[8]  = (uint8_t)(0x42u);  /* tag=2 (video), len=2 */
     ext[9]  = 97u;               /* VIC 97 = 3840×2160@60 */
     ext[10] = 96u;               /* VIC 96 = 3840×2160@50 */
 
     /* HDMI Forum VSDB for HDMI 2.0 */
     ext[11] = (uint8_t)(0x77u);  /* tag=3 (vendor), len=7 */
     ext[12] = 0xD8u; ext[13] = 0x5Du; ext[14] = 0xC4u; /* IEEE OUI */
     ext[15] = 0x01u;             /* Physical address 1.0.0.0 */
     ext[16] = 0x00u;
     ext[17] = 0x78u;             /* max TMDS = 600 MHz (120 × 5) */
     ext[18] = 0x00u;
 
     /* HDR Static Metadata extended tag */
     ext[19] = (uint8_t)(0xE6u);  /* tag=7 (extended), len=6 */
     ext[20] = 0x06u;             /* extended tag = HDR Static Metadata */
     ext[21] = 0x0Fu;             /* EOTF: SDR+HDR+ST2084+HLG */
     ext[22] = 0x01u;             /* Static Metadata type 1 */
     ext[23] = 0x00u;
     ext[24] = 0x00u;
     ext[25] = 0x00u;
 
     /* Speaker allocation */
     ext[26] = 0x83u;             /* tag=4, len=3 */
     ext[27] = 0x01u;             /* FL/FR */
     ext[28] = 0x00u;
     ext[29] = 0x00u;
 
     /* CEA block 0 checksum */
     uint8_t esum = 0;
     for (int i = 128; i < 255; i++) esum += s_fake_edid[i];
     s_fake_edid[255] = (uint8_t)(256u - esum);
 }
 
 /* =========================================================================
  * Stub HAL ops
  * ====================================================================== */
 
 static int stub_init(uiox_hdmi_hw_t *hw)
 {
     (void)hw;
     printf("  [hal] init  HDMI TX  base=0x%08lX\n",
            (unsigned long)hw->base_addr);
     hw->connected = true;
     return 0;
 }
 
 static void stub_deinit (uiox_hdmi_hw_t *hw) { (void)hw; printf("  [hal] deinit\n"); }
 static int  stub_enable (uiox_hdmi_hw_t *hw) { (void)hw; printf("  [hal] enable\n"); return 0; }
 static void stub_disable(uiox_hdmi_hw_t *hw) { (void)hw; printf("  [hal] disable\n"); }
 
 static int stub_set_timing(uiox_hdmi_hw_t *hw, const uiox_hdmi_timing_t *t)
 {
     (void)hw;
     printf("  [hal] timing  %ux%u@%u Hz  pclk=%u kHz  VIC=%u\n",
            t->h_active, t->v_active, t->refresh_hz,
            t->pixel_clk_khz, t->vic);
     return 0;
 }
 
 static int stub_set_colorspace(uiox_hdmi_hw_t *hw,
                                 uiox_hdmi_colorspace_t cs,
                                 uiox_hdmi_bpc_t bpc)
 {
     (void)hw;
     static const char *cs_names[] = {"RGB","YCbCr444","YCbCr422","YCbCr420"};
     static const uint8_t bpc_vals[] = {8,10,12,16};
     printf("  [hal] colorspace=%s  bpc=%u\n",
            cs_names[cs], bpc_vals[bpc]);
     return 0;
 }
 
 static int stub_set_audio(uiox_hdmi_hw_t *hw,
                            const uiox_hdmi_audio_cfg_t *a)
 {
     (void)hw;
     printf("  [hal] audio  %u Hz  %u ch  %u bps  fmt=%u\n",
            a->sample_rate_hz, a->channels, a->bits_per_sample, a->fmt);
     return 0;
 }
 
 static int stub_set_frl_rate(uiox_hdmi_hw_t *hw, uiox_hdmi_frl_rate_t r)
 {
     (void)hw;
     static const char *rates[] = {"3G3L","6G3L","6G4L","8G4L","10G4L","12G4L"};
     printf("  [hal] FRL rate = %s\n", rates[r]);
     return 0;
 }
 
 static int stub_phy_power(uiox_hdmi_hw_t *hw, bool on)
 {
     (void)hw;
     printf("  [hal] PHY %s\n", on ? "ON" : "OFF");
     return 0;
 }
 
 static int stub_pll_set(uiox_hdmi_hw_t *hw, uint32_t clk_khz)
 {
     (void)hw;
     printf("  [hal] PLL → %u kHz  (%.3f MHz)\n",
            clk_khz, (double)clk_khz / 1000.0);
     return 0;
 }
 
 static uint32_t s_vblank = 0;
 static int stub_wait_vblank(uiox_hdmi_hw_t *hw, uint32_t timeout_ms)
 {
     (void)hw; (void)timeout_ms;
     hw->vblank_count++;
     s_vblank++;
     return 0;
 }
 
 static uint32_t s_flip_count = 0;
 static int stub_flip(uiox_hdmi_hw_t *hw, uintptr_t phys, uint32_t stride)
 {
     (void)hw;
     printf("  [hal] flip  phys=0x%08lX  stride=%u  (#%u)\n",
            (unsigned long)phys, stride, ++s_flip_count);
     return 0;
 }
 
 static int stub_ddc_read(uiox_hdmi_hw_t *hw, uint8_t dev_addr,
                           uint8_t reg, uint8_t *buf, uint16_t len)
 {
     (void)hw;
     printf("  [hal] DDC read  addr=0x%02X  reg=0x%02X  len=%u\n",
            dev_addr, reg, len);
     uint16_t offset = (reg == 0x80u) ? 128u : 0u;
     uint16_t copy   = (len < (uint16_t)(256u - offset)) ?
                        len : (uint16_t)(256u - offset);
     memcpy(buf, &s_fake_edid[offset], copy);
     return 0;
 }
 
 static int stub_ddc_write(uiox_hdmi_hw_t *hw, uint8_t dev_addr,
                            uint8_t reg, const uint8_t *buf, uint16_t len)
 { (void)hw; (void)dev_addr; (void)reg; (void)buf; (void)len; return 0; }
 
 static uiox_hdcp_state_t s_hdcp = UIOX_HDCP_DISABLED;
 static int stub_hdcp_start(uiox_hdmi_hw_t *hw, uint8_t ver)
 {
     (void)hw;
     printf("  [hal] HDCP %u start\n", ver);
     s_hdcp = UIOX_HDCP_AUTHENTICATED;  /* immediate success in sim */
     return 0;
 }
 
 static void stub_hdcp_stop(uiox_hdmi_hw_t *hw)
 { (void)hw; s_hdcp = UIOX_HDCP_DISABLED; printf("  [hal] HDCP stop\n"); }
 
 static int stub_hdcp_status(uiox_hdmi_hw_t *hw, uiox_hdcp_state_t *out)
 { (void)hw; *out = s_hdcp; return 0; }
 
 static int stub_cec_send(uiox_hdmi_hw_t *hw, uint8_t dst_la,
                           const uint8_t *msg, uint8_t len)
 {
     (void)hw;
     printf("  [hal] CEC TX → LA=0x%X  opcode=0x%02X  len=%u\n",
            dst_la, len >= 2 ? msg[1] : 0, len);
     return 0;
 }
 
 static int stub_cec_recv(uiox_hdmi_hw_t *hw, uint8_t *src_la,
                           uint8_t *msg, uint8_t *len)
 { (void)hw; *src_la = 0; *len = 0; return -EAGAIN; }
 
 static uint32_t s_if_count = 0;
 static int stub_infoframe_send(uiox_hdmi_hw_t *hw,
                                 const uint8_t *pkt, uint8_t len)
 {
     (void)hw;
     printf("  [hal] infoframe  type=0x%02X  len=%u  (#%u)\n",
            pkt[0], len, ++s_if_count);
     return 0;
 }
 
 static int stub_audio_write(uiox_hdmi_hw_t *hw,
                              const uint8_t *samples, uint32_t bytes)
 {
     (void)hw; (void)samples;
     printf("  [hal] audio write  %u bytes\n", bytes);
     return 0;
 }
 
 static bool stub_hpd_state(uiox_hdmi_hw_t *hw)
 { (void)hw; return true; }
 
 static void stub_isr_hpd    (uiox_hdmi_hw_t *hw) { (void)hw; }
 static void stub_isr_hdcp   (uiox_hdmi_hw_t *hw) { (void)hw; }
 static void stub_isr_vblank (uiox_hdmi_hw_t *hw) { (void)hw; }
 static void stub_isr_audio  (uiox_hdmi_hw_t *hw) { (void)hw; }
 
 static const uiox_hdmi_hw_ops_t stub_ops = {
     .init           = stub_init,
     .deinit         = stub_deinit,
     .enable         = stub_enable,
     .disable        = stub_disable,
     .set_timing     = stub_set_timing,
     .set_colorspace = stub_set_colorspace,
     .set_audio      = stub_set_audio,
     .set_frl_rate   = stub_set_frl_rate,
     .phy_power      = stub_phy_power,
     .pll_set        = stub_pll_set,
     .wait_vblank    = stub_wait_vblank,
     .flip           = stub_flip,
     .ddc_read       = stub_ddc_read,
     .ddc_write      = stub_ddc_write,
     .hdcp_start     = stub_hdcp_start,
     .hdcp_stop      = stub_hdcp_stop,
     .hdcp_status    = stub_hdcp_status,
     .cec_send       = stub_cec_send,
     .cec_recv       = stub_cec_recv,
     .infoframe_send = stub_infoframe_send,
     .audio_write    = stub_audio_write,
     .hpd_state      = stub_hpd_state,
     .isr_hpd        = stub_isr_hpd,
     .isr_hdcp       = stub_isr_hdcp,
     .isr_vblank     = stub_isr_vblank,
     .isr_audio      = stub_isr_audio,
 };
 
 /* =========================================================================
  * Hardware device instance
  * ====================================================================== */
 
 static uiox_hdmi_hw_t s_hw = {
     .base_addr  = 0x58000000uL,
     .irq_hpd    = 48,
     .irq_hdcp   = 49,
     .irq_vblank = 50,
     .irq_audio  = 51,
     .caps       = UIOX_HDMI_CAP_HDMI20  |
                   UIOX_HDMI_CAP_HDCP14  |
                   UIOX_HDMI_CAP_CEC     |
                   UIOX_HDMI_CAP_ARC     |
                   UIOX_HDMI_CAP_HDR10   |
                   UIOX_HDMI_CAP_VRR     |
                   UIOX_HDMI_CAP_ALLM    |
                   UIOX_HDMI_CAP_AUDIO_HBR,
     .version    = UIOX_HDMI_VER_20,
     .pll_ref_hz = 27000000u,
 };
 
 /* =========================================================================
  * Event callback
  * ====================================================================== */
 
 static void on_hdmi_event(uiox_hdmi_evt_t evt, void *ctx)
 {
     (void)ctx;
     printf("  [event] %s\n", uiox_hdmi_evt_name(evt));
 }
 
 /* =========================================================================
  * Test pattern generators
  * ====================================================================== */
 
 static void draw_solid(uiox_hdmi_fb_t *fb, uint32_t colour)
 {
     uiox_hdmi_buf_clear_fb(fb, colour);
 }
 
 static void draw_colour_bars(uiox_hdmi_fb_t *fb)
 {
     static const uint32_t bars[8] = {
         0xFFFFFFFF, 0xFFFFFF00, 0xFF00FFFF, 0xFF00FF00,
         0xFFFF00FF, 0xFFFF0000, 0xFF0000FF, 0xFF000000,
     };
     uint32_t *px = (uint32_t *)fb->vaddr;
     uint16_t bar_w = fb->width / 8u;
     for (uint16_t y = 0; y < fb->height; y++)
         for (uint16_t x = 0; x < fb->width; x++) {
             uint8_t b = (uint8_t)(x / bar_w);
             if (b > 7u) b = 7u;
             px[(uint32_t)y * fb->width + x] = bars[b];
         }
 }
 
 /* =========================================================================
  * main
  * ====================================================================== */
 
 int main(void)
 {
     printf("=== UIOX HDMI Stack Demo ===\n\n");
 
     /* Build fake EDID before any DDC reads */
     build_fake_edid();
 
     /* ------------------------------------------------------------------ */
     /* 1. Open device                                                      */
     /* ------------------------------------------------------------------ */
 
     printf("--- Open ---\n");
     uiox_hdmi_device_t dev;
     uiox_hdmi_open_params_t p;
     memset(&p, 0, sizeof(p));
 
     p.hw              = &s_hw;
     p.hw_ops          = &stub_ops;
     p.cs              = UIOX_HDMI_CS_RGB;
     p.bpc             = UIOX_HDMI_BPC_8;
     p.pref_w          = 3840u;
     p.pref_h          = 2160u;
     p.pref_hz         = 60u;
     p.dpms_timeout_ms = 300000u;  /* 5 minutes */
     p.evt_cb          = on_hdmi_event;
 
     int rc = uiox_hdmi_open(&dev, &p);
     if (rc < 0) {
         printf("[error] uiox_hdmi_open failed: %d\n", rc);
         return 1;
     }
 
     /* ------------------------------------------------------------------ */
     /* 2. Print sink information                                           */
     /* ------------------------------------------------------------------ */
 
     printf("\n--- Sink information ---\n");
     uiox_hdmi_print_info(&dev);
 
     uint16_t res_w = 0, res_h = 0;
     uiox_hdmi_get_resolution(&dev, &res_w, &res_h);
     printf("  Resolution   : %u × %u\n", res_w, res_h);
     printf("  Connected    : %s\n", uiox_hdmi_connected(&dev) ? "YES" : "NO");
 
     /* ------------------------------------------------------------------ */
     /* 3. Start display                                                    */
     /* ------------------------------------------------------------------ */
 
     printf("\n--- Start ---\n");
     rc = uiox_hdmi_start(&dev);
     if (rc < 0) {
         printf("[error] uiox_hdmi_start failed: %d\n", rc);
         uiox_hdmi_close(&dev);
         return 1;
     }
     printf("  HDMI output : ACTIVE\n");
     printf("  State       : %s\n",
            uiox_hdmi_state_name(dev.subsys.state));
 
     /* ------------------------------------------------------------------ */
     /* 4. HDR10 metadata                                                   */
     /* ------------------------------------------------------------------ */
 
     printf("\n--- HDR10 metadata ---\n");
     uiox_hdmi_hdr_t hdr;
     memset(&hdr, 0, sizeof(hdr));
     hdr.eotf          = 2u;     /* SMPTE ST 2084 (PQ) */
     hdr.metadata_type = 0u;     /* Static Metadata Type 1 */
     /* BT.2020 display primaries (×0.00002) */
     hdr.display_primaries_x[0] = 17000u; /* R x = 0.708 */
     hdr.display_primaries_y[0] = 17200u; /* R y = 0.292 */
     hdr.display_primaries_x[1] = 8500u;  /* G x = 0.170 */
     hdr.display_primaries_y[1] = 39850u; /* G y = 0.797 */
     hdr.display_primaries_x[2] = 6550u;  /* B x = 0.131 */
     hdr.display_primaries_y[2] = 2300u;  /* B y = 0.046 */
     hdr.white_point_x  = 15635u;         /* D65 x = 0.3127 */
     hdr.white_point_y  = 16450u;         /* D65 y = 0.3290 */
     hdr.max_luminance  = 1000u;          /* 1000 cd/m² */
     hdr.min_luminance  = 5u;             /* 0.0005 cd/m² */
     hdr.max_cll        = 1000u;
     hdr.max_fall       = 400u;
     rc = uiox_hdmi_set_hdr(&dev, &hdr);
     printf("  HDR10 EOTF=PQ  MaxCLL=%u  MaxFALL=%u  rc=%d\n",
            hdr.max_cll, hdr.max_fall, rc);
 
     /* ------------------------------------------------------------------ */
     /* 5. Audio configuration                                              */
     /* ------------------------------------------------------------------ */
 
     printf("\n--- Audio configuration ---\n");
     uiox_hdmi_audio_cfg_t audio = {
         .fmt             = UIOX_HDMI_AUDIO_LPCM,
         .sample_rate_hz  = 48000u,
         .channels        = 2u,
         .bits_per_sample = 24u,
     };
     rc = uiox_hdmi_set_audio(&dev, &audio);
     printf("  LPCM 48kHz  2ch  24-bit  rc=%d\n", rc);
 
     /* Write some silent audio samples (1 ms of 48kHz 2ch 24-bit = 288 bytes) */
     static uint8_t silence[288];
     memset(silence, 0, sizeof(silence));
     rc = uiox_hdmi_audio_write(&dev, silence, sizeof(silence));
     printf("  Audio write  %zu bytes  rc=%d\n", sizeof(silence), rc);
 
     /* ------------------------------------------------------------------ */
     /* 6. CEC commands                                                     */
     /* ------------------------------------------------------------------ */
 
     printf("\n--- CEC ---\n");
 
     /* IMAGE_VIEW_ON → TV */
     rc = uiox_hdmi_cec_send(&dev, UIOX_CEC_LA_TV,
                               UIOX_CEC_OP_IMAGE_VIEW_ON, NULL, 0u);
     printf("  CEC IMAGE_VIEW_ON → TV  rc=%d\n", rc);
 
     /* ACTIVE_SOURCE — our physical address 1.0.0.0 */
     uint8_t phys_addr[2] = { 0x10u, 0x00u };
     rc = uiox_hdmi_cec_send(&dev, UIOX_CEC_LA_BROADCAST,
                               UIOX_CEC_OP_ACTIVE_SOURCE,
                               phys_addr, 2u);
     printf("  CEC ACTIVE_SOURCE (1.0.0.0)  rc=%d\n", rc);
 
     /* SYSTEM_AUDIO_MODE_REQUEST → audio system */
     rc = uiox_hdmi_cec_send(&dev, UIOX_CEC_LA_AUDIO_SYSTEM,
                               UIOX_CEC_OP_SYSTEM_AUDIO_MODE_REQ,
                               phys_addr, 2u);
     printf("  CEC SYSTEM_AUDIO_MODE_REQUEST  rc=%d\n", rc);


     /* ------------------------------------------------------------------ */
     /* CEC receive poll (non-blocking)                                     */
     /* ------------------------------------------------------------------ */

     printf("\n--- CEC RX poll ---\n");
     uint8_t src_la  = 0;
     uint8_t opcode  = 0;
     uint8_t params[14];
     uint8_t plen    = 0;

     int cec_rc = uiox_hdmi_proto_cec_recv(&dev.subsys.proto,
                                        &src_la, &opcode,
                                        params,  &plen);
     if (cec_rc == 0) {
         printf("  CEC RX  src=0x%X  opcode=0x%02X  plen=%u\n",
                src_la, opcode, plen);
     } else if (cec_rc == -EAGAIN) {
         printf("  CEC RX  no message pending\n");   /* normal — not an error */
     } else {
         printf("  CEC RX  error: %d\n", cec_rc);
     }

     /* ------------------------------------------------------------------ */
     /* 7. Framebuffer render and present                                   */
     /* ------------------------------------------------------------------ */
 
     printf("\n--- Frame rendering (6 frames) ---\n");
 
     uint32_t now_ms = 0u;
     for (int frame = 0; frame < 6; frame++) {
         now_ms += 17u;  /* ~60 fps */
         uiox_hdmi_tick(&dev, now_ms);
         uiox_hdmi_activity(&dev, now_ms);
 
         uiox_hdmi_fb_t *fb = uiox_hdmi_acquire(&dev);
         if (!fb) {
             printf("  [frame %d] acquire failed\n", frame);
             continue;
         }
 
         /* Alternate test patterns */
         switch (frame % 3) {
         case 0:
             draw_solid(fb, 0xFF1A1A2Eu);
             printf("  [frame %d] solid dark blue\n", frame);
             break;
         case 1:
             draw_colour_bars(fb);
             printf("  [frame %d] SMPTE colour bars\n", frame);
             break;
         case 2:
             draw_solid(fb, 0xFF0D0D0Du);
             printf("  [frame %d] near-black (HDR test)\n", frame);
             break;
         }
 
         rc = uiox_hdmi_present(&dev, fb);
         printf("  [frame %d] present  rc=%d  flip#=%u  vblank=%u\n",
                frame, rc, s_flip_count, s_vblank);
     }
 
     /* ------------------------------------------------------------------ */
     /* 8. HDCP status check                                                */
     /* ------------------------------------------------------------------ */
 
     printf("\n--- HDCP ---\n");
     const char *hdcp_str =
         (dev.subsys.sink.hdcp_state == UIOX_HDCP_AUTHENTICATED) ?
         "AUTHENTICATED" :
         (dev.subsys.sink.hdcp_state == UIOX_HDCP_AUTHENTICATING) ?
         "AUTHENTICATING" :
         (dev.subsys.sink.hdcp_state == UIOX_HDCP_FAILED) ?
         "FAILED" : "DISABLED";
     printf("  HDCP state : %s\n", hdcp_str);
 
     /* ------------------------------------------------------------------ */
     /* 9. Infoframe periodic tick                                          */
     /* ------------------------------------------------------------------ */
 
     printf("\n--- Infoframe tick loop (3 × 500 ms) ---\n");
     for (int i = 0; i < 3; i++) {
         now_ms += 500u;
         uiox_hdmi_tick(&dev, now_ms);
         printf("  tick t=%u ms  infoframes_sent=%u\n",
                now_ms, s_if_count);
     }
 
     /* ------------------------------------------------------------------ */
     /* 10. SPD infoframe                                                   */
     /* ------------------------------------------------------------------ */
 
     printf("\n--- SPD Infoframe ---\n");
     rc = uiox_hdmi_proto_send_spd(&dev.subsys.proto,
                                    "UIOX GmbH", "HDMI Demo");
     printf("  SPD  vendor='UIOX GmbH'  product='HDMI Demo'  rc=%d\n", rc);
 
     /* ------------------------------------------------------------------ */
     /* 11. DPMS suspend / wake                                            */
     /* ------------------------------------------------------------------ */
 
     printf("\n--- DPMS suspend/wake ---\n");
     uiox_hdmi_subsys_set_dpms(&dev.subsys, 100u); /* 100ms timeout */
     now_ms += 200u;
     uiox_hdmi_tick(&dev, now_ms);
     printf("  State after idle: %s\n",
            uiox_hdmi_state_name(dev.subsys.state));
 
     uiox_hdmi_activity(&dev, now_ms);
     printf("  State after activity: %s\n",
            uiox_hdmi_state_name(dev.subsys.state));
 
     /* ------------------------------------------------------------------ */
     /* 12. Statistics                                                       */
     /* ------------------------------------------------------------------ */
 
     printf("\n--- Statistics ---\n");
     uiox_hdmi_print_stats(&dev);
 
     /* ------------------------------------------------------------------ */
     /* 13. Stop and close                                                  */
     /* ------------------------------------------------------------------ */
 
     printf("\n--- Stop and close ---\n");
     uiox_hdmi_stop(&dev);
     printf("  State : %s\n", uiox_hdmi_state_name(dev.subsys.state));
     uiox_hdmi_close(&dev);
     printf("  Device: CLOSED\n");
 
     printf("\n=== UIOX HDMI Demo complete ===\n");
     return 0;
 } 
 