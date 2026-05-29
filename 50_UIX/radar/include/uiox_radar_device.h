/**
 * @file    uiox_radar_device.h
 * @brief   UIOX Radar top-level application-facing device API.
 *
 * Single include for application code. Wraps the entire radar stack:
 * HAL → interface → sensor → DSP → subsystem → point cloud output.
 *
 * @date    2026-05-26
 */
//Layer 5 — Device API
 #ifndef UIOX_RADAR_DEVICE_H
 #define UIOX_RADAR_DEVICE_H
 
 #include "uiox_radar_subsys.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * Open parameters
  * ====================================================================== */
 
 typedef struct {
     /* HAL */
     uiox_radar_hw_t             *hw;
     const uiox_radar_hw_ops_t   *hw_ops;
 
     /* Sensor bus (SPI) */
     uiox_radar_bus_ops_t         bus;
     const char                  *sensor_name;
     uint16_t                     device_id;
 
     /* Interface */
     uiox_radar_if_type_t         if_type;
     uint8_t                      num_rx;
     uint8_t                      num_tx;
     uint8_t                      queue_count;
 
     /* Chirp config */
     uiox_radar_chirp_cfg_t       chirp;
 
     /* DSP config */
     uiox_radar_dsp_cfg_t         dsp;
 
     /* Tracker config */
     uiox_radar_tracker_cfg_t     tracker;
 } uiox_radar_open_params_t;
 
 /* =========================================================================
  * Device handle
  * ====================================================================== */
 
 typedef struct {
     uiox_radar_pipeline_t   pipeline;
     uiox_radar_hw_t        *hw;
     bool                    open;
 } uiox_radar_device_t;
 
 /* =========================================================================
  * Application API
  * ====================================================================== */
 
 /**
  * @brief  Open and fully initialise a radar device.
  * @return 0 on success, negative errno on failure.
  */
 int  uiox_radar_open  (uiox_radar_device_t      *dev,
                         const uiox_radar_open_params_t *p);
 
 /** @brief  Start radar frame transmission and ADC capture. */
 int  uiox_radar_start (uiox_radar_device_t *dev);
 
 /** @brief  Stop transmission and capture. */
 void uiox_radar_stop  (uiox_radar_device_t *dev);
 
 /** @brief  Close device and release all resources. */
 void uiox_radar_close (uiox_radar_device_t *dev);
 
 /**
  * @brief  Process one radar frame and return a point cloud.
  *
  * Non-blocking — returns NULL if no new ADC frame is ready.
  * The returned pointer is valid until the next call to this function.
  *
  * @param  dev   Opened, started radar device.
  * @return Point cloud, or NULL if not yet ready.
  */
 uiox_radar_point_cloud_t *uiox_radar_get_point_cloud(
     uiox_radar_device_t *dev);
 
 /** @brief  Update chirp configuration mid-stream. */
 int  uiox_radar_set_chirp(uiox_radar_device_t        *dev,
                            const uiox_radar_chirp_cfg_t *chirp);
 
 /** @brief  Get current sensor performance metrics. */
 const uiox_radar_perf_t *uiox_radar_get_perf(
     const uiox_radar_device_t *dev);
 
 /** @brief  Get snapshot of all active tracks. */
 uint16_t uiox_radar_get_tracks(const uiox_radar_device_t *dev,
                                 uiox_radar_track_t *out,
                                 uint16_t max_tracks);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_RADAR_DEVICE_H */
 