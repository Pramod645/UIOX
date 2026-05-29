/**
 * @file    uiox_hdmi_proto.h
 * @brief   UIOX HDMI protocol layer — infoframes, CEC, ARC/eARC.
 *
 * Implements:
 *   - AVI Infoframe (CEA-861-H Table 6)
 *   - Audio Infoframe (CEA-861-H Table 8)
 *   - HDR Static Metadata Infoframe (CTA-861-H)
 *   - Vendor Specific Infoframe (VSIF) for HDMI 2.1 features
 *   - Source Product Description (SPD) Infoframe
 *   - General Control Packet (GCP) for deep colour / AVMUTE
 *   - CEC command builder and dispatcher (CEC 2.0 subset)
 *   - ARC / eARC audio channel management
 *
 * @date    2026-05-28
 */
//Layer 3 — Protocol
 #ifndef UIOX_HDMI_PROTO_H
 #define UIOX_HDMI_PROTO_H
 
 #include "uiox_hdmi_if.h"
 #include "uiox_hdmi_sink.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * AVI Infoframe — colorimetry / scan / aspect
  * ====================================================================== */
 
 typedef struct {
     uint8_t  scan_info;       /**< 0=no data, 1=over, 2=under             */
     uint8_t  bar_info;        /**< 0=none, 1=vert, 2=horiz, 3=both       */
     bool     active_format;
     uint8_t  colorimetry;     /**< 0=no data, 1=BT.601, 2=BT.709         */
     uint8_t  picture_aspect;  /**< 0=no data, 1=4:3, 2=16:9              */
     uint8_t  active_aspect;   /**< 0=same, 8=4:3, 9=16:9, 10=14:9       */
     bool     it_content;
     uint8_t  ext_colorimetry; /**< 0=xvYCC601, 1=xvYCC709, 4=BT.2020   */
     uint8_t  rgb_quant;       /**< 0=default, 1=limited, 2=full          */
     uint8_t  nups;            /**< Non-uniform picture scaling            */
     uint8_t  vic;             /**< Video Identification Code              */
     uint8_t  ycc_quant;       /**< YCC quantisation range                 */
     uint8_t  cn;              /**< IT content type                        */
     uint8_t  pr;              /**< Pixel repetition factor                */
     uint16_t top, bottom, left, right; /**< Bar data (if bar_info set)   */
 } uiox_hdmi_avi_t;
 
 /* =========================================================================
  * Audio Infoframe
  * ====================================================================== */
 
 typedef struct {
     uint8_t  channel_count;   /**< 0=same as stream, 1=2ch, etc.         */
     uint8_t  coding_type;     /**< 0=stream, 1=LPCM, 2=AC-3, etc.       */
     uint8_t  sample_size;     /**< 0=stream, 1=16, 2=20, 3=24 bit       */
     uint8_t  sample_freq;     /**< 0=stream, 1=32k, 2=44.1k, 3=48k     */
     uint8_t  channel_alloc;   /**< CEA-861 Table 28                      */
     bool     down_mix;
     uint8_t  lfe_level;       /**< 0=no info, 1=0dB, 2=+10dB            */
     uint8_t  level_shift;     /**< 0..15 dB                              */
 } uiox_hdmi_audio_info_t;
 
 /* =========================================================================
  * HDR Static Metadata Infoframe (CEA-861.3)
  * ====================================================================== */
 
 typedef struct {
     uint8_t  eotf;            /**< 0=SDR, 1=HDR, 2=SMPTE-ST-2084, 3=HLG*/
     uint8_t  metadata_type;   /**< 0=Type 1 static metadata              */
     uint16_t display_primaries_x[3]; /**< Red,Green,Blue x (×0.00002)   */
     uint16_t display_primaries_y[3]; /**< Red,Green,Blue y              */
     uint16_t white_point_x, white_point_y;
     uint16_t max_luminance;   /**< ×1 cd/m²                              */
     uint16_t min_luminance;   /**< ×0.0001 cd/m²                        */
     uint16_t max_cll;         /**< Max Content Light Level (cd/m²)       */
     uint16_t max_fall;        /**< Max Frame Average Light Level          */
 } uiox_hdmi_hdr_t;
 
 /* =========================================================================
  * CEC logical addresses and opcodes
  * ====================================================================== */
 
 #define UIOX_CEC_LA_TV              0x00u
 #define UIOX_CEC_LA_RECORDER1       0x01u
 #define UIOX_CEC_LA_PLAYBACK1       0x04u
 #define UIOX_CEC_LA_AUDIO_SYSTEM    0x05u
 #define UIOX_CEC_LA_BROADCAST       0x0Fu
 
 #define UIOX_CEC_OP_IMAGE_VIEW_ON   0x04u
 #define UIOX_CEC_OP_TEXT_VIEW_ON    0x0Du
 #define UIOX_CEC_OP_ACTIVE_SOURCE   0x82u
 #define UIOX_CEC_OP_STANDBY         0x36u
 #define UIOX_CEC_OP_SET_STREAM_PATH 0x86u
 #define UIOX_CEC_OP_SYSTEM_AUDIO_MODE_REQ 0x70u
 #define UIOX_CEC_OP_REPORT_PHYS_ADDR 0x84u
 #define UIOX_CEC_OP_DEVICE_VENDOR_ID 0x87u
 #define UIOX_CEC_OP_OSD_NAME         0x47u
 
 /* =========================================================================
  * Protocol context
  * ====================================================================== */
 
 typedef struct {
     uiox_hdmi_if_t    *hif;
     uiox_hdmi_sink_t  *sink;
     uiox_hdmi_avi_t    avi;
     uiox_hdmi_audio_info_t audio_info;
     uiox_hdmi_hdr_t    hdr;
     bool               hdr_enabled;
     bool               avmute;
     uint8_t            cec_la;       /**< Our CEC logical address          */
     uint32_t           vendor_id;
     const char        *osd_name;
     uint32_t           last_infoframe_ms;
     uint32_t           infoframe_interval_ms; /**< Resend interval        */
 } uiox_hdmi_proto_t;
 
 /* =========================================================================
  * Protocol API
  * ====================================================================== */
 
 int  uiox_hdmi_proto_init          (uiox_hdmi_proto_t *proto,
                                      uiox_hdmi_if_t    *hif,
                                      uiox_hdmi_sink_t  *sink);
 
 /** Build and send AVI Infoframe from current timing/colorspace. */
 int  uiox_hdmi_proto_send_avi      (uiox_hdmi_proto_t *proto);
 
 /** Build and send Audio Infoframe. */
 int  uiox_hdmi_proto_send_audio_if (uiox_hdmi_proto_t *proto);
 
 /** Build and send HDR Static Metadata Infoframe. */
 int  uiox_hdmi_proto_send_hdr      (uiox_hdmi_proto_t *proto,
                                      const uiox_hdmi_hdr_t *hdr);
 
 /** Build and send SPD Infoframe. */
 int  uiox_hdmi_proto_send_spd      (uiox_hdmi_proto_t *proto,
                                      const char *vendor,
                                      const char *product);
 
 /** Send General Control Packet (AVMUTE / deep colour). */
 int  uiox_hdmi_proto_send_gcp      (uiox_hdmi_proto_t *proto, bool mute);
 
 /** Send a CEC command. */
 int  uiox_hdmi_proto_cec_send      (uiox_hdmi_proto_t *proto,
                                      uint8_t dst_la, uint8_t opcode,
                                      const uint8_t *params, uint8_t plen);
 
 /** Poll for incoming CEC message. Returns 0 if received. */
 int  uiox_hdmi_proto_cec_recv      (uiox_hdmi_proto_t *proto,
                                      uint8_t *src_la, uint8_t *opcode,
                                      uint8_t *params, uint8_t *plen);
 
 /** Periodic tick — re-sends infoframes on schedule. */
 void uiox_hdmi_proto_tick          (uiox_hdmi_proto_t *proto,
                                      uint32_t now_ms);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_HDMI_PROTO_H */
 