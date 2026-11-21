/*
 * 50_UIX/00_libs/00_uixlibs/dev/hdmi/uiox_hdmi_device.h
 *
 * UIOX HDMI top-level application-facing device API — EL0.
 *
 *   21_apps/app_hdmi.c → dev/hdmi/uiox_hdmi_device.c
 *     → dev/uiox_dev_stream.c → 40_SCIX → 30_KIX
 *
 * No 30_KIX header.
 *
 * ── CHANGED from the kernel-side original ───────────────────────────
 *   · "uiox_hdmi_subsys.h" removed — it is EL1.  The state enum it
 *     carried is MIRRORED here, source named beside it.
 *   · uiox_hdmi_open_params_t lost hw/hw_ops/cs/bpc/pref_* — bring-up
 *     arguments an EL0 caller does not have.  dpms_timeout_ms is KEPT:
 *     it is a policy the app chooses and row 422 carries it.
 *   · uiox_hdmi_device_t holds a uiox_dev_handle_t, not a
 *     uiox_hdmi_subsys_t.
 *
 * ── WHICH KERNEL ROWS EXIST ─────────────────────────────────────────
 *   421  hdmi_state()          state()
 *   422  hdmi_dpms(timeout_ms) open()'s dpms_timeout_ms
 *   423  hdmi_activity(now_ms) activity()
 *   424  hdmi_mode(packed)     open()'s pref_w / pref_h / pref_hz
 *
 * ── CONTROL-ONLY, AND THE GATE IS WHY ───────────────────────────────
 * Every row above is a VALUE, so this layer has no user-address check
 * anywhere — like camera and monitor.
 *
 *   acquire() / present(fb)
 *       The triple buffer is held as fb_display / fb_render /
 *       fb_pending POINTERS inside the subsystem, and the pool lives in
 *       32_FS/00_drvbuff/01_Com/hdmi/uiox_hdmi_buf.h.  A frame must
 *       survive BETWEEN TWO SYSCALLS — acquire, render, present — and
 *       then be flipped on the next VBlank.  The trap carries no state
 *       across calls, so neither can be a row.
 *
 *       ⚠ UNBLOCKING IT NEEDS A POOL INDEX the subsystem resolves — a
 *         small integer the caller holds instead of a pointer.  That is
 *         a kernel-side addition to uiox_hdmi_buf.h, not something this
 *         layer can invent.  MONITOR shares the decision.
 *
 *   set_hdr / set_audio / audio_write / cec_send
 *       take structures and byte buffers the kernel would have to gate,
 *       and no row exists for any of them.
 *   tick / set_evt_cb / get_resolution / print_*
 *       the kernel's timer, a callback into EL0, and kernel state.
 */
#ifndef UIOX_HDMI_DEVICE_H
#define UIOX_HDMI_DEVICE_H

#include "../uiox_dev_stream.h"
#include "../uiox_dev.h"

#ifdef __cplusplus
extern "C" {
#endif

#define UIOX_HDMI_DEVPATH      "hdmi"
#define UIOX_HDMI_DPMS_MAX     0xFFFFFFFFull

/* uiox_hdmi_subsys.h */
typedef enum {
    UIOX_HDMI_SUBSYS_STOPPED = 0,
    UIOX_HDMI_SUBSYS_PROBING,
    UIOX_HDMI_SUBSYS_RUNNING,
    UIOX_HDMI_SUBSYS_SUSPENDED,
} uiox_hdmi_subsys_state_t;

/* uiox_hdmi_proto.h */
typedef enum {
    UIOX_HDMI_EVT_CONNECT = 0,
    UIOX_HDMI_EVT_DISCONNECT,
    UIOX_HDMI_EVT_HDCP_AUTH,
    UIOX_HDMI_EVT_HDCP_FAIL,
    UIOX_HDMI_EVT_VBLANK,
    UIOX_HDMI_EVT_AUDIO_UNDERRUN,
    UIOX_HDMI_EVT_ERROR,
} uiox_hdmi_evt_t;

typedef void (*uiox_hdmi_evt_cb_t)(uiox_hdmi_evt_t evt, void *ctx);

/* ── what an EL0 caller chooses at open ──────────────────────────── */
typedef struct {
    uix_uint16_t  pref_w;
    uix_uint16_t  pref_h;
    uix_uint8_t   pref_hz;
    uix_uint32_t  dpms_timeout_ms;   /**< 0 = never blank */
} uiox_hdmi_open_params_t;

/* ── the device handle ───────────────────────────────────────────────
 * The shape's own handle.  No HAL: the kernel owns the transmitter, and
 * /dev/hdmi0 is how it is reached. */
typedef struct {
    uiox_dev_handle_t        h;
    uiox_hdmi_subsys_state_t last_state;
    uix_bool_t               open;
} uiox_hdmi_device_t;

/* ── the API ───────────────────────────────────────────────────────── */
uiox_dev_err_t uiox_hdmi_open    (uiox_hdmi_device_t *dev,
                                  const uiox_hdmi_open_params_t *p,
                                  int unit, int flags);
uiox_dev_err_t uiox_hdmi_close   (uiox_hdmi_device_t *dev);

uiox_dev_err_t uiox_hdmi_activity(uiox_hdmi_device_t *dev, uix_uint32_t now_ms);
uiox_dev_err_t uiox_hdmi_set_dpms(uiox_hdmi_device_t *dev, uix_uint32_t timeout_ms);

uiox_hdmi_subsys_state_t uiox_hdmi_state(const uiox_hdmi_device_t *dev);

const char *uiox_hdmi_state_name(uiox_hdmi_subsys_state_t s);
const char *uiox_hdmi_evt_name  (uiox_hdmi_evt_t evt);

#endif /* UIOX_HDMI_DEVICE_H */
