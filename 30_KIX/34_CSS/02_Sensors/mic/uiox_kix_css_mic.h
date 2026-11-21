/*
 * 34_CSS/02_Sensors/mic/uiox_kix_css_mic.h
 *
 * MIC's syscall entry points — the functions the CSS dispatch table
 * takes the address of.
 *
 * ── numbers 409-412, CSS-PRIVATE ────────────────────────────────────
 * No BSD syscall vocabulary exists for a microphone.  Same block, same
 * mark, same place in the range this kernel leaves unimplemented.
 *
 * ── read() is the first row in this arm that WRITES EL0 memory ──────
 * wifi_quality filled 24 bytes of scalars; this fills arbitrary PCM.
 * The gate, and the bound BEFORE the gate, are what keep a caller from
 * naming an address it does not own or a length that overflows the
 * arithmetic the check itself performs.
 *
 * ── one instance ────────────────────────────────────────────────────
 * uiox_mic_probe() registers one microphone; there is no array to
 * index, so the slots are ignored.
 */
#ifndef UIOX_KIX_CSS_MIC_H
#define UIOX_KIX_CSS_MIC_H

#include "uiox_kix_sccss.h"
#include "uiox_mic_subsys.h"

uiox_mic_subsys_t *uiox_mic_subsys_current(void);

int64_t uiox_kix_css_mic_read (uiox_uintptr_t buf, uiox_uint64_t n_samples);
int64_t uiox_kix_css_mic_gain (uiox_uint64_t gain_db);
int64_t uiox_kix_css_mic_mute (uiox_uint64_t on);
int64_t uiox_kix_css_mic_state(void);

#endif
