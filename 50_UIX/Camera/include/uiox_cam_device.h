/**
 * @file    uiox_cam_device.h
 * @brief   High-level camera device API for applications.
 *
 * Simplifies open/config/start/capture/stop operations.
 */

 #ifndef UIOX_CAM_DEVICE_H
 #define UIOX_CAM_DEVICE_H
 
 #include "uiox_cam_subsys.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 typedef struct {
     uiox_cam_pipeline_t pipeline;
     uiox_cam_hw_t      *hw;
 } uiox_cam_device_t;
 
 typedef struct {
     /* HAL */
     uiox_cam_hw_t            *hw;
     const uiox_cam_hw_ops_t  *hw_ops;
 
     /* Sensor bus (I2C) */
     uiox_cam_bus_ops_t        bus;
 
     /* Sensor ID */
     const char               *sensor_name;
     uint8_t                   i2c_addr;
     uint16_t                  chip_id_reg;
     uint16_t                  chip_id_val;
 
     /* Interface defaults */
     uint8_t                   lanes;
     uint8_t                   virt_chan;
     uiox_csi_dt_t             csi_dt;
 
     /* Format */
     uiox_cam_pixfmt_t         pixfmt;
     uint16_t                  width, height;
     uint32_t                  stride;
     uint8_t                   fps;
     uint8_t                   queue_count; /* buffers to prime */
 } uiox_cam_open_params_t;
 
 /* Open and initialise a camera device */
 int  uiox_cam_open (uiox_cam_device_t *dev, const uiox_cam_open_params_t *p);
 
 /* Start/stop streaming */
 int  uiox_cam_start(uiox_cam_device_t *dev);
 void uiox_cam_stop (uiox_cam_device_t *dev);
 
 /* Non-blocking capture dequeue (returns NULL if no new frame) */
 uiox_cam_frame_t *uiox_cam_capture_try(uiox_cam_device_t *dev);
 
 /* Set common controls */
 int  uiox_cam_set_exposure(uiox_cam_device_t *dev, uint32_t exposure_us);
 int  uiox_cam_set_gain    (uiox_cam_device_t *dev, uint16_t gain_x100);
 
 #ifdef __cplusplus
 }
 #endif
 #endif
 