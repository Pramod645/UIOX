/*
 * 34_CSS/02_Sensors/camera/uiox_kix_css_cam.h
 *
 * CAMERA's syscall entry points — numbers 417-420, CSS-PRIVATE.
 *
 * ── FOUR ROWS, AND NONE OF THEM NEEDS A GATE ────────────────────────
 * Every one takes or returns a VALUE: a state, a microsecond exposure, a
 * gain in hundredths, a boolean.  uiox_kix_sccss.h declares no
 * user-address gate because whether one is needed depends on what a
 * family's calls take — and this family's take none.  That makes camera
 * the cleanest set in the arm so far.
 *
 * ── WHAT IS NOT HERE, and why ───────────────────────────────────────
 *   dequeue()
 *       returns uiox_cam_frame_t *, and its own header says "ownership
 *       is transferred to caller".  The payload is f->paddr — a PHYSICAL
 *       address, which the demo's code prints directly.  There is
 *       nothing to copy into a caller buffer: a frame is a device-memory
 *       descriptor only something able to program DMA can consume.
 *       HDMI's FLIP and TB4's get_router hit the same wall.
 *   prime(count) / buf_free()
 *       pool operations on kernel-owned frames.  A trap cannot hand out
 *       a frame it cannot later identify.
 *   build() / config()
 *       bring-up.  count, stride, lanes and the CSI data type arrive at
 *       probe time, not from EL0.
 *
 * ── one instance ────────────────────────────────────────────────────
 * Slots are ignored: uiox_cam_probe() registers one camera.
 */
#ifndef UIOX_KIX_CSS_CAM_H
#define UIOX_KIX_CSS_CAM_H

#include "uiox_kix_sccss.h"
#include "uiox_cam_subsys.h"     /* uiox_cam_pipeline_t, the state enum */

uiox_cam_pipeline_t *uiox_kix_css_cam_current(void);

int64_t uiox_kix_css_cam_state   (void);
int64_t uiox_kix_css_cam_exposure(uiox_uint64_t us);
int64_t uiox_kix_css_cam_gain    (uiox_uint64_t gain_x100);
int64_t uiox_kix_css_cam_stream  (uiox_uint64_t on);

#endif /* UIOX_KIX_CSS_CAM_H */
