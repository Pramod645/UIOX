/*
 * 34_CSS/03_NonSensors/speaker/uiox_kix_css_spk.h
 *
 * SPK's syscall entry points — numbers 413-416, CSS-PRIVATE.
 * See uiox_kix_sccss_numbers.h for the allocation.
 *
 * ── write() sends an EL0 buffer INTO the kernel ─────────────────────
 * mic_read copied out; this copies in.  An unchecked write buffer is a
 * kernel READ primitive: the caller points the kernel at memory it does
 * not own and the kernel transmits whatever is there.
 *
 * n_stereo counts stereo PAIRS, so the span is n_stereo * 4 bytes —
 * two int16_t per pair.  Reading it as n_stereo * 2 would validate half
 * the buffer uiox_spk_if_write actually reads.
 *
 * ── WHAT IS NOT HERE, and why ───────────────────────────────────────
 *   add_stream(data, n_stereo, gain, loop)
 *       `loop=true` leaves the kernel holding an EL0 pointer PAST THE
 *       RETURN.  A buffer that passed the gate can be unmapped or
 *       reused the moment the call ends, and the trap shape cannot
 *       express "this must stay valid until you stop stream N".
 *   stop_stream(id)
 *       unreachable — no trap can have created a stream to stop.
 *   set_eq(band, hz, gain_db, q)
 *       three floats; the trap has no float convention and int64_t
 *       does not carry them.
 *
 * ── one instance ────────────────────────────────────────────────────
 * Slots are ignored: there is no multi-codec model and no array.
 */
#ifndef UIOX_KIX_CSS_SPK_H
#define UIOX_KIX_CSS_SPK_H

#include "uiox_kix_sccss.h"
#include "uiox_spk_subsys.h"

uiox_spk_subsys_t *uiox_spk_subsys_current(void);

int64_t uiox_kix_css_spk_write (uiox_uintptr_t samples, uiox_uint64_t n_stereo);
int64_t uiox_kix_css_spk_volume(uiox_uint64_t vol_pct);
int64_t uiox_kix_css_spk_mute  (uiox_uint64_t on);
int64_t uiox_kix_css_spk_state (void);

#endif /* UIOX_KIX_CSS_SPK_H */
