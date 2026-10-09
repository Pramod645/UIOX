/*
 * @file    uiox_kbd_if.c
 * @brief   UIOX Keyboard interface driver implementation.
 *
 * v1.1.0:
 *   · uiox_kbd_if_config() now initialises the owned ring (kif->rx)
 *   · uiox_kbd_if_scan() falls back to kif->rx when rb is NULL
 *   · scan_i2c() and scan_ps2() read hw->ops, not hw->priv — see below
 */
#include "uiox_kbd_if.h"

int uiox_kbd_if_config(uiox_kbd_if_t     *kif,
                        uiox_kbd_hw_t     *hw,
                        uiox_kbd_if_type_t type)
{
    if (!kif || !hw) return -EINVAL;
    memset(kif, 0, sizeof(*kif));
    kif->hw     = hw;
    kif->type   = type;

    /* The ring read() drains.  31_drvbuff owns the implementation; this
     * just gives the driver one. */
    uiox_kbd_buf_init(&kif->rx);

    kif->primed = true;
    return 0;
}

/* -------------------------------------------------------------------------
 * Ghost-key filter: a key press is considered valid only if it is the
 * sole column change in its row, OR if the same column also changed in
 * exactly one other row (N-key rollover detection).
 * For simplicity this implementation accepts all simultaneous presses —
 * extend with KRO detection for production.
 * ---------------------------------------------------------------------- */

static void scan_matrix(uiox_kbd_if_t      *kif,
                         uiox_kbd_ringbuf_t *rb,
                         uint64_t            ts_ns)
{
    uiox_kbd_hw_t *hw = kif->hw;
    uint8_t rows = hw->matrix.num_rows;
    uint8_t cols = hw->matrix.num_cols;

    /* Save previous state */
    memcpy(kif->matrix.prev_state, kif->matrix.row_state,
           sizeof(kif->matrix.prev_state));

    /* Scan each row */
    for (uint8_t r = 0; r < rows; r++) {
        uint16_t cols_raw = 0;
        uiox_kbd_hw_scan_row(hw, r, &cols_raw);

        /* Active-low polarity inversion */
        if (hw->matrix.active_low)
            cols_raw = (uint16_t)(~cols_raw) & ((1u << cols) - 1u);

        kif->matrix.row_state[r] = cols_raw;
    }

    /* Detect changes and push events */
    for (uint8_t r = 0; r < rows; r++) {
        uint16_t changed = kif->matrix.row_state[r] ^
                           kif->matrix.prev_state[r];
        if (!changed) continue;

        for (uint8_t c = 0; c < cols; c++) {
            if (!(changed & (1u << c))) continue;

            bool pressed = (kif->matrix.row_state[r] >> c) & 1u;
            uiox_kbd_event_t ev = {
                .ev_type   = pressed ? UIOX_KBD_EV_PRESS
                                     : UIOX_KBD_EV_RELEASE,
                .row       = r,
                .col       = c,
                .scancode  = (uint8_t)((r << 4u) | c),
                .keycode   = 0,       /* filled by event layer */
                .unicode   = 0,
                .modifiers = 0,
                .ts_ns     = ts_ns,
            };
            uiox_kbd_buf_push(rb, &ev);
            kif->change_count++;
        }
    }
}

static void scan_direct(uiox_kbd_if_t      *kif,
                          uiox_kbd_ringbuf_t *rb,
                          uint64_t            ts_ns)
{
    uint32_t state = 0;
    uiox_kbd_hw_read_direct(kif->hw, &state);

    if (kif->hw->direct_active_low)
        state = ~state & ((1u << kif->hw->num_direct) - 1u);

    kif->direct_prev  = kif->direct_state;
    kif->direct_state = state;

    uint32_t changed = state ^ kif->direct_prev;
    if (!changed) return;

    for (uint8_t i = 0; i < kif->hw->num_direct; i++) {
        if (!(changed & (1u << i))) continue;
        bool pressed = (state >> i) & 1u;
        uiox_kbd_event_t ev = {
            .ev_type  = pressed ? UIOX_KBD_EV_PRESS : UIOX_KBD_EV_RELEASE,
            .row      = 0xFFu,
            .col      = i,
            .scancode = (uint8_t)(0x80u | i),
            .ts_ns    = ts_ns,
        };
        uiox_kbd_buf_push(rb, &ev);
        kif->change_count++;
    }
}

static void scan_i2c(uiox_kbd_if_t      *kif,
                      uiox_kbd_ringbuf_t *rb,
                      uint64_t            ts_ns)
{
    /* hw->ops, not hw->priv.  uiox_kbd_hw_t declares `ops` and `drv_priv`;
     * there is no `priv` member, and uiox_kbd_hw_init() writes hw->ops. */
    const uiox_kbd_hw_ops_t *ops =
        (const uiox_kbd_hw_ops_t *)kif->hw->ops;
    if (!ops || !ops->i2c_read) return;

    /* TCA8418: read key event register 0x04 */
    uint8_t key_ev = 0;
    if (ops->i2c_read(kif->hw, 0x04u, &key_ev) < 0) return;
    if (!key_ev) return;

    bool pressed  = (key_ev & 0x80u) != 0u;
    uint8_t code  = key_ev & 0x7Fu;
    uint8_t row   = (uint8_t)((code - 1u) / 10u);
    uint8_t col   = (uint8_t)((code - 1u) % 10u);

    uiox_kbd_event_t ev = {
        .ev_type  = pressed ? UIOX_KBD_EV_PRESS : UIOX_KBD_EV_RELEASE,
        .row      = row,
        .col      = col,
        .scancode = code,
        .ts_ns    = ts_ns,
    };
    uiox_kbd_buf_push(rb, &ev);
    kif->change_count++;
}

static void scan_ps2(uiox_kbd_if_t      *kif,
                      uiox_kbd_ringbuf_t *rb,
                      uint64_t            ts_ns)
{
    /* Same correction as scan_i2c — hw->ops, not hw->priv. */
    const uiox_kbd_hw_ops_t *ops =
        (const uiox_kbd_hw_ops_t *)kif->hw->ops;
    if (!ops || !ops->ps2_recv) return;

    uint8_t sc = 0;
    if (ops->ps2_recv(kif->hw, &sc, 1u) < 0) return;

    /* PS/2 set-2: 0xF0 prefix = break (release) */
    static bool s_break = false;
    if (sc == 0xF0u) { s_break = true; return; }

    bool pressed = !s_break;
    s_break = false;

    uiox_kbd_event_t ev = {
        .ev_type  = pressed ? UIOX_KBD_EV_PRESS : UIOX_KBD_EV_RELEASE,
        .row      = 0xFFu,
        .col      = 0xFFu,
        .scancode = sc,
        .ts_ns    = ts_ns,
    };
    uiox_kbd_buf_push(rb, &ev);
    kif->change_count++;
}

int uiox_kbd_if_scan(uiox_kbd_if_t      *kif,
                      uiox_kbd_ringbuf_t *rb,
                      uint64_t            ts_ns)
{
    if (!kif || !kif->primed) return -EINVAL;

    /* NULL means "the ring this driver owns" — the one read() drains. */
    if (!rb) rb = &kif->rx;

    kif->scan_count++;

    switch (kif->type) {
    case UIOX_KBD_IF_MATRIX:  scan_matrix(kif, rb, ts_ns); break;
    case UIOX_KBD_IF_GPIO:    scan_direct(kif, rb, ts_ns); break;
    case UIOX_KBD_IF_I2C:     scan_i2c   (kif, rb, ts_ns); break;
    case UIOX_KBD_IF_PS2:     scan_ps2   (kif, rb, ts_ns); break;
    default: break;
    }

    return (int)kif->change_count;
}

bool uiox_kbd_if_key_pressed(const uiox_kbd_if_t *kif,
                               uint8_t row, uint8_t col)
{
    if (!kif || row >= UIOX_KBD_MAX_ROWS || col >= UIOX_KBD_MAX_COLS)
        return false;
    return (kif->matrix.row_state[row] >> col) & 1u;
}

bool uiox_kbd_if_direct_pressed(const uiox_kbd_if_t *kif, uint8_t idx)
{
    if (!kif || idx >= 32u) return false;
    return (kif->direct_state >> idx) & 1u;
}

/* ═════════════════════════════════════════════════════════════════════
 * Event path — the consumer the ring never had
 *
 * uiox_kbd_buf_pop() was defined in 31_drvbuff and called by nobody.  These
 * are its callers, reached from 32_FS's read branch for a char inode whose
 * idev_class resolves to KBD.
 * ═════════════════════════════════════════════════════════════════════ */

bool uiox_kbd_event_pending(uiox_dev_dev_t *dev)
{
    if (!dev) return false;
    uiox_kbd_if_t *kif = (uiox_kbd_if_t *)dev->drv_priv;
    if (!kif) return false;
    return !uiox_kbd_buf_empty(&kif->rx);
}

int uiox_kbd_event_read(uiox_dev_dev_t *dev, void *rec, uint16_t maxlen)
{
    if (!dev || !rec) return -EINVAL;

    /* A short buffer is the caller's error, not an empty queue. */
    if (maxlen < (uint16_t)sizeof(uiox_kbd_event_t)) return -EINVAL;

    uiox_kbd_if_t *kif = (uiox_kbd_if_t *)dev->drv_priv;
    if (!kif) return -EINVAL;

    uiox_kbd_event_t ev;
    if (!uiox_kbd_buf_pop(&kif->rx, &ev))
        return -EAGAIN;   /* never 0 — see the header note */

    memcpy(rec, &ev, sizeof(ev));
    return (int)sizeof(ev);
}
