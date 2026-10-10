/**
 * @file    uiox_kbd_if.h
 * @brief   UIOX Keyboard interface driver.
 *
 * Sits between HAL and event layer. Manages:
 *   - Full matrix scan (all rows, ghost-key filtering)
 *   - Direct GPIO polling
 *   - I2C keyboard controller polling (TCA8418, PCF8574)
 *   - PS/2 scancode reception
 *   - Raw scancode → position mapping
 *   - Key state tracking (pressed/released bitmaps)
 *
 * @date    2026-05-27
 */
//Layer 2 — Interface Driver
 /*
 * @file    uiox_kbd_if.h
 * @brief   UIOX Keyboard interface driver.
 *
 * v1.1.0: gains an OWNED ring (rx) and the two event entry points read()
 *         reaches.  No change to 31_drvbuff — uiox_kbd_buf.{h,c} already
 *         had init/push/pop/empty/count; the ring simply had no owner and
 *         no consumer.
 */
//Layer 2 — Interface Driver
#ifndef UIOX_KBD_IF_H
#define UIOX_KBD_IF_H

#include "uiox_kbd_hw.h"
#include "uiox_kbd_buf.h"
#include "uiox_devclass.h"   /* uiox_dev_dev_t — device registry descriptor */
#include "uiox_klibc.h"

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * Matrix state snapshot
 * ====================================================================== */

typedef struct {
    uint16_t  row_state[UIOX_KBD_MAX_ROWS]; /**< Current col bitmask/row  */
    uint16_t  prev_state[UIOX_KBD_MAX_ROWS];/**< Previous col bitmask/row */
} uiox_kbd_matrix_state_t;

/* =========================================================================
 * Interface descriptor
 * ====================================================================== */

typedef struct {
    uiox_kbd_hw_t           *hw;
    uiox_kbd_if_type_t       type;
    uiox_kbd_matrix_state_t  matrix;
    uint32_t                 direct_state;      /**< Current direct key bits */
    uint32_t                 direct_prev;       /**< Previous direct key bits*/
    uint32_t                 scan_count;        /**< Total scan cycles       */
    uint32_t                 change_count;      /**< Total state changes     */
    uint64_t                 events_dropped;    /**< Pushes refused by a full ring */

    /* ── the owned ring ──────────────────────────────────────────────────
     * uiox_kbd_if_scan(kif, rb, ts_ns) takes the ring as a PARAMETER —
     * fine for an in-kernel consumer that holds one, useless for read(),
     * which has no ring to hand over.  rx is that owner: the scan pushes
     * into it when its argument is NULL, and uiox_kbd_event_read() pops
     * from it.
     * ───────────────────────────────────────────────────────────────── */
    uiox_kbd_ringbuf_t       rx;

    bool                     primed;
} uiox_kbd_if_t;

/* =========================================================================
 * Interface API
 * ====================================================================== */

int  uiox_kbd_if_config (uiox_kbd_if_t    *kif,
                          uiox_kbd_hw_t    *hw,
                          uiox_kbd_if_type_t type);

/**
 * @brief  Perform one full scan cycle.
 *
 * @param  kif   Interface descriptor.
 * @param  rb    Ring buffer to push into.  NULL = kif->rx (the owned one).
 * @param  ts_ns Current timestamp in nanoseconds.
 * @return Number of key state changes detected.
 */
int  uiox_kbd_if_scan   (uiox_kbd_if_t      *kif,
                          uiox_kbd_ringbuf_t *rb,
                          uint64_t            ts_ns);

/** Query whether a specific matrix key is currently pressed. */
bool uiox_kbd_if_key_pressed(const uiox_kbd_if_t *kif,
                              uint8_t row, uint8_t col);

/** Query whether a direct GPIO key is pressed. */
bool uiox_kbd_if_direct_pressed(const uiox_kbd_if_t *kif, uint8_t idx);

/* =========================================================================
 * Event path — reached from read() on /dev/kbd<N>
 *
 * These two are the family-3 half of uiox_event_ops_t, which 32_FS finds
 * at sizeof(uiox_dev_ops_t) inside the class's ops struct (the head comes
 * first — uiox_devclass.h §6).  ioctl carries commands the other way and
 * never touches the ring.
 *
 * event_read returns bytes written, or a negative errno — NEVER 0.  An
 * empty queue is what event_pending() is for, and conflating the two makes
 * a spurious read indistinguishable from a real one.
 * ====================================================================== */

bool uiox_kbd_event_pending(uiox_dev_dev_t *dev);
int  uiox_kbd_event_read   (uiox_dev_dev_t *dev, void *rec, uint16_t maxlen);

#ifdef __cplusplus
}
#endif
#endif /* UIOX_KBD_IF_H */
