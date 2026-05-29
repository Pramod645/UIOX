/**
 * @file    uiox_radar_subsys.c
 * @brief   UIOX Radar subsystem — pipeline, tracking, point cloud.
 * @date    2026-05-26
 */

 #include "uiox_radar_subsys.h"
 #include <string.h>
 #include <math.h>
 #include <errno.h>
 
 #ifndef M_PI
 #define M_PI 3.14159265358979323846
 #endif
 
 /* =========================================================================
  * Pipeline build
  * ====================================================================== */
 
 int uiox_radar_pipeline_build(uiox_radar_pipeline_t    *pl,
                                uiox_radar_hw_t          *hw,
                                const uiox_radar_bus_ops_t *bus,
                                const char               *sensor_name,
                                uint16_t                  device_id)
 {
     if (!pl || !hw || !bus) return -EINVAL;
     memset(pl, 0, sizeof(*pl));
 
     /* Wire sensor */
     pl->sensor.name      = sensor_name;
     pl->sensor.device_id = device_id;
     pl->sensor.bus       = bus;
 
     /* Wire interface hw */
     pl->rif.hw = hw;
 
     /* Detect and reset sensor */
     int rc = uiox_radar_sensor_detect(&pl->sensor);
     if (rc < 0) return rc;
 
     rc = uiox_radar_sensor_reset(&pl->sensor);
     if (rc < 0) return rc;
 
     pl->next_track_id = 1;
     return 0;
 }
 
 /* =========================================================================
  * Pipeline config
  * ====================================================================== */
 
 int uiox_radar_pipeline_config(uiox_radar_pipeline_t      *pl,
                                 const uiox_radar_chirp_cfg_t *chirp,
                                 const uiox_radar_dsp_cfg_t   *dsp_cfg,
                                 const uiox_radar_tracker_cfg_t *trk_cfg,
                                 uint8_t  num_rx,
                                 uint8_t  num_tx,
                                 uiox_radar_if_type_t if_type,
                                 uint8_t  queue_count)
 {
     if (!pl || !chirp || !dsp_cfg || !trk_cfg) return -EINVAL;
 
     /* 1. Programme sensor chirp */
     int rc = uiox_radar_sensor_config(&pl->sensor, chirp);
     if (rc < 0) return rc;
 
     /* 2. Configure data interface */
     rc = uiox_radar_if_config(&pl->rif, pl->rif.hw, if_type,
                                num_rx, num_tx,
                                chirp->num_chirps,
                                chirp->num_adc_samples,
                                UIOX_RADAR_ADC_COMPLEX_1X);
     if (rc < 0) return rc;
 
     /* 3. Prime DMA buffers */
    /* 3. Prime DMA buffers */
    rc = uiox_radar_if_prime(&pl->rif, queue_count);
    if (rc < 0) return rc;

    /* 4. Init DSP context */
    rc = uiox_radar_dsp_init(&pl->dsp, dsp_cfg);
    if (rc < 0) return rc;

    /* 5. Copy tracker config */
    memcpy(&pl->tracker_cfg, trk_cfg, sizeof(*trk_cfg));
    memset(pl->tracks, 0, sizeof(pl->tracks));

    return 0;
}

/* =========================================================================
 * Start / Stop
 * ====================================================================== */

int uiox_radar_pipeline_start(uiox_radar_pipeline_t *pl)
{
    if (!pl) return -EINVAL;

    int rc = uiox_radar_sensor_stream(&pl->sensor, true);
    if (rc < 0) return rc;

    rc = uiox_radar_hw_start(pl->rif.hw);
    if (rc < 0) return rc;

    pl->state = UIOX_RADAR_PIPE_STREAMING;
    return 0;
}

void uiox_radar_pipeline_stop(uiox_radar_pipeline_t *pl)
{
    if (!pl) return;
    uiox_radar_hw_stop(pl->rif.hw);
    uiox_radar_sensor_stream(&pl->sensor, false);
    uiox_radar_dsp_deinit(&pl->dsp);
    pl->state = UIOX_RADAR_PIPE_STOPPED;
}

/* =========================================================================
 * Tracker — simple nearest-neighbour association
 * ====================================================================== */

static float track_dist(const uiox_radar_track_t    *t,
                         const uiox_radar_detection_t *d)
{
    float dr  = t->range_m      - d->range_m;
    float dv  = t->velocity_mps - d->velocity_mps;
    float daz = t->azimuth_deg  - d->azimuth_deg;
    return sqrtf(dr*dr + dv*dv + daz*daz);
}

static void tracker_update(uiox_radar_pipeline_t    *pl,
                            uiox_radar_det_frame_t   *dets,
                            const uiox_radar_perf_t  *perf)
{
    const uiox_radar_tracker_cfg_t *cfg = &pl->tracker_cfg;

    /* Mark all active tracks as unupdated this frame */
    bool updated[UIOX_RADAR_MAX_TRACKS] = {false};

    /* --- Associate detections to tracks --- */
    for (uint16_t di = 0; di < dets->num_detections; di++) {
        uiox_radar_detection_t *det = &dets->detections[di];

        /* Resolve physical units from bin indices */
        if (perf) {
            det->range_m      = (float)det->range_bin    * perf->range_fft_bin_m;
            det->velocity_mps = ((float)det->doppler_bin -
                                 (float)(pl->dsp.cfg.doppler_fft_size / 2))
                                 * perf->doppler_fft_bin_mps;
        }

        /* Find nearest track within gate */
        float   best_dist = 1e9f;
        int16_t best_idx  = -1;

        for (uint16_t ti = 0; ti < cfg->max_tracks; ti++) {
            uiox_radar_track_t *t = &pl->tracks[ti];
            if (t->state == UIOX_TRACK_DELETED) continue;

            float dist = track_dist(t, det);
            if (dist < best_dist &&
                fabsf(t->range_m      - det->range_m)      < cfg->gate_range_m &&
                fabsf(t->velocity_mps - det->velocity_mps) < cfg->gate_velocity_mps &&
                fabsf(t->azimuth_deg  - det->azimuth_deg)  < cfg->gate_azimuth_deg) {
                best_dist = dist;
                best_idx  = (int16_t)ti;
            }
        }

        if (best_idx >= 0) {
            /* Update existing track */
            uiox_radar_track_t *t = &pl->tracks[best_idx];
            /* Simple α-β filter update (α=0.5) */
            t->range_m      = 0.5f * t->range_m      + 0.5f * det->range_m;
            t->velocity_mps = 0.5f * t->velocity_mps + 0.5f * det->velocity_mps;
            t->azimuth_deg  = 0.5f * t->azimuth_deg  + 0.5f * det->azimuth_deg;
            t->snr_db       = det->snr_db;
            t->misses       = 0;
            t->hits++;
            t->age++;
            if (t->state == UIOX_TRACK_TENTATIVE &&
                t->hits >= cfg->confirm_hits)
                t->state = UIOX_TRACK_CONFIRMED;
            updated[best_idx] = true;
        } else {
            /* Spawn new tentative track */
            for (uint16_t ti = 0; ti < cfg->max_tracks; ti++) {
                if (pl->tracks[ti].state == UIOX_TRACK_DELETED) {
                    uiox_radar_track_t *t = &pl->tracks[ti];
                    memset(t, 0, sizeof(*t));
                    t->id           = pl->next_track_id++;
                    t->state        = UIOX_TRACK_TENTATIVE;
                    t->range_m      = det->range_m;
                    t->velocity_mps = det->velocity_mps;
                    t->azimuth_deg  = det->azimuth_deg;
                    t->snr_db       = det->snr_db;
                    t->hits         = 1;
                    t->age          = 1;
                    updated[ti]     = true;
                    break;
                }
            }
        }
    }

    /* --- Age unupdated tracks --- */
    for (uint16_t ti = 0; ti < cfg->max_tracks; ti++) {
        uiox_radar_track_t *t = &pl->tracks[ti];
        if (t->state == UIOX_TRACK_DELETED) continue;
        if (!updated[ti]) {
            t->misses++;
            t->age++;
            if (t->misses >= cfg->max_misses)
                t->state = UIOX_TRACK_DELETED;
        }
    }
}

/* =========================================================================
 * Detection to point cloud conversion (spherical → Cartesian)
 * ====================================================================== */

static void build_point_cloud(const uiox_radar_pipeline_t  *pl,
                               const uiox_radar_det_frame_t *dets,
                               uiox_radar_point_cloud_t     *cloud)
{
    cloud->num_points = 0;
    cloud->frame_id   = dets->frame_id;
    cloud->ts_ns      = dets->ts_ns;

    for (uint16_t di = 0;
         di < dets->num_detections && cloud->num_points < UIOX_RADAR_MAX_POINTS;
         di++) {

        const uiox_radar_detection_t *det = &dets->detections[di];
        uiox_radar_point_t *pt = &cloud->points[cloud->num_points++];

        float az_rad = det->azimuth_deg   * (float)(M_PI / 180.0);
        float el_rad = det->elevation_deg * (float)(M_PI / 180.0);

        pt->x_m          = det->range_m * cosf(el_rad) * cosf(az_rad);
        pt->y_m          = det->range_m * cosf(el_rad) * sinf(az_rad);
        pt->z_m          = det->range_m * sinf(el_rad);
        pt->velocity_mps = det->velocity_mps;
        pt->snr_db       = det->snr_db;

        /* Find associated confirmed track */
        pt->track_id = 0xFFFFu;
        for (uint16_t ti = 0; ti < pl->tracker_cfg.max_tracks; ti++) {
            const uiox_radar_track_t *t = &pl->tracks[ti];
            if (t->state != UIOX_TRACK_CONFIRMED) continue;
            if (fabsf(t->range_m    - det->range_m)    < 0.5f &&
                fabsf(t->azimuth_deg - det->azimuth_deg) < 5.0f) {
                pt->track_id = t->id;
                break;
            }
        }
    }
}

/* =========================================================================
 * Main pipeline process tick
 * ====================================================================== */

uiox_radar_point_cloud_t *uiox_radar_pipeline_process(
    uiox_radar_pipeline_t    *pl,
    uiox_radar_point_cloud_t *cloud_buf)
{
    if (!pl || !cloud_buf) return NULL;
    if (pl->state != UIOX_RADAR_PIPE_STREAMING) return NULL;

    /* 1. Dequeue completed ADC frame */
    uiox_radar_frame_t *raw = uiox_radar_if_dequeue(&pl->rif);
    if (!raw) return NULL;

    /* 2. Allocate DSP detection output */
    static uiox_radar_det_frame_t s_det_frame;
    memset(&s_det_frame, 0, sizeof(s_det_frame));

    /* 3. Run full DSP chain */
    int rc = uiox_radar_dsp_process(&pl->dsp, raw, &s_det_frame);

    /* 4. Return raw buffer to DMA ring immediately */
    uiox_radar_buf_free(raw);
    uiox_radar_frame_t *fresh = uiox_radar_buf_alloc_raw();
    if (fresh) {
        const uiox_radar_hw_ops_t *ops =
            (const uiox_radar_hw_ops_t *)pl->rif.hw->priv;
        if (ops && ops->dma_queue)
            ops->dma_queue(pl->rif.hw, fresh->paddr, pl->rif.frame_bytes);
    }

    if (rc < 0) return NULL;

    /* 5. Update tracker */
    tracker_update(pl, &s_det_frame, uiox_radar_sensor_perf(&pl->sensor));

    /* 6. Build point cloud output */
    build_point_cloud(pl, &s_det_frame, cloud_buf);

    return cloud_buf;
}

 