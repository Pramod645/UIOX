/*
 * 50_UIX/00_libs/00_uixlibs/dev/cam/uiox_cam_device.h
 *
 * UIOX camera top-level application-facing device API — EL0.
 *
 *   21_apps/app_cam.c → dev/cam/uiox_cam_device.c
 *     → dev/uiox_dev_stream.c → 40_SCIX → 30_KIX
 *
 * No 30_KIX header.
 *
 * ── CHANGED from the kernel-side original ───────────────────────────
 *   · "uiox_cam_subsys.h" removed — it is EL1.  The enums it carried
 *     are MIRRORED here, source named beside each.
 *   · uiox_cam_device_t holds a uiox_dev_handle_t, not a
 *     uiox_cam_pipeline_t.
 *
 * ── uiox_cam_open_params_t: KEPT, with the HAL half removed ─────────
 * The original carried both halves and they are not the same thing.
 *
 *   REMOVED — the kernel probe's, an EL0 caller has none of them:
 *     hw, hw_ops                  the CSI-2 controller
 *     bus                         i2c_write8/read8/delay_ms
 *     sensor_name, i2c_addr       "imx219" @ 0x10 — the probe matched it
 *     chip_id_reg, chip_id_val
 *     lanes, virt_chan, csi_dt     CSI-2 wiring, fixed at probe
 *     queue_count                 how many DMA buffers the pool primes
 *
 *   KEPT — the APPLICATION's choice, and the rows carry none of them:
 *     width, height, fps          a mode is the app's call
 *     pixfmt, stride              tied to the mode
 *
 * Exposure and gain are NOT here: they have their own rows (418, 419)
 * and change mid-stream, so they are separate calls rather than open
 * arguments.
 *
 * ── THE ONLY FAMILY-2 DEVICE WITH NO GATE ───────────────────────────
 * Every row takes or returns a VALUE.  No pointer crosses the trap in
 * either direction, so this layer has no user-address check anywhere —
 * unlike mic's sample/byte read or spk's pair/byte write.
 *
 * ── WHAT IS NOT HERE, and why ───────────────────────────────────────
 *   capture_try()   returns uiox_cam_frame_t *, and the demo prints
 *                   f->paddr from it — a PHYSICAL address.  Nothing to
 *                   copy into a caller buffer: a frame is a
 *                   device-memory descriptor only something able to
 *                   program DMA can consume.  HDMI's FLIP and TB4's
 *                   get_router hit the same wall.
 *   buf_alloc / buf_free / buf_free_count   pool operations on
 *                   kernel-owned frames.
 */
#ifndef UIOX_CAM_DEVICE_H
#define UIOX_CAM_DEVICE_H

#include "../uiox_dev_stream.h"
#include "../uiox_dev.h"

#ifdef __cplusplus
extern "C" {
#endif

#define UIOX_CAM_DEVPATH      "cam"
#define UIOX_CAM_GAIN_MAX   65535u     /* uint16_t hundredths */
#define UIOX_CAM_FPS_MAX      255u

/* uiox_cam_subsys.h */
typedef enum {
    UIOX_CAM_STATE_STOPPED = 0,
    UIOX_CAM_STATE_STREAMING,
} uiox_cam_state_t;

/* uiox_cam_subsys.h — the pixel formats the kernel's config takes.
 * Values MUST stay identical: they cross the trap as integers. */
typedef enum {
    UIOX_CAM_PIX_FMT_RAW8 = 0,
    UIOX_CAM_PIX_FMT_RAW10,
    UIOX_CAM_PIX_FMT_RAW12,
    UIOX_CAM_PIX_FMT_YUV420,
    UIOX_CAM_PIX_FMT_YUV422,
    UIOX_CAM_PIX_FMT_RGB888,
} uiox_cam_pixfmt_t;

/* ── what an EL0 caller chooses at open ──────────────────────────── */
typedef struct {
    uix_uint16_t  width;
    uix_uint16_t  height;
    uix_uint8_t   fps;
    uix_uint8_t   pixfmt;      /* uiox_cam_pixfmt_t */
    uix_uint32_t  stride;      /* bytes per line    */
} uiox_cam_open_params_t;

/* ── the device handle ───────────────────────────────────────────────
 * The shape's own handle.  No HAL, no bus, no pipeline: the kernel owns
 * all three, and /dev/cam0 is how they are reached. */
typedef struct {
    uiox_dev_handle_t  h;
    uiox_cam_state_t   last_state;
    uix_bool_t         open;
} uiox_cam_device_t;

/* ── the API ─────────────────────────────────────────────────────────
 * open takes the mode; the four rows (417-420) carry the controls.
 */
uiox_dev_err_t uiox_cam_open        (uiox_cam_device_t *dev,
                                     const uiox_cam_open_params_t *p,
                                     int unit, int flags);
uiox_dev_err_t uiox_cam_close       (uiox_cam_device_t *dev);

uiox_dev_err_t uiox_cam_set_exposure(uiox_cam_device_t *dev, uix_uint32_t us);
uiox_dev_err_t uiox_cam_set_gain    (uiox_cam_device_t *dev, uix_uint16_t x100);
uiox_dev_err_t uiox_cam_stream      (uiox_cam_device_t *dev, uix_bool_t on);

uiox_cam_state_t uiox_cam_state(const uiox_cam_device_t *dev);

const char *uiox_cam_state_name (uiox_cam_state_t s);
const char *uiox_cam_pixfmt_name(uiox_cam_pixfmt_t f);

#ifdef __cplusplus
}
#endif
#endif /* UIOX_CAM_DEVICE_H */
