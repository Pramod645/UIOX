/**
 * @file    uiox_hdmi_sink.c
 * @brief   UIOX HDMI sink abstraction implementation.
 * @date    2026-05-28
 */

 #include "uiox_hdmi_sink.h"
 #include "uiox_klibc.h"
 
 /* -------------------------------------------------------------------------
  * EDID checksum
  * ---------------------------------------------------------------------- */
 
 static bool edid_ok(const uint8_t *b, uint16_t len)
 {
     uint8_t sum = 0;
     for (uint16_t i = 0; i < len; i++) sum += b[i];
     return sum == 0u;
 }
 
 /* -------------------------------------------------------------------------
  * Manufacturer 5-bit packed → 3-char ASCII
  * ---------------------------------------------------------------------- */
 
 static void decode_mfr(const uint8_t *b, char *out)
 {
     uint16_t v = (uint16_t)((b[8] << 8) | b[9]);
     out[0] = (char)('@' + ((v >> 10) & 0x1Fu));
     out[1] = (char)('@' + ((v >>  5) & 0x1Fu));
     out[2] = (char)('@' + ((v >>  0) & 0x1Fu));
     out[3] = '\0';
 }
 
 /* -------------------------------------------------------------------------
  * Parse 18-byte DTD into timing struct
  * ---------------------------------------------------------------------- */
 
 static bool parse_dtd(const uint8_t *d, uiox_hdmi_timing_t *t)
 {
     uint32_t clk = (uint32_t)((d[1] << 8) | d[0]) * 10u;
     if (!clk) return false;
     t->pixel_clk_khz = clk;
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
     uint32_t ht = t->h_active + t->h_front + t->h_sync + t->h_back;
     uint32_t vt = t->v_active + t->v_front + t->v_sync + t->v_back;
     t->refresh_hz = (ht && vt) ?
         (uint8_t)(clk * 1000u / (ht * vt)) : 60u;
     return true;
 }
 
 /* -------------------------------------------------------------------------
  * Parse CEA-861 extension block
  * ---------------------------------------------------------------------- */
 
 static void parse_cea861(uiox_hdmi_sink_t *sink, const uint8_t *ext)
 {
     uiox_hdmi_edid_t *e = &sink->edid;
     if (ext[0] != 0x02u) return;  /* Not CEA extension */
 
     uint8_t dtd_offset = ext[2];
     uint8_t flags      = ext[3];
 
     e->ycbcr444 = (flags & 0x10u) != 0u;
     e->ycbcr422 = (flags & 0x20u) != 0u;
 
     /* Parse data blocks between offset 4 and dtd_offset */
     uint8_t pos = 4u;
     while (pos < dtd_offset && pos < 127u) {
         uint8_t tag = (ext[pos] >> 5u) & 0x07u;
         uint8_t len = ext[pos] & 0x1Fu;
         pos++;
         if (pos + len > 127u) break;
 
         switch (tag) {
         case 0x01: /* Audio data block — Short Audio Descriptors */
             for (uint8_t i = 0;
                  i + 2 < len && e->num_sads < UIOX_HDMI_MAX_SAD; i += 3) {
                 uiox_hdmi_sad_t *s = &e->sads[e->num_sads++];
                 s->format       = (ext[pos + i] >> 3u) & 0x0Fu;
                 s->max_channels = (ext[pos + i] & 0x07u) + 1u;
                 s->sample_rates = ext[pos + i + 1];
                 s->bit_depths   = ext[pos + i + 2];
             }
             break;
 
         case 0x02: /* Video data block — VIC codes */
             for (uint8_t i = 0;
                  i < len && e->num_modes < UIOX_HDMI_MAX_MODES; i++) {
                 e->modes[e->num_modes].vic = ext[pos + i] & 0x7Fu;
                 e->num_modes++;
             }
             break;
 
         case 0x04: /* Speaker allocation */
             if (len >= 1) e->speaker_alloc = ext[pos];
             break;
 
         case 0x07: /* Extended tag */
             if (len >= 1) {
                 uint8_t etag = ext[pos];
                 switch (etag) {
                 case 0x00: /* Video Capability */
                     if (len >= 2) e->ycbcr420 = (ext[pos+1] & 0x20u) != 0u;
                     break;
                 case 0x0D: /* Colorimetry */
                     break;
                 case 0x06: /* HDR Static Metadata */
                     if (len >= 3) {
                         e->hdr10 = (ext[pos+1] & 0x02u) != 0u;
                         e->hlg   = (ext[pos+1] & 0x08u) != 0u;
                     }
                     break;
                 case 0x79: /* HDMI Forum VSDB */
                     if (len >= 7) {
                         e->max_frl_rate = (uiox_hdmi_frl_rate_t)
                                           ((ext[pos+5] >> 4u) & 0x0Fu);
                         e->vrr  = (ext[pos+6] & 0x40u) != 0u;
                         e->allm = (ext[pos+6] & 0x02u) != 0u;
                     }
                     break;
                 default: break;
                 }
             }
             break;
 
         default: break;
         }
         pos += len;
     }
 
     /* Parse DTDs in CEA extension */
     for (uint8_t d = dtd_offset; d + 18u <= 127u; d += 18u) {
         if (e->num_modes >= UIOX_HDMI_MAX_MODES) break;
         if (!ext[d] && !ext[d+1]) break;
         uiox_hdmi_timing_t *t = &e->modes[e->num_modes];
         if (parse_dtd(&ext[d], t)) e->num_modes++;
     }
 }
 
 /* =========================================================================
  * Public API
  * ====================================================================== */
 
 int uiox_hdmi_sink_probe(uiox_hdmi_sink_t *sink, uiox_hdmi_hw_t *hw)
 {
     if (!sink || !hw) return -EINVAL;
     memset(&sink->edid, 0, sizeof(sink->edid));
 
     /* Read EDID block 0 (128 bytes) at DDC address 0x50 */
     int rc = uiox_hdmi_hw_ddc_read(hw, 0x50u, 0x00u,
                                     sink->edid.raw, 128u);
     if (rc < 0) return rc;
     if (!edid_ok(sink->edid.raw, 128u)) return -EBADMSG;
 
     /* Read CEA-861 extension if present */
     if (sink->edid.raw[126] >= 1u) {
         uiox_hdmi_hw_ddc_read(hw, 0x50u, 0x80u,
                                &sink->edid.raw[128], 128u);
     }
 
     return uiox_hdmi_sink_parse_edid(sink);
 }
 
 int uiox_hdmi_sink_parse_edid(uiox_hdmi_sink_t *sink)
 {
     uiox_hdmi_edid_t *e = &sink->edid;
     const uint8_t *b    = e->raw;
 
     e->valid = edid_ok(b, 128u);
     if (!e->valid) return -EBADMSG;
 
     decode_mfr(b, e->manufacturer);
     e->product_code = (uint16_t)((b[9] << 8) | b[8]);
     e->serial       = (uint32_t)(b[15]<<24 | b[14]<<16 | b[13]<<8 | b[12]);
     e->h_size_mm    = b[21];
     e->v_size_mm    = b[22];
 
     /* Parse 4 × 18-byte descriptors */
     e->num_modes = 0;
     e->preferred_mode_idx = 0;
     for (int di = 0; di < 4; di++) {
         const uint8_t *d = &b[54 + di * 18];
         if (d[0] || d[1]) {
             uiox_hdmi_timing_t *t = &e->modes[e->num_modes];
             if (parse_dtd(d, t)) {
                 if (di == 0) e->preferred_mode_idx = e->num_modes;
                 e->num_modes++;
             }
         } else if (d[3] == 0xFCu) {
             memset(e->name, 0, sizeof(e->name));
             memcpy(e->name, &d[5], 13u);
             for (int k = 12; k >= 0; k--) {
                 if (e->name[k] == 0x0A || e->name[k] == ' ')
                     e->name[k] = '\0';
                 else break;
             }
         }
     }
 
     /* HDCP 1.4 presence — detected via HDCP key from HDCP handshake;
      * for now assume if sink is HDMI 1.4+ it supports HDCP 1.4       */
     e->hdcp14 = true;
 
     /* Parse CEA-861 extension */
     if (b[126] >= 1u) parse_cea861(sink, &b[128]);
 
     /* Determine HDMI version from max */
    /* Determine HDMI version from max TMDS clock */
    uint8_t max_tmds = b[131]; /* byte 131 of CEA ext = max TMDS / 5 */
    e->max_tmds_mhz  = (uint16_t)(max_tmds * 5u);
    if (e->max_frl_rate > 0)
        e->hdmi_ver = UIOX_HDMI_VER_21;
    else if (e->max_tmds_mhz > 340u)
        e->hdmi_ver = UIOX_HDMI_VER_20;
    else
        e->hdmi_ver = UIOX_HDMI_VER_14;

    return 0;
}

int uiox_hdmi_sink_select_mode(uiox_hdmi_sink_t *sink,
                                uint16_t pref_w, uint16_t pref_h,
                                uint8_t  pref_hz)
{
    if (!sink || !sink->edid.num_modes) return -ENOENT;
    uiox_hdmi_edid_t *e = &sink->edid;

    if (pref_w == 0 && pref_h == 0) {
        memcpy(&sink->current_mode,
               &e->modes[e->preferred_mode_idx], sizeof(sink->current_mode));
        return 0;
    }

    for (uint8_t i = 0; i < e->num_modes; i++) {
        const uiox_hdmi_timing_t *m = &e->modes[i];
        if (m->h_active != pref_w || m->v_active != pref_h) continue;
        if (pref_hz && m->refresh_hz != pref_hz)            continue;
        memcpy(&sink->current_mode, m, sizeof(*m));
        return 0;
    }
    return -ENOENT;
}

int uiox_hdmi_sink_hdcp_start(uiox_hdmi_sink_t *sink, uiox_hdmi_hw_t *hw)
{
    if (!sink || !hw) return -EINVAL;
    const uiox_hdmi_hw_ops_t *ops = (const uiox_hdmi_hw_ops_t *)hw->priv;
    if (!ops || !ops->hdcp_start) return -ENOSYS;

    uint8_t ver = sink->edid.hdcp23 ? 23u : 14u;
    int rc = ops->hdcp_start(hw, ver);
    if (rc == 0) sink->hdcp_state = UIOX_HDCP_AUTHENTICATING;
    return rc;
}

void uiox_hdmi_sink_hdcp_stop(uiox_hdmi_sink_t *sink, uiox_hdmi_hw_t *hw)
{
    if (!sink || !hw) return;
    const uiox_hdmi_hw_ops_t *ops = (const uiox_hdmi_hw_ops_t *)hw->priv;
    if (ops && ops->hdcp_stop) ops->hdcp_stop(hw);
    sink->hdcp_state = UIOX_HDCP_DISABLED;
}

const uiox_hdmi_timing_t *uiox_hdmi_sink_timing(const uiox_hdmi_sink_t *s)
{
    return s ? &s->current_mode : NULL;
}

void uiox_hdmi_sink_print(const uiox_hdmi_sink_t *sink)
{
    if (!sink) return;
    const uiox_hdmi_edid_t *e = &sink->edid;
    static const char *ver_names[] = {"HDMI 1.4","HDMI 2.0","HDMI 2.1"};
    printf("  Sink name    : %s\n",   e->valid ? e->name : "(no EDID)");
    printf("  Manufacturer : %s\n",   e->manufacturer);
    printf("  Size         : %u × %u mm\n", e->h_size_mm, e->v_size_mm);
    printf("  HDMI version : %s\n",   ver_names[e->hdmi_ver]);
    printf("  Max TMDS     : %u MHz\n", e->max_tmds_mhz);
    printf("  YCbCr 4:4:4  : %s\n",   e->ycbcr444 ? "yes" : "no");
    printf("  YCbCr 4:2:0  : %s\n",   e->ycbcr420 ? "yes" : "no");
    printf("  HDR10        : %s\n",   e->hdr10    ? "yes" : "no");
    printf("  HLG          : %s\n",   e->hlg      ? "yes" : "no");
    printf("  VRR          : %s\n",   e->vrr      ? "yes" : "no");
    printf("  ALLM         : %s\n",   e->allm     ? "yes" : "no");
    printf("  HDCP 1.4     : %s\n",   e->hdcp14   ? "yes" : "no");
    printf("  Modes found  : %u\n",   e->num_modes);
    printf("  Audio SADs   : %u\n",   e->num_sads);
    printf("  Current mode : %u × %u @ %u Hz  pclk=%u kHz\n",
           sink->current_mode.h_active,
           sink->current_mode.v_active,
           sink->current_mode.refresh_hz,
           sink->current_mode.pixel_clk_khz);
}
 