/**
 * @file    uiox_mon_panel.c
 * @brief   UIOX Monitor panel abstraction implementation.
 * @date    2026-05-27
 */

 #include "uiox_mon_panel.h"
 #include <string.h>
 #include <stdio.h>
 #include <errno.h>
 
 /* =========================================================================
  * EDID checksum validation
  * ====================================================================== */
 
 static bool edid_checksum_ok(const uint8_t *buf)
 {
     uint8_t sum = 0;
     for (int i = 0; i < UIOX_MON_EDID_SIZE; i++) sum += buf[i];
     return sum == 0u;
 }
 
 /* =========================================================================
  * EDID manufacturer ID decode (3 × 5-bit letters packed in 2 bytes)
  * ====================================================================== */
 
 static void edid_decode_mfr(const uint8_t *buf, char *out)
 {
     uint16_t v = (uint16_t)((buf[8] << 8) | buf[9]);
     out[0] = (char)('@' + ((v >> 10) & 0x1Fu));
     out[1] = (char)('@' + ((v >>  5) & 0x1Fu));
     out[2] = (char)('@' + ((v >>  0) & 0x1Fu));
     out[3] = '\0';
 }
 
 /* =========================================================================
  * Detailed timing descriptor → mode
  * Bytes 54..71 of EDID (18-byte detailed timing block)
  * ====================================================================== */
 
 static bool edid_parse_dtd(const uint8_t *d, uiox_mon_timing_t *t)
 {
     uint32_t px_clk = (uint32_t)((d[1] << 8) | d[0]) * 10u; /* ×10 kHz */
     if (!px_clk) return false;
 
     t->pixel_clk_khz = px_clk;
     t->h_active  = (uint16_t)(d[2] | ((d[4] & 0xF0u) << 4u));
     t->h_front   = (uint16_t)(d[8] | ((d[11] & 0xC0u) << 2u));
     t->h_sync    = (uint16_t)(d[9] | ((d[11] & 0x30u) << 4u));
     t->h_back    = (uint16_t)(((d[3] | ((d[4] & 0x0Fu) << 8u)) -
                                 t->h_active - t->h_front - t->h_sync));
     t->v_active  = (uint16_t)(d[5] | ((d[7] & 0xF0u) << 4u));
     t->v_front   = (uint16_t)((d[10] >> 4u) | ((d[11] & 0x0Cu) << 2u));
     t->v_sync    = (uint16_t)((d[10] & 0x0Fu) | ((d[11] & 0x03u) << 4u));
     t->v_back    = (uint16_t)(((d[6] | ((d[7] & 0x0Fu) << 8u)) -
                                 t->v_active - t->v_front - t->v_sync));
     t->h_sync_pos = (d[17] & 0x02u) != 0u;
     t->v_sync_pos = (d[17] & 0x04u) != 0u;
     t->interlaced = (d[17] & 0x80u) != 0u;
 
     /* Approximate refresh rate */
     uint32_t h_total = t->h_active + t->h_front + t->h_sync + t->h_back;
     uint32_t v_total = t->v_active + t->v_front + t->v_sync + t->v_back;
     if (h_total && v_total)
         t->refresh_hz = (uint8_t)(px_clk * 1000u / (h_total * v_total));
 
     return true;
 }
 
 /* =========================================================================
  * Parse full EDID
  * ====================================================================== */
 
 int uiox_mon_panel_parse_edid(uiox_mon_panel_t *panel)
 {
     uiox_mon_edid_t *e = &panel->edid;
     const uint8_t   *b = e->raw;
 
     e->valid = edid_checksum_ok(b);
     if (!e->valid) return -EBADMSG;
 
     edid_decode_mfr(b, e->manufacturer);
     e->product_code = (uint16_t)((b[9] << 8) | b[8]);
     e->serial       = (uint32_t)(b[15] << 24 | b[14] << 16 |
                                   b[13] << 8  | b[12]);
     e->h_size_mm    = b[21];
     e->v_size_mm    = b[22];
     e->gamma_x100   = (uint8_t)(b[23] + 100u);
     e->digital      = (b[20] & 0x80u) != 0u;
     e->dpms_standby = (b[24] & 0x80u) != 0u;
     e->dpms_suspend = (b[24] & 0x40u) != 0u;
     e->dpms_off     = (b[24] & 0x20u) != 0u;
 
     /* Colour depth from digital input definition */
     if (e->digital) {
         static const uint8_t cbd[] = {0,6,8,10,12,14,16,0};
         e->color_depth = cbd[(b[20] >> 4u) & 0x07u];
     }
 
     /* Parse 4 × 18-byte descriptors at offsets 54,72,90,108 */
     e->num_modes = 0;
     for (int di = 0; di < 4 && e->num_modes < UIOX_MON_MAX_MODES; di++) {
         const uint8_t *d = &b[54 + di * 18];
         /* Detailed timing descriptor if pixel clock != 0 */
         if (d[0] || d[1]) {
             uiox_mon_mode_t *m = &e->modes[e->num_modes];
             if (edid_parse_dtd(d, &m->timing)) {
                 m->preferred = (di == 0);   /* First DTD = preferred */
                 m->native    = (di == 0);
                 m->cea       = false;
                 e->num_modes++;
             }
         }
         /* Monitor name descriptor (tag 0xFC) */
         else if (d[3] == 0xFCu) {
             memset(e->name, 0, sizeof(e->name));
             memcpy(e->name, &d[5], 13u);
             /* Strip trailing 0x0A / spaces */
             for (int k = 12; k >= 0; k--) {
                 if (e->name[k] == 0x0A || e->name[k] == ' ')
                     e->name[k] = '\0';
                 else break;
             }
         }
     }
 
     return 0;
 }
 
 /* =========================================================================
  * Panel probe
  * ====================================================================== */
 
 int uiox_mon_panel_probe(uiox_mon_panel_t *panel, uiox_mon_hw_t *hw)
 {
     if (!panel || !hw) return -EINVAL;
     memset(&panel->edid, 0, sizeof(panel->edid));
 
     /* Try EDID read */
     int rc = uiox_mon_hw_read_edid(hw, panel->edid.raw);
     if (rc == 0) {
         rc = uiox_mon_panel_parse_edid(panel);
         if (rc < 0) {
             /* EDID parse failed — fall back to fixed timing */
             if (panel->fixed_timing) {
                 panel->current_mode.timing   = *panel->fixed_timing;
                 panel->current_mode.preferred = true;
                 panel->current_mode.native    = true;
                 return 0;
             }
             return rc;
         }
     } else if (panel->fixed_timing) {
         /* No DDC — use fixed panel timing */
         panel->current_mode.timing    = *panel->fixed_timing;
         panel->current_mode.preferred = true;
         panel->current_mode.native    = true;
         return 0;
     } else {
         return -ENODEV;
     }
 
     /* Select preferred mode by default */
     return uiox_mon_panel_select_mode(panel, 0u, 0u, 0u);
 }
 
 /* =========================================================================
  * Mode selection (best fit for requested resolution)
  * ====================================================================== */
 
 int uiox_mon_panel_select_mode(uiox_mon_panel_t *panel,
                                 uint16_t pref_w, uint16_t pref_h,
                                 uint8_t  pref_hz)
 {
     if (!panel || !panel->edid.num_modes) return -ENOENT;
 
     const uiox_mon_mode_t *best = NULL;
 
     for (uint8_t i = 0; i < panel->edid.num_modes; i++) {
         const uiox_mon_mode_t *m = &panel->edid.modes[i];
 
         /* If no preference given, pick preferred/native */
         if (pref_w == 0 && pref_h == 0) {
             if (m->preferred) { best = m; break; }
             if (!best || m->native) best = m;
             continue;
         }
 
         if (m->timing.h_active != pref_w) continue;
         if (m->timing.v_active != pref_h) continue;
         if (pref_hz && m->timing.refresh_hz != pref_hz) continue;
         best = m;
         break;
     }
 
     if (!best) return -ENOENT;
     memcpy(&panel->current_mode, best, sizeof(*best));
     return 0;
 }
 
 int uiox_mon_panel_power_on(uiox_mon_panel_t *panel, uiox_mon_hw_t *hw)
 {
     if (!panel || !hw) return -EINVAL;
     /* In a real driver: assert VDD, wait power_on_delay_ms,
      * assert RESET, then enable backlight after backlight_on_delay_ms */
     (void)panel->power_on_delay_ms;
     int rc = uiox_mon_hw_set_dpms(hw, UIOX_MON_DPMS_ON);
     if (rc == 0) panel->powered = true;
     return rc;
 }
 
 void uiox_mon_panel_power_off(uiox_mon_panel_t *panel, uiox_mon_hw_t *hw)
 {
     if (!panel || !hw) return;
     uiox_mon_hw_set_dpms(hw, UIOX_MON_DPMS_OFF);
     panel->powered = false;
 }
 
 const uiox_mon_timing_t *uiox_mon_panel_timing(const uiox_mon_panel_t *p)
 {
     return p ? &p->current_mode.timing : NULL;
 }
 
 void uiox_mon_panel_print(const uiox_mon_panel_t *panel)
 {
     if (!panel) return;
     const uiox_mon_edid_t *e = &panel->edid;
     printf("  Panel name   : %s\n",   e->valid ? e->name        : "(no EDID)");
     printf("  Manufacturer : %s\n",   e->valid ? e->manufacturer: "---");
     printf("  Product code : 0x%04X\n", e->product_code);
     printf("  Size         : %u × %u mm\n", e->h_size_mm, e->v_size_mm);
     printf("  Gamma        : %.2f\n",  (float)e->gamma_x100 / 100.0f);
     printf("  Colour depth : %u bpc\n", e->color_depth);
     printf("  Modes found  : %u\n",   e->num_modes);
     printf("  Current mode : %u × %u @ %u Hz  pclk=%u kHz\n",
            panel->current_mode.timing.h_active,
            panel->current_mode.timing.v_active,
            panel->current_mode.timing.refresh_hz,
            panel->current_mode.timing.pixel_clk_khz);
     printf("  Powered      : %s\n",   panel->powered ? "YES" : "NO");
 }
 