/*
 * @file    uiox_mouse_if.c
 * @brief   UIOX Mouse interface driver implementation.
 *
 * v1.1.0:
 *   · uiox_mouse_if_config() now initialises the owned ring (mif->rx)
 *   · uiox_mouse_if_poll() falls back to mif->rx when dst_rb is NULL
 *   · the two event entry points appended
 */
#include "uiox_mouse_if.h"
#include "uiox_klibc.h"

int uiox_mouse_if_config(uiox_mouse_if_t *mif, uiox_mouse_hw_t *hw)
{
    if (!mif || !hw) return -EINVAL;
    memset(mif, 0, sizeof(*mif));
    mif->hw     = hw;

    /* The ring read() drains.  31_drvbuff owns the implementation. */
    uiox_mouse_buf_init(&mif->rx);

    mif->primed = true;
    return 0;
}

int uiox_mouse_if_poll(uiox_mouse_if_t      *mif,
                        uiox_mouse_ringbuf_t *dst_rb)
{
    if (!mif || !mif->primed) return -EINVAL;

    /* NULL means "the ring this driver owns" — the one read() drains. */
    if (!dst_rb) dst_rb = &mif->rx;

    mif->stats.poll_count++;

    bool connected = uiox_mouse_hw_connected(mif->hw);
    if (connected != mif->prev_connected) {
        mif->prev_connected = connected;
        uiox_mouse_event_t ev;
        memset(&ev, 0, sizeof(ev));
        ev.type = connected ? UIOX_MOUSE_EV_CONNECT
                            : UIOX_MOUSE_EV_DISCONNECT;
        if (uiox_mouse_buf_push(dst_rb, &ev))
            connected ? mif->stats.connect_events++
                      : mif->stats.disconnect_events++;
        else
            mif->stats.reports_dropped++;
    }

    if (!connected) return 0;

    uiox_mouse_raw_t raw;
    memset(&raw, 0, sizeof(raw));
    int r = uiox_mouse_hw_read_report(mif->hw, &raw);
    if (r <= 0) return r;

    mif->stats.reports_received++;
    int pushed = 0;

    if (raw.dx || raw.dy) {
        uiox_mouse_event_t ev;
        memset(&ev, 0, sizeof(ev));
        ev.type    = UIOX_MOUSE_EV_MOVE;
        ev.dx      = raw.dx;
        ev.dy      = raw.dy;
        ev.buttons = raw.buttons;
        ev.ts_ns   = raw.ts_ns;
        if (uiox_mouse_buf_push(dst_rb, &ev)) pushed++;
        else mif->stats.reports_dropped++;
    }

    uint8_t changed = raw.buttons ^ mif->prev_buttons;
    uint8_t b;
    for (b = 0; b < UIOX_MOUSE_MAX_BUTTONS; b++) {
        if (!(changed & (1u << b))) continue;
        bool pressed = (raw.buttons >> b) & 1u;
        uiox_mouse_event_t ev;
        memset(&ev, 0, sizeof(ev));
        ev.type    = pressed ? UIOX_MOUSE_EV_BTN_PRESS
                             : UIOX_MOUSE_EV_BTN_RELEASE;
        ev.button  = b;
        ev.buttons = raw.buttons;
        ev.ts_ns   = raw.ts_ns;
        if (uiox_mouse_buf_push(dst_rb, &ev)) pushed++;
        else mif->stats.reports_dropped++;
    }
    mif->prev_buttons = raw.buttons;

    if (raw.dz) {
        uiox_mouse_event_t ev;
        memset(&ev, 0, sizeof(ev));
        ev.type    = UIOX_MOUSE_EV_SCROLL_V;
        ev.dz      = raw.dz;
        ev.buttons = raw.buttons;
        ev.ts_ns   = raw.ts_ns;
        if (uiox_mouse_buf_push(dst_rb, &ev)) pushed++;
    }

    if (raw.dw) {
        uiox_mouse_event_t ev;
        memset(&ev, 0, sizeof(ev));
        ev.type    = UIOX_MOUSE_EV_SCROLL_H;
        ev.dw      = raw.dw;
        ev.buttons = raw.buttons;
        ev.ts_ns   = raw.ts_ns;
        if (uiox_mouse_buf_push(dst_rb, &ev)) pushed++;
    }

    return pushed;
}

void uiox_mouse_if_stats_get(const uiox_mouse_if_t *mif,
                               uiox_mouse_if_stats_t *out)
{
    if (!mif || !out) return;
    memcpy(out, &mif->stats, sizeof(*out));
}

void uiox_mouse_if_stats_reset(uiox_mouse_if_t *mif)
{
    if (!mif) return;
    memset(&mif->stats, 0, sizeof(mif->stats));
}

/* ═════════════════════════════════════════════════════════════════════
 * Event path — the consumer the ring never had
 * ═════════════════════════════════════════════════════════════════════ */

bool uiox_mouse_event_pending(uiox_dev_dev_t *dev)
{
    if (!dev) return false;
    uiox_mouse_if_t *mif = (uiox_mouse_if_t *)dev->drv_priv;
    if (!mif) return false;
    return !uiox_mouse_buf_empty(&mif->rx);
}

int uiox_mouse_event_read(uiox_dev_dev_t *dev, void *rec, uint16_t maxlen)
{
    if (!dev || !rec) return -EINVAL;
    if (maxlen < (uint16_t)sizeof(uiox_mouse_event_t)) return -EINVAL;

    uiox_mouse_if_t *mif = (uiox_mouse_if_t *)dev->drv_priv;
    if (!mif) return -EINVAL;

    uiox_mouse_event_t ev;
    if (!uiox_mouse_buf_pop(&mif->rx, &ev))
        return -EAGAIN;   /* never 0 */

    memcpy(rec, &ev, sizeof(ev));
    return (int)sizeof(ev);
}
