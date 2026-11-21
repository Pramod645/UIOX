#include "uiox_kix_css_cam.h"

#define CAM_EINVAL  ((int64_t)-22)
#define CAM_ENODEV  ((int64_t)-19)

/* sensor_set_gain takes uint16_t hundredths; sensor_set_exposure uint32_t µs */
#define CAM_GAIN_MAX   ((uiox_uint64_t)65535u)

int64_t uiox_kix_css_cam_state(void)
{
    uiox_cam_pipeline_t *pl = uiox_kix_css_cam_current();

    if (!pl) return CAM_ENODEV;

    return (int64_t)pl->state;   /* UIOX_CAM_STATE_STOPPED / _STREAMING */
}

int64_t uiox_kix_css_cam_exposure(uiox_uint64_t us)
{
    uiox_cam_pipeline_t *pl = uiox_kix_css_cam_current();

    if (!pl)                              return CAM_ENODEV;
    if (pl->state == UIOX_CAM_STATE_STOPPED) return CAM_ENODEV;

    /* µs is uint32_t in the sensor API; a larger value would truncate */
    if (us > (uiox_uint64_t)0xFFFFFFFFull) return CAM_EINVAL;

    return (int64_t)uiox_cam_sensor_set_exposure(&pl->sensor,
                                                 (uiox_uint32_t)us);
}

int64_t uiox_kix_css_cam_gain(uiox_uint64_t gain_x100)
{
    uiox_cam_pipeline_t *pl = uiox_kix_css_cam_current();

    if (!pl)                              return CAM_ENODEV;
    if (pl->state == UIOX_CAM_STATE_STOPPED) return CAM_ENODEV;

    /* the sensor API takes uint16_t hundredths */
    if (gain_x100 > CAM_GAIN_MAX)         return CAM_EINVAL;

    return (int64_t)uiox_cam_sensor_set_gain(&pl->sensor,
                                             (uiox_uint16_t)gain_x100);
}

int64_t uiox_kix_css_cam_stream(uiox_uint64_t on)
{
    uiox_cam_pipeline_t *pl = uiox_kix_css_cam_current();

    if (!pl) return CAM_ENODEV;

    if (on)
        return (int64_t)uiox_cam_pipeline_start(pl);

    uiox_cam_pipeline_stop(pl);
    return 0;                     /* stop returns void */
}
