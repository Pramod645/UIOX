/**
 * @file    uiox_radar_subsys.h
 * @brief   UIOX Radar subsystem — pipeline, tracking, point cloud output.
 *
 * Assembles sensor + interface + DSP into a complete processing pipeline
 * and adds a lightweight Kalman-filter tracker to produce a stable
 * point-cloud output for application consumption.
 *
 * @date    2026-05-26
 */
//Layer 4 — Radar Subsystem
 #ifndef UIOX_RADAR_SUBSYS_H
 #define UIOX_RADAR_SUBSYS_H
 
 #include "uiox_radar_if.h"
 #include "uiox_radar_sensor.h"
 #include "uiox_radar_dsp.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * Tracker configuration
  * ====================================================================== */
 
 #define UIOX_RADAR_MAX_TRACKS   32
 
 typedef struct {
     uint16_t  max_tracks;
     float     gate_range_m;       /**< Association gate in range (m)        */
     float     gate_velocity_mps;  /**< Association gate in velocity (m/s)   */
     float     gate_azimuth_deg;   /**< Association gate in azimuth (deg)    */
     uint8_t   max_misses;         /**< Frames without update before delete  */
     uint8_t   confirm_hits;       /**< Hits before track promoted to active */
 } uiox_radar_tracker_cfg_t;
 
 /* =========================================================================
  * Track state (per target)
  * ====================================================================== */
 
 typedef enum {
     UIOX_TRACK_TENTATIVE = 0,
     UIOX_TRACK_CONFIRMED,
     UIOX_TRACK_DELETED,
 } uiox_track_state_t;
 
 typedef struct {
     uint16_t           id;
     uiox_track_state_t state;
     float              range_m;
     float              velocity_mps;
     float              azimuth_deg;
     float              elevation_deg;
     float              snr_db;
     uint8_t            age;        /**< Frames this track has been active   */
     uint8_t            misses;     /**< Consecutive frames without update   */
     uint8_t            hits;       /**< Total detection hits                */
 } uiox_radar_track_t;
 
 /* =========================================================================
  * Point cloud entry (output to application)
  * ====================================================================== */
 
 typedef struct {
     float    x_m;           /**< Cartesian X (forward)                     */
     float    y_m;           /**< Cartesian Y (lateral)                     */
     float    z_m;           /**< Cartesian Z (height)                      */
     float    velocity_mps;  /**< Radial velocity                           */
     float    snr_db;        /**< SNR of detection                          */
     uint16_t track_id;      /**< Associated track ID (0xFFFF = untracked)  */
 } uiox_radar_point_t;
 
 #define UIOX_RADAR_MAX_POINTS   128
 
 typedef struct {
     uiox_radar_point_t points[UIOX_RADAR_MAX_POINTS];
     uint16_t           num_points;
     uint32_t           frame_id;
     uint64_t           ts_ns;
 } uiox_radar_point_cloud_t;
 
 /* =========================================================================
  * Pipeline
  * ====================================================================== */
 
 typedef enum {
     UIOX_RADAR_PIPE_STOPPED = 0,
     UIOX_RADAR_PIPE_STREAMING,
 } uiox_radar_pipe_state_t;
 
 typedef struct {
     uiox_radar_if_t          rif;
     uiox_radar_sensor_t      sensor;
     uiox_radar_dsp_t         dsp;
     uiox_radar_pipe_state_t  state;
 
     /* Tracker */
     uiox_radar_tracker_cfg_t tracker_cfg;
     uiox_radar_track_t       tracks[UIOX_RADAR_MAX_TRACKS];
     uint16_t                 next_track_id;
 } uiox_radar_pipeline_t;
 
 /* =========================================================================
  * Subsystem API
  * ====================================================================== */
 
 int  uiox_radar_pipeline_build(uiox_radar_pipeline_t    *pl,
                                 uiox_radar_hw_t          *hw,
                                 const uiox_radar_bus_ops_t *bus,
                                 const char               *sensor_name,
                                 uint16_t                  device_id);
 
 int  uiox_radar_pipeline_config(uiox_radar_pipeline_t    *pl,
                                  const uiox_radar_chirp_cfg_t *chirp,
                                  const uiox_radar_dsp_cfg_t   *dsp_cfg,
                                  const uiox_radar_tracker_cfg_t *trk_cfg,
                                  uint8_t num_rx, uint8_t num_tx,
                                  uiox_radar_if_type_t if_type,
                                  uint8_t queue_count);
 
 int  uiox_radar_pipeline_start(uiox_radar_pipeline_t *pl);
 void uiox_radar_pipeline_stop (uiox_radar_pipeline_t *pl);
 
 /**
  * @brief  Process one radar frame: dequeue ADC, run DSP, update tracker,
  *         produce point cloud. Non-blocking — returns NULL if no frame ready.
  */
 uiox_radar_point_cloud_t *uiox_radar_pipeline_process(
     uiox_radar_pipeline_t    *pl,
     uiox_radar_point_cloud_t *cloud_buf);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_RADAR_SUBSYS_H */
 