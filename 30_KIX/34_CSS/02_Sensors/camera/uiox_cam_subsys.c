#include "uiox_cam_subsys.h"
#include <string.h>
#include <errno.h>

int uiox_cam_pipeline_build(uiox_cam_pipeline_t *pl,
                            uiox_cam_hw_t *hw,
                            const uiox_cam_bus_ops_t *bus,
                            const char *sensor_name,
                            uint8_t i2c_addr,
                            uint16_t chip_id_reg,
                            uint16_t chip_id_val)
{
    if (!pl || !hw || !bus) return -EINVAL;
    memset(pl, 0, sizeof(*pl));
    pl->sensor.name       = sensor_name;
    pl->sensor.i2c_addr   = i2c_addr;
    pl->sensor.chip_id_reg= chip_id_reg;
    pl->sensor.chip_id_val= chip_id_val;
    pl->sensor.bus        = bus;

    /* Detect and reset sensor */
    int rc = uiox_cam_sensor_detect(&pl->sensor);
    if (rc < 0) return rc;
    rc = uiox_cam_sensor_reset(&pl->sensor);
    if (rc < 0) return rc;

    /* Ready to configure interface later */
    (void)hw;
    return 0;
}

int uiox_cam_pipeline_config(uiox_cam_pipeline_t *pl,
                             uint16_t w, uint16_t h, uint8_t fps,
                             uiox_cam_pixfmt_t fmt,
                             uint8_t lanes, uint8_t vc,
                             uiox_csi_dt_t dt,
                             uint32_t stride,
                             uint8_t queue_count)
{
    if (!pl || !pl->cif.hw) return -EINVAL;

    /* Program sensor mode first */
    int rc = uiox_cam_sensor_set_mode(&pl->sensor, w, h, fps,
                                      /* link freq */ 400000000u);
    if (rc < 0) return rc;

    /* Configure interface */
    rc = uiox_cam_if_config(&pl->cif, pl->cif.hw, lanes, vc, dt, fmt, w, h, stride);
    if (rc < 0) return rc;

    /* Prime buffers */
    rc = uiox_cam_if_prime(&pl->cif, queue_count);
    if (rc < 0) return rc;
    pl->buffers = (uint8_t)rc;
    pl->primed = 1;
    return 0;
}

int uiox_cam_pipeline_start(uiox_cam_pipeline_t *pl)
{
    if (!pl || !pl->primed) return -EINVAL;
    int rc = uiox_cam_sensor_stream(&pl->sensor, true);
    if (rc < 0) return rc;
    rc = uiox_cam_hw_start(pl->cif.hw);
    if (rc < 0) return rc;
    pl->state = UIOX_CAM_STATE_STREAMING;
    return 0;
}

void uiox_cam_pipeline_stop(uiox_cam_pipeline_t *pl)
{
    if (!pl) return;
    uiox_cam_hw_stop(pl->cif.hw);
    uiox_cam_sensor_stream(&pl->sensor, false);
    pl->state = UIOX_CAM_STATE_STOPPED;
}

uiox_cam_frame_t *uiox_cam_pipeline_dequeue(uiox_cam_pipeline_t *pl)
{
    if (!pl || pl->state != UIOX_CAM_STATE_STREAMING) return NULL;
    return uiox_cam_if_complete(&pl->cif);
}
