/*
 * 34_CSS/01_Com/bt/uiox_kix_css_bt.h
 *
 * BT's syscall entry points — the functions the CSS dispatch table in
 * 10_sccss/src/uiox_kix_css_dispatch.c takes the address of.
 *
 * ── THE NUMBERS ARE CSS-PRIVATE ─────────────────────────────────────
 * Bluetooth has no BSD syscall vocabulary either, so these sit on the
 * next free numbers in the block this kernel marks as its own:
 *
 *   405  SYS_BT_SCAN_START   css-private
 *   406  SYS_BT_SCAN_STOP    css-private
 *   407  SYS_BT_DISCONNECT   css-private
 *   408  SYS_BT_STATE        css-private
 *
 * ── ONE INSTANCE, NOT A UNIT INDEX ──────────────────────────────────
 * uiox_bt_probe() registers one adapter — s_bt_dev.  There is no
 * multi-adapter model and no table to index, so these entry points use
 * the one subsystem instance and ignore any slot a caller supplies.
 *
 * ── WHAT IS NOT HERE ────────────────────────────────────────────────
 *   connect(addr, is_ble)   uiox_bt_addr_t is a STRUCT — six register
 *                           slots cannot carry it without unpacking,
 *                           and that unpacking is a design decision.
 *   hci_cmd(...)            SEVEN arguments.  The trap carries six.
 *                           It cannot be a row at all.
 *   acl_send / gatt_write   byte buffers both ways; each needs a gate
 *                           and a length bound before it can be one.
 *
 * @version 1.0.0  @date 2026-11-21
 */
#ifndef UIOX_KIX_CSS_BT_H
#define UIOX_KIX_CSS_BT_H

#include "uiox_kix_sccss.h"      /* SCCSS_E* codes */
#include "uiox_bt_subsys.h"      /* uiox_bt_subsys_t, the state enum */

int64_t uiox_kix_css_bt_scan_start(uiox_uint64_t dur_s);
int64_t uiox_kix_css_bt_scan_stop (void);
int64_t uiox_kix_css_bt_disconnect(uiox_uint64_t handle);
int64_t uiox_kix_css_bt_state     (void);

#endif /* UIOX_KIX_CSS_BT_H */
