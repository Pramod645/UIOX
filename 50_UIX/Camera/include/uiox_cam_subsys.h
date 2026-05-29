/**
 * @file    uiox_cam_subsys.h
 * @brief   Camera subsystem: pipeline, controls, streaming state.
 */

 #ifndef UIOX_CAM_SUBSYS_H
 #define UIOX_CAM_SUBSYS_H
 
 #include "uiox_cam_if.h"
 #include "uiox_cam_sensor.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 typedef enum {
     UIOX_CAM_STATE_STOPPED = 0,
     UIOX_CAM_STATE_STREAMING
 } uiox_cam_state_t;
 
 typedef struct {
     uiox_cam_if_t      cif;
     uiox_cam_sensor_t  sensor;
     uiox_cam_state_t   state;
     uint8_t            primed;
     uint8_t            buffers;      /* number of queued capture buffers */
 } uiox_cam_pipeline_t;
 
 /* Build a pipeline: sensor + interface over given HAL + bus ops */
 int  uiox_cam_pipeline_build(uiox_cam_pipeline_t *pl,
                              uiox_cam_hw_t *hw,
                              const uiox_cam_bus_ops_t *bus,
                              const char *sensor_name,
                              uint8_t i2c_addr,
                              uint16_t chip_id_reg,
                              uint16_t chip_id_val);
 
 /* Configure mode (resolution/fps/format) and prime N buffers */
 int  uiox_cam_pipeline_config(uiox_cam_pipeline_t *pl,
                               uint16_t w, uint16_t h, uint8_t fps,
                               uiox_cam_pixfmt_t fmt,
                               uint8_t lanes, uint8_t vc,
                               uiox_csi_dt_t dt,
                               uint32_t stride,
                               uint8_t queue_count);
 
 /* Start/stop streaming */
 int  uiox_cam_pipeline_start(uiox_cam_pipeline_t *pl);
 void uiox_cam_pipeline_stop (uiox_cam_pipeline_t *pl);
 
 /* Drain a completed frame (NULL if none). Ownership is transferred to caller. */
 uiox_cam_frame_t *uiox_cam_pipeline_dequeue(uiox_cam_pipeline_t *pl);
 
 #ifdef __cplusplus
 }
 #endif
 #endif
 