/**
 * @file    uiox_cam_if.h
 * @brief   Camera interface driver abstraction (CSI-2 / DVP).
 *
 * Configures lanes, virtual channels, DT routing, sync polarities, and
 * binds DMA buffers to hardware via HAL.
 */

 #ifndef UIOX_CAM_IF_H
 #define UIOX_CAM_IF_H
 
 #include "uiox_cam_hw.h"
 #include "uiox_cam_buf.h"
 #include "uiox_klibc.h"

 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 typedef struct {
     uiox_cam_hw_t     *hw;
     uint8_t            lanes;        /* 1..4 */
     uint8_t            virt_chan;    /* 0..3 */
     uiox_csi_dt_t      csi_dt;
     uiox_cam_pixfmt_t  pixfmt;
     uint16_t           width;
     uint16_t           height;
     uint32_t           stride;
     /* error counters */
     uint32_t           err_crc;
     uint32_t           err_ecc;
     uint32_t           err_fifo;
 } uiox_cam_if_t;
 
 /* Configure CSI/DVP with a pixel format and frame size */
 int uiox_cam_if_config(uiox_cam_if_t *cif,
                        uiox_cam_hw_t *hw,
                        uint8_t lanes, uint8_t vc,
                        uiox_csi_dt_t dt,
                        uiox_cam_pixfmt_t fmt,
                        uint16_t w, uint16_t h, uint32_t stride);
 
 /* Queue N buffers to hardware DMA */
 int uiox_cam_if_prime(uiox_cam_if_t *cif, int count);
 
 /* Poll for completed frame (non-blocking). Returns frame* or NULL if none. */
 uiox_cam_frame_t *uiox_cam_if_complete(uiox_cam_if_t *cif);
 
 #ifdef __cplusplus
 }
 #endif
 #endif
 