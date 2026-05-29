/**
 * @file    uiox_cam_hw.h
 * @brief   UIOX Camera Hardware Abstraction Layer (HAL).
 *
 * Lowest-level interface to camera capture hardware (CSI-2 receiver
 * or parallel DVP). Owns MMIO register access, clock/reset control,
 * DMA engines (capture to memory), and ISR top-half.
 */

 #ifndef UIOX_CAM_HW_H
 #define UIOX_CAM_HW_H
 
 #include <stdint.h>
 #include <stdbool.h>
 #include <stddef.h>
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* Capabilities */
 #define UIOX_CAM_CAP_CSI2        (1u << 0)
 #define UIOX_CAM_CAP_DVP         (1u << 1)
 #define UIOX_CAM_CAP_DMA_CONTIG  (1u << 2)
 #define UIOX_CAM_CAP_DMA_SCATTER (1u << 3)
 #define UIOX_CAM_CAP_EMBD_DATA   (1u << 4)  /* Embedded data short packets */
 
 /* CSI data types (subset) */
 typedef enum {
     UIOX_CSI_DT_RAW8   = 0x2A,
     UIOX_CSI_DT_RAW10  = 0x2B,
     UIOX_CSI_DT_RAW12  = 0x2C,
     UIOX_CSI_DT_YUV420 = 0x18,
     UIOX_CSI_DT_YUV422 = 0x1E,
     UIOX_CSI_DT_RGB888 = 0x24,
     UIOX_CSI_DT_Embedded = 0x12
 } uiox_csi_dt_t;
 
 typedef enum {
     UIOX_CAM_PIX_FMT_RAW8,
     UIOX_CAM_PIX_FMT_RAW10,
     UIOX_CAM_PIX_FMT_RAW12,
     UIOX_CAM_PIX_FMT_YUV420,
     UIOX_CAM_PIX_FMT_YUV422,
     UIOX_CAM_PIX_FMT_RGB888
 } uiox_cam_pixfmt_t;
 
 typedef struct {
     uintptr_t    base_addr;     /* MMIO base of CSI/ISP */
     uint32_t     irq;
     uint32_t     caps;          /* UIOX_CAM_CAP_* */
     uint8_t      max_lanes;     /* e.g., 2 or 4 for CSI-2 */
     void        *priv;          /* driver private */
 } uiox_cam_hw_t;
 
 typedef struct {
     /* Clocks and resets */
     int  (*init)        (uiox_cam_hw_t *hw);
     void (*deinit)      (uiox_cam_hw_t *hw);
     int  (*start)       (uiox_cam_hw_t *hw);
     void (*stop)        (uiox_cam_hw_t *hw);
 
     /* CSI/DVP config */
     int  (*set_csi)     (uiox_cam_hw_t *hw,
                          uint8_t lanes,
                          uiox_csi_dt_t dt,
                          uint8_t virt_chan);
 
     int  (*set_format)  (uiox_cam_hw_t *hw,
                          uint16_t width, uint16_t height,
                          uiox_cam_pixfmt_t fmt);
 
     /* DMA programming — queue a frame buffer (physical address + length) */
     int  (*dma_queue)   (uiox_cam_hw_t *hw, uintptr_t phys, uint32_t length);
 
     /* Poll or acknowledge DMA completion; returns bytes captured or <0 on error */
     int  (*dma_complete)(uiox_cam_hw_t *hw, uintptr_t *phys_out, uint32_t *bytes_out);
 
     /* ISR top-half */
     void (*isr)         (uiox_cam_hw_t *hw);
 
 } uiox_cam_hw_ops_t;
 
 /* HAL API */
 int  uiox_cam_hw_init   (uiox_cam_hw_t *hw, const uiox_cam_hw_ops_t *ops);
 int  uiox_cam_hw_start  (uiox_cam_hw_t *hw);
 void uiox_cam_hw_stop   (uiox_cam_hw_t *hw);
 
 /* Convenience */
 static inline uint32_t uiox_cam_caps(const uiox_cam_hw_t *hw) { return hw->caps; }
 
 #ifdef __cplusplus
 }
 #endif
 #endif
 