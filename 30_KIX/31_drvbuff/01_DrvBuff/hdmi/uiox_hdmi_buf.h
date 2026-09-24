/**
 * @file    uiox_hdmi_buf.h
 * @brief   UIOX HDMI audio/video packet and infoframe buffer pool.
 *
 * Two pools:
 *   VIDEO pool — framebuffers for pixel data (triple-buffered scanout)
 *   PACKET pool — HDMI data island packets (infoframes, audio samples, CEC)
 *
 * @date    2026-05-28
 */
//Layer 1.5 — Buffer Manager
 #ifndef UIOX_HDMI_BUF_H
 #define UIOX_HDMI_BUF_H
 
 #include "uiox_hdmi_hw.h"
 #include "uiox_klibc.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * Pool sizing
  * ====================================================================== */
 
 #define UIOX_HDMI_FB_POOL_SIZE      3      /**< Triple framebuffer pool    */
 #define UIOX_HDMI_FB_MAX_BYTES      (3840u * 2160u * 4u) /**< 4K XRGB     */
 #define UIOX_HDMI_FB_ALIGN          4096
 
 #define UIOX_HDMI_PKT_POOL_SIZE     32     /**< HDMI packet pool           */
 #define UIOX_HDMI_PKT_MAX_BYTES     36     /**< Max infoframe payload      */
 
 /* =========================================================================
  * Framebuffer descriptor
  * ====================================================================== */
 
 typedef enum {
     UIOX_HDMI_FB_FREE = 0,
     UIOX_HDMI_FB_RENDERING,
     UIOX_HDMI_FB_PENDING,
     UIOX_HDMI_FB_DISPLAYED,
 } uiox_hdmi_fb_state_t;
 
 typedef struct uiox_hdmi_fb {
     uint8_t    *vaddr;
     uintptr_t   paddr;
     uint32_t    capacity;
     uint16_t    width, height;
     uint32_t    stride;
     uint8_t     bpc;
     uiox_hdmi_colorspace_t cs;
     uiox_hdmi_fb_state_t   state;
     uint32_t    frame_id;
     uint64_t    ts_ns;
     uint8_t     in_use;
     struct uiox_hdmi_fb *next;
 } uiox_hdmi_fb_t;
 
 /* =========================================================================
  * HDMI data island packet descriptor
  * ====================================================================== */
 
 typedef enum {
     UIOX_HDMI_PKT_AVI     = 0x82u, /**< AVI Infoframe                    */
     UIOX_HDMI_PKT_AUDIO   = 0x84u, /**< Audio Infoframe                  */
     UIOX_HDMI_PKT_SPD     = 0x83u, /**< Source Product Description       */
     UIOX_HDMI_PKT_HDRVS   = 0x87u, /**< HDR Static Metadata              */
     UIOX_HDMI_PKT_GCP     = 0x3Bu, /**< General Control Packet           */
     UIOX_HDMI_PKT_VENDOR  = 0x81u, /**< Vendor Specific Infoframe        */
     UIOX_HDMI_PKT_ACR     = 0x01u, /**< Audio Clock Regeneration         */
     UIOX_HDMI_PKT_ASP     = 0x02u, /**< Audio Sample Packet              */
 } uiox_hdmi_pkt_type_t;
 
 typedef struct uiox_hdmi_pkt {
     uint8_t  type;
     uint8_t  version;
     uint8_t  length;
     uint8_t  checksum;
     uint8_t  payload[UIOX_HDMI_PKT_MAX_BYTES];
     uint8_t  in_use;
     struct uiox_hdmi_pkt *next;
 } uiox_hdmi_pkt_t;
 
 /* =========================================================================
  * Pool API
  * ====================================================================== */
 
 void            uiox_hdmi_buf_init        (uint16_t w, uint16_t h,
                                             uint32_t stride,
                                             uiox_hdmi_colorspace_t cs,
                                             uint8_t bpc);
 uiox_hdmi_fb_t *uiox_hdmi_buf_alloc_fb   (void);
 void            uiox_hdmi_buf_free_fb     (uiox_hdmi_fb_t *fb);
 void            uiox_hdmi_buf_ref_fb      (uiox_hdmi_fb_t *fb);
 uint8_t         uiox_hdmi_buf_fb_free     (void);
 
 uiox_hdmi_pkt_t *uiox_hdmi_buf_alloc_pkt (void);
 void             uiox_hdmi_buf_free_pkt   (uiox_hdmi_pkt_t *pkt);
 uint16_t         uiox_hdmi_buf_pkt_free   (void);
 
 void uiox_hdmi_buf_clear_fb(uiox_hdmi_fb_t *fb, uint32_t colour);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_HDMI_BUF_H */
 