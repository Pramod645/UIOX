/*
 * 50_UIX/00_libs/00_uixlibs/dev/spk/uiox_spk_device.h
 *
 * UIOX Speaker top-level application-facing device API — EL0.
 *
 *   21_apps/app_spk.c → dev/spk/uiox_spk_device.c
 *     → dev/uiox_dev_stream.c → 40_SCIX → 30_KIX
 *
 * No 30_KIX header.
 *
 * ── CHANGED from the kernel-side original ───────────────────────────
 *   · "uiox_spk_subsys.h" removed — it is EL1.  The enums it carried
 *     are MIRRORED here, source named beside each.
 *   · uiox_spk_open_params_t lost hw/hw_ops/codec/fmt — bring-up
 *     arguments an EL0 caller does not have.
 *   · uiox_spk_device_t holds a uiox_dev_handle_t, not a
 *     uiox_spk_subsys_t.
 *
 * ── WHICH KERNEL ROWS EXIST ─────────────────────────────────────────
 *   413  spk_write(samples, n_stereo)  write()      ← the gate is here
 *   414  spk_volume(vol_pct)           set_volume()
 *   415  spk_mute(on)                  set_mute()
 *   416  spk_state()                   state()
 *
 * ── write() counts STEREO PAIRS ─────────────────────────────────────
 * n_stereo counts pairs, so the byte span is n_stereo * 4 — two
 * uix_int16_t per pair.  uiox_dev_stream_write takes bytes, so the
 * multiply lives in this layer, in the same unit the kernel's row 413
 * uses on the other side.
 */
#ifndef UIOX_SPK_DEVICE_H
#define UIOX_SPK_DEVICE_H

#include "../uiox_dev_stream.h"
#include "../uiox_dev.h"

#ifdef __cplusplus
extern "C" {
#endif

#define UIOX_SPK_DEVPATH          "spk"
#define UIOX_SPK_MAX_WRITE_PAIRS  32768u

/* uiox_spk_subsys.h */
typedef enum {
    UIOX_SPK_STATE_STOPPED = 0,
    UIOX_SPK_STATE_PLAYING,
    UIOX_SPK_STATE_PAUSED,
    UIOX_SPK_STATE_STOPPING,
} uiox_spk_state_t;

/* uiox_spk_subsys.h */
typedef enum {
    UIOX_SPK_EVT_START = 0,
    UIOX_SPK_EVT_STOP,
    UIOX_SPK_EVT_UNDERRUN,
    UIOX_SPK_EVT_STREAM_END,
    UIOX_SPK_EVT_VOLUME_CHANGE,
    UIOX_SPK_EVT_FAULT,
} uiox_spk_evt_t;

typedef void (*uiox_spk_evt_cb_t)(uiox_spk_evt_t evt, void *ctx);

/* ── the device handle ───────────────────────────────────────────────
 * The shape's own handle.  No HAL pointer and no codec: the kernel owns
 * both, and /dev/spk0 is how they are reached. */
typedef struct {
    uiox_dev_handle_t  h;
    uiox_spk_state_t   last_state;
    uix_bool_t         open;
} uiox_spk_device_t;

/* ── the API ─────────────────────────────────────────────────────────
 * Four verbs with kernel rows (413-416):
 */
uiox_dev_err_t uiox_spk_open      (uiox_spk_device_t *dev, int unit, int flags);
uiox_dev_err_t uiox_spk_close     (uiox_spk_device_t *dev);

uix_ssize_t    uiox_spk_write     (uiox_spk_device_t *dev,
                                   const uix_int16_t *samples,
                                   uix_uint32_t n_stereo);
uiox_dev_err_t uiox_spk_set_volume(uiox_spk_device_t *dev, uix_uint8_t vol_pct);
uiox_dev_err_t uiox_spk_set_mute  (uiox_spk_device_t *dev, uix_bool_t mute);

uiox_spk_state_t uiox_spk_state(const uiox_spk_device_t *dev);

/* ── declared, but with NO kernel row yet ────────────────────────────
 *   play(data, n_stereo, gain, loop)
 *       `loop=true` would leave the kernel holding an EL0 pointer past
 *       the return; the trap has no way to say "keep this valid until
 *       you stop stream N".  No row, so no verb here.
 *   stop_stream(id)             no trap can have created a stream.
 *   set_eq(band, hz, gain_db, q) three floats; no float convention.
 *   set_bass / set_treble       reach the codec, not the ring.
 *   tick / print_stats          the kernel's timer and its own state.
 *
 * None is declared here, so a missing symbol is a compile error rather
 * than a link error at the end.
 */

const char *uiox_spk_state_name(uiox_spk_state_t s);
const char *uiox_spk_evt_name  (uiox_spk_evt_t evt);

#ifdef __cplusplus
}
#endif
#endif /* UIOX_SPK_DEVICE_H */
