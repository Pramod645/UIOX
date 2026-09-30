/**
 * @file    uiox_hdmi_sink.h
 * @brief   UIOX HDMI sink abstraction (EDID, HDCP, mode selection).
 *
 * Manages:
 *   - EDID block 0 + extensions (CEA-861) parse
 *   - Supported video timing mode database
 *   - HDCP 1.4 / 2.3 capability detection and key exchange
 *   - Audio capability parsing (SAD blocks)
 *   - Preferred/native mode selection
 *   - Sink identity (manufacturer, product, serial)
 *
 * @date    2026-05-28
 */
//Layer 2b — Sink Abstraction
 #ifndef UIOX_HDMI_SINK_H
 #define UIOX_HDMI_SINK_H
 
 #include "uiox_hdmi_hw.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 #define UIOX_HDMI_EDID_SIZE        256   /**< EDID block 0 + CEA ext      */
 #define UIOX_HDMI_MAX_MODES        64
 #define UIOX_HDMI_SINK_NAME_MAX    14
 #define UIOX_HDMI_MAX_SAD          8     /**< Short Audio Descriptors      */
 
 /* =========================================================================
  * Short Audio Descriptor
  * ====================================================================== */
 
 typedef struct {
     uint8_t  format;        /**< Audio format code (1=LPCM, 2=AC-3, etc.) */
     uint8_t  max_channels;  /**< Maximum channels (1-8)                    */
     uint8_t  sample_rates;  /**< Bitmask: bit0=32k, bit1=44.1k, bit2=48k  */
     uint8_t  bit_depths;    /**< Bitmask: bit0=16, bit1=20, bit2=24       */
     uint32_t max_bitrate;   /**< Max compressed bitrate (kbps, HBR)        */
 } uiox_hdmi_sad_t;
 
 /* =========================================================================
  * Parsed EDID / sink capabilities
  * ====================================================================== */
 
 typedef struct {
     uint8_t   raw[UIOX_HDMI_EDID_SIZE];
     bool      valid;
     char      manufacturer[4];
     uint16_t  product_code;
     uint32_t  serial;
     char      name[UIOX_HDMI_SINK_NAME_MAX];
     uint16_t  h_size_mm, v_size_mm;
 
     /* Video */
     uiox_hdmi_timing_t modes[UIOX_HDMI_MAX_MODES];
     uint8_t            num_modes;
     uint8_t            preferred_mode_idx;
     bool               ycbcr444, ycbcr422, ycbcr420;
     bool               hdr10, hlg;
     bool               dc_30bit, dc_36bit;  /**< Deep colour support       */
     bool               vrr, allm;
 
     /* Audio */
     uiox_hdmi_sad_t    sads[UIOX_HDMI_MAX_SAD];
     uint8_t            num_sads;
     uint8_t            speaker_alloc;
 
     /* HDCP */
     bool               hdcp14;
     bool               hdcp23;
 
     /* HDMI version */
     uiox_hdmi_ver_t    hdmi_ver;
     uint16_t           max_tmds_mhz;    /**< Max TMDS clock (MHz)         */
     uiox_hdmi_frl_rate_t max_frl_rate;
 
     /* Physical address (CEC) */
     uint16_t           cec_phys_addr;
 } uiox_hdmi_edid_t;
 
 /* =========================================================================
  * Sink descriptor
  * ====================================================================== */
 
 typedef struct {
     uiox_hdmi_edid_t    edid;
     uiox_hdmi_timing_t  current_mode;
     bool                powered;
     uiox_hdcp_state_t   hdcp_state;
 } uiox_hdmi_sink_t;
 
 /* =========================================================================
  * Sink API
  * ====================================================================== */
 
 int  uiox_hdmi_sink_probe      (uiox_hdmi_sink_t *sink,
                                  uiox_hdmi_hw_t   *hw);
 int  uiox_hdmi_sink_parse_edid (uiox_hdmi_sink_t *sink);
 int  uiox_hdmi_sink_select_mode(uiox_hdmi_sink_t *sink,
                                  uint16_t pref_w, uint16_t pref_h,
                                  uint8_t  pref_hz);
 int  uiox_hdmi_sink_hdcp_start (uiox_hdmi_sink_t *sink,
                                  uiox_hdmi_hw_t   *hw);
 void uiox_hdmi_sink_hdcp_stop  (uiox_hdmi_sink_t *sink,
                                  uiox_hdmi_hw_t   *hw);
 const uiox_hdmi_timing_t *uiox_hdmi_sink_timing(const uiox_hdmi_sink_t *s);
 void uiox_hdmi_sink_print      (const uiox_hdmi_sink_t *sink);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_HDMI_SINK_H */
 