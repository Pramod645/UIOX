/*
 * 50_UIX/00_libs/00_uixlibs/dev/tpwd/uiox_dev_tpwd.h
 *
 * TPWD — the application's view of the touch-password panel, at EL0.
 *
 * ── FAMILY 3 (EVENT) ────────────────────────────────────────────────
 * The third and last EVENT device, after kbd and mouse.  Read-based:
 *
 *     uiox_tpwd_if.h:157-160
 *     typedef struct {
 *         uiox_dev_ops_t     head;  // name, probe, init, deinit, ioctl
 *         uiox_event_ops_t   ev;    // init, deinit, pending, read, isr
 *     } uiox_tpwd_ops_t;
 *
 * ── read() RETURNS THE RAW RECORD, NOT THE COOKED ONE ───────────────
 * uiox_tpwd_if.h:146-149 states it: the ring holds uiox_tpwd_raw_evt_t,
 * the RAW hardware event.  uiox_tpwd_touch_evt_t is the normalised form
 * and comes from the gesture layer, not from read().  So this layer
 * mirrors the raw record.
 *
 * ── event_read NEVER RETURNS 0 ──────────────────────────────────────
 * uiox_tpwd_if.h:151: "event_read returns bytes written or a negative
 * errno — NEVER 0."  A zero-length read is a fault, not an empty queue;
 * the layer encodes that so no app has to discover it.
 *
 * ── WHAT IS DELIBERATELY ABSENT ─────────────────────────────────────
 *   enrol / verify / delete   no command exists.  The security engine
 *                             (uiox_tpwd_sec.c) owns the credential pool
 *                             — 4 slots, HMAC-SHA256 hash, salt, attempt
 *                             counter — and sits kernel-side.  Handing
 *                             every EL0 process the enrolment path to a
 *                             credential store is a security decision,
 *                             not a wrapper.
 *   set_backlight             no command in this vocabulary
 *
 * v1.0.0  @date 2026-11-18
 */
#ifndef UIOX_DEV_TPWD_H
#define UIOX_DEV_TPWD_H

#include "../uiox_dev_event.h"     /* the EVENT shape */
#include "../sys/uix_types.h"
#include "../PoStd/uix_poll.h"

/* The node stem.  uiox_tpwd_if.h:144 says "/dev/tp<N>" — not "tpwd". */
#define UIOX_DEV_TPWD_DEVPATH  "tp"

/* Mirrored from 30_KIX/30_DSS/10_DI/02_Sensors/touchpwd/uiox_tpwd_hw.h:61 */
#define UIOX_DEV_TPWD_MAX_TOUCH_POINTS  5u

typedef struct {
    unsigned char  id;        /* touch point ID, multi-touch tracking */
    unsigned char  active;
} uiox_dev_tpwd_pt_t;

/* The kernel's uiox_tpwd_raw_evt_t (uiox_tpwd_hw.h:76-81), mirrored.
 * FIVE points, then num_points, then ts_ns, then gesture_id.  The
 * compiler inserts padding between num_points and ts_ns (the uint64_t
 * wants 8-byte alignment) and again at the end — so this must be the
 * same declaration in the same order, not a packed equivalent.  A wrong
 * bound on pts[] would shift ts_ns and gesture_id by whole points. */
typedef struct {
    uiox_dev_tpwd_pt_t pts[UIOX_DEV_TPWD_MAX_TOUCH_POINTS];
    unsigned char      num_points;
    unsigned long long ts_ns;
    unsigned char      gesture_id;   /* HW gesture code, chip-specific */
} uiox_dev_tpwd_raw_evt_t;

/* The kernel's subsystem events (uiox_tpwd_subsys.h:35-44), for
 * printing and for tracking auth state.  Nine values, 0..8. */
#define UIOX_DEV_TPWD_EVT_AUTH_OK     0
#define UIOX_DEV_TPWD_EVT_AUTH_FAIL   1
#define UIOX_DEV_TPWD_EVT_LOCKED      2
#define UIOX_DEV_TPWD_EVT_TIMEOUT     3
#define UIOX_DEV_TPWD_EVT_CANCELLED   4
#define UIOX_DEV_TPWD_EVT_ENROLLED    5
#define UIOX_DEV_TPWD_EVT_DIGIT       6
#define UIOX_DEV_TPWD_EVT_BACKSPACE   7
#define UIOX_DEV_TPWD_EVT_LOGOUT      8

typedef enum {
    UIOX_DEV_TPWD_STATE_UNKNOWN = 0,
    UIOX_DEV_TPWD_STATE_IDLE,
    UIOX_DEV_TPWD_STATE_ENROLLING,
    UIOX_DEV_TPWD_STATE_VERIFYING,
    UIOX_DEV_TPWD_STATE_LOCKED,
    UIOX_DEV_TPWD_STATE_AUTHENTICATED
} uiox_dev_tpwd_state_t;

typedef struct {
    uiox_dev_handle_t    h;
    uiox_dev_tpwd_state_t auth_state;   /* from the events seen */
} uiox_dev_tpwd_t;

uiox_dev_err_t uiox_dev_tpwd_open (uiox_dev_tpwd_t *t, int unit, int flags);
uiox_dev_err_t uiox_dev_tpwd_close(uiox_dev_tpwd_t *t);

/* Blocks until a raw touch event is ready, backed by ev.pending. */
uiox_dev_err_t uiox_dev_tpwd_wait (uiox_dev_tpwd_t *t, int timeout_ms);

/* Pops ONE raw event.  A short read is a fault — the driver's
 * event_read never returns 0 — so it is reported as UIOX_DEV_EIO. */
uiox_dev_err_t uiox_dev_tpwd_read (uiox_dev_tpwd_t *t,
                                   uiox_dev_tpwd_raw_evt_t *out);

/* The auth state inferred from the events seen so far.  Note that the
 * raw record carries a chip gesture_id, not a subsystem event — the
 * AUTH_* transitions live above in the subsystem, so this is only
 * updated if the driver surfaces them through the same read. */
uiox_dev_tpwd_state_t uiox_dev_tpwd_state(const uiox_dev_tpwd_t *t);

#endif /* UIOX_DEV_TPWD_H */
