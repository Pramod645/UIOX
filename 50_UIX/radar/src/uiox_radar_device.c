/**
 * @file    uiox_radar_device.c
 * @brief   UIOX Radar device API implementation.
 * @date    2026-05-26
 */

 #include "uiox_radar_device.h"
 #include <string.h>
 #include <errno.h>
 
 static uiox_radar_point_cloud_t s_cloud_buf;
 
 int uiox_radar_open(uiox_radar_device_t          *dev,
                     const uiox_radar_open_params_t *p)
 {
     if (!dev || !p || !p->hw || !p->hw_ops) return -EINVAL;
     memset(dev, 0, sizeof(*dev));
     dev->hw = p->hw;
 
     /* 1. Init HAL */
     int rc = uiox_radar_hw_init(p->hw, p->hw_ops);
     if (rc < 0) return rc;
 
     /* 2. Build pipeline (detect + reset sensor) */
     rc = uiox_radar_pipeline_build(&dev->pipeline,
                                     p->hw,
                                     &p->bus,
                                     p->sensor_name,
                                     p->device_id);
     if (rc < 0) return rc;
 
     /* 3. Configure pipeline */
     rc = uiox_radar_pipeline_config(&dev->pipeline,
                                      &p->chirp,
                                      &p->dsp,
                                      &p->tracker,
                                      p->num_rx,
                                      p->num_tx,
                                      p->if_type,
                                      p->queue_count);
     if (rc < 0) return rc;
 
     dev->open = true;
     return 0;
 }
 
 int uiox_radar_start(uiox_radar_device_t *dev)
 {
     if (!dev || !dev->open) return -EINVAL;
     return uiox_radar_pipeline_start(&dev->pipeline);
 }
 
 void uiox_radar_stop(uiox_radar_device_t *dev)
 {
     if (!dev || !dev->open) return;
     uiox_radar_pipeline_stop(&dev->pipeline);
 }
 
 void uiox_radar_close(uiox_radar_device_t *dev)
 {
     if (!dev) return;
     uiox_radar_stop(dev);
     uiox_radar_hw_deinit(dev->hw);
     dev->open = false;
 }
 
 uiox_radar_point_cloud_t *uiox_radar_get_point_cloud(
     uiox_radar_device_t *dev)
 {
     if (!dev || !dev->open) return NULL;
     return uiox_radar_pipeline_process(&dev->pipeline, &s_cloud_buf);
 }
 
 int uiox_radar_set_chirp(uiox_radar_device_t          *dev,
                           const uiox_radar_chirp_cfg_t *chirp)
 {
     if (!dev || !chirp) return -EINVAL;
     return uiox_radar_sensor_config(&dev->pipeline.sensor, chirp);
 }
 
 const uiox_radar_perf_t *uiox_radar_get_perf(
     const uiox_radar_device_t *dev)
 {
     if (!dev) return NULL;
     return uiox_radar_sensor_perf(&dev->pipeline.sensor);
 }
 
 uint16_t uiox_radar_get_tracks(const uiox_radar_device_t *dev,
                                 uiox_radar_track_t *out,
                                 uint16_t max_tracks)
 {
     if (!dev || !out || !max_tracks) return 0;
     uint16_t count = 0;
     for (uint16_t i = 0;
          i < dev->pipeline.tracker_cfg.max_tracks && count < max_tracks;
          i++) {
         const uiox_radar_track_t *t = &dev->pipeline.tracks[i];
         if (t->state == UIOX_TRACK_CONFIRMED) {
             memcpy(&out[count++], t, sizeof(*t));
         }
     }
     return count;
 }
 