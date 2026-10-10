/*
 * @file    uiox_mouse_if.h
 * @brief   UIOX Mouse interface driver.
 *
 * v1.1.0: gains an OWNED ring (rx) and the two event entry points read()
 *         reaches.  No change to 31_drvbuff — uiox_mouse_buf.{h,c} already
 *         had init/push/pop/empty/count.
 */
#ifndef UIOX_MOUSE_IF_H
#define UIOX_MOUSE_IF_H

#include "uiox_mouse_hw.h"
#include "uiox_mouse_buf.h"
#include "uiox_devclass.h"   /* uiox_dev_dev_t — device registry descriptor */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint64_t poll_count;
    uint64_t reports_received;
    uint64_t reports_dropped;
    uint64_t connect_events;
    uint64_t disconnect_events;
} uiox_mouse_if_stats_t;

typedef struct {
    uiox_mouse_hw_t       *hw;
    bool                   primed;
    bool                   prev_connected;
    uint8_t                prev_buttons;
    uiox_mouse_if_stats_t  stats;

    /* ── the owned ring ──────────────────────────────────────────────────
     * uiox_mouse_if_poll(mif, dst_rb) takes the ring as a PARAMETER, the
     * same as the keyboard's scan.  read() cannot supply one, so the
     * driver owns it: poll() pushes into it when its argument is NULL, and
     * uiox_mouse_event_read() pops from it.
     * ───────────────────────────────────────────────────────────────── */
    uiox_mouse_ringbuf_t   rx;
} uiox_mouse_if_t;

int  uiox_mouse_if_config    (uiox_mouse_if_t *mif, uiox_mouse_hw_t *hw);

/**
 * @param dst_rb  Ring to push into.  NULL = mif->rx (the owned one).
 */
int  uiox_mouse_if_poll      (uiox_mouse_if_t *mif,
                               uiox_mouse_ringbuf_t *dst_rb);
void uiox_mouse_if_stats_get (const uiox_mouse_if_t *mif,
                               uiox_mouse_if_stats_t *out);
void uiox_mouse_if_stats_reset(uiox_mouse_if_t *mif);

/* =========================================================================
 * Event path — reached from read() on /dev/mouse<N>
 *
 * The family-3 half of uiox_event_ops_t, which 32_FS finds at
 * sizeof(uiox_dev_ops_t) inside the class's ops struct.  event_read returns
 * bytes written or a negative errno — NEVER 0.
 * ====================================================================== */

bool uiox_mouse_event_pending(uiox_dev_dev_t *dev);
int  uiox_mouse_event_read   (uiox_dev_dev_t *dev, void *rec, uint16_t maxlen);

#ifdef __cplusplus
}
#endif
#endif /* UIOX_MOUSE_IF_H */
