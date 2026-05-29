#include "uiox_cam_device.h"
#include "uiox_cam_hw.h"
#include <string.h>
#include <errno.h>
 
 int uiox_cam_open(uiox_cam_device_t *dev, const uiox_cam_open_params_t *p)
 {
     if (!dev || !p || !p->hw || !p->hw_ops) return -EINVAL;
     memset(dev, 0, sizeof(*dev));
     dev->hw = p->hw;
 
     /* 1. Bind HAL ops and initialise hardware */
     int rc = uiox_cam_hw_init(p->hw, p->hw_ops);
     if (rc < 0) return rc;
 
     /* 2. Build pipeline: detect sensor, reset it */
     rc = uiox_cam_pipeline_build(&dev->pipeline,
                                   p->hw,
                                   &p->bus,
                                   p->sensor_name,
                                   p->i2c_addr,
                                   p->chip_id_reg,
                                   p->chip_id_val);
     if (rc < 0) return rc;
 
     /* 3. Wire hw into the pipeline interface before config */
     dev->pipeline.cif.hw = p->hw;
 
     /* 4. Configure mode, interface, and prime DMA buffers */
     rc = uiox_cam_pipeline_config(&dev->pipeline,
                                    p->width,
                                    p->height,
                                    p->fps,
                                    p->pixfmt,
                                    p->lanes,
                                    p->virt_chan,
                                    p->csi_dt,
                                    p->stride,
                                    p->queue_count);
     if (rc < 0) return rc;
 
     return 0;
 }
 
 int uiox_cam_start(uiox_cam_device_t *dev)
 {
     if (!dev) return -EINVAL;
     return uiox_cam_pipeline_start(&dev->pipeline);
 }
 
 void uiox_cam_stop(uiox_cam_device_t *dev)
 {
     if (!dev) return;
     uiox_cam_pipeline_stop(&dev->pipeline);
 }
 
 uiox_cam_frame_t *uiox_cam_capture_try(uiox_cam_device_t *dev)
 {
     if (!dev) return NULL;
     return uiox_cam_pipeline_dequeue(&dev->pipeline);
 }
 
 int uiox_cam_set_exposure(uiox_cam_device_t *dev, uint32_t exposure_us)
 {
     if (!dev) return -EINVAL;
     return uiox_cam_sensor_set_exposure(&dev->pipeline.sensor, exposure_us);
 }
 
 int uiox_cam_set_gain(uiox_cam_device_t *dev, uint16_t gain_x100)
 {
     if (!dev) return -EINVAL;
     return uiox_cam_sensor_set_gain(&dev->pipeline.sensor, gain_x100);
 }
 