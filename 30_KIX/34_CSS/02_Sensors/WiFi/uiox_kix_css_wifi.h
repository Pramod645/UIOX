/*
 * 34_CSS/02_Sensors/WiFi/uiox_kix_css_wifi.h
 *
 * WiFi's syscall entry points — the functions the CSS dispatch table in
 * 10_sccss/src/uiox_kix_css_dispatch.c takes the address of.
 *
 * ── THE NUMBERS ARE CSS-PRIVATE ─────────────────────────────────────
 * 802.11 has no BSD syscall vocabulary, so these four sit on numbers
 * this kernel leaves unimplemented and marks as its own:
 *
 *   400  SYS_WIFI_SCAN        css-private
 *   401  SYS_WIFI_DISCONNECT  css-private
 *   402  SYS_WIFI_QUALITY     css-private
 *   403  SYS_WIFI_STATE       css-private
 *
 * See 10_sccss/include/uiox_kix_sccss_numbers.h for the allocation and
 * the reason the block was placed at 400.
 *
 * ── ONE INSTANCE, NOT A UNIT INDEX ──────────────────────────────────
 * uiox_wifi_probe() registers one device — s_wifi_dev, with
 * .cls = UIOX_DEVCLASS_WIFI = UIOX_DEVCLASS_(STREAM, 1).  There is no
 * multi-radio model in this tree and no table to index, so these entry
 * points use the one subsystem instance the probe bound and ignore any
 * slot a caller supplies.  A unit argument would imply an array that
 * does not exist; when one appears, it arrives here first.
 *
 * ── WHAT IS NOT HERE ────────────────────────────────────────────────
 *   connect(ssid, passphrase)   a credential crossing the trap is a
 *                               design decision.  No number, no row,
 *                               no entry point until it is made.
 *   bss_list(count_out)         returns a pointer into proto->bss_cache
 *                               — kernel memory.  It needs to become a
 *                               copy-into-caller-buffer call, which
 *                               means a size argument its signature
 *                               does not have.  Not here either.
 *
 * @version 1.0.0  @date 2026-11-21
 */
#ifndef UIOX_KIX_CSS_WIFI_H
#define UIOX_KIX_CSS_WIFI_H

#include "uiox_kix_sccss.h"      /* SCCSS_E* codes, the gate's shape */
#include "uiox_wifi_subsys.h"    /* uiox_wifi_subsys_t, quality, state */

/* The one instance the probe bound.  Declared so the entry points can
 * reach it without each resolving the question for itself — the same
 * role uiox_kix_scpcs_current() plays for the process side. */
uiox_wifi_subsys_t *uiox_kix_css_wifi_current(void);

/* ── The four entry points, six-slot shape ─────────────────────────── */

/* scan(timeout_ms) -> 0, or negative errno.
 * Blocks until the scan completes or the timeout expires. */
int64_t uiox_kix_css_wifi_scan      (uiox_uint64_t timeout_ms);

/* disconnect(void) -> 0, or negative errno. */
int64_t uiox_kix_css_wifi_disconnect(void);

/* quality(out) -> 0, or negative errno.
 * out is MANDATORY and WRITTEN THROUGH for sizeof(uiox_wifi_quality_t). */
int64_t uiox_kix_css_wifi_quality   (uiox_uintptr_t out);

/* state(void) -> the uiox_wifi_subsys_state_t value, or negative errno. */
int64_t uiox_kix_css_wifi_state     (void);

#endif /* UIOX_KIX_CSS_WIFI_H */
