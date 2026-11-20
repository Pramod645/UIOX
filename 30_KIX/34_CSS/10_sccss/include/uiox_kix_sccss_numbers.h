/* ═══════════════════════════════════════════════════════════════════
 *  30_KIX/34_CSS/10_sccss/include/uiox_kix_sccss_numbers.h
 *
 *  SCCSS — the BSD syscall numbers THIS layer's dispatch table routes,
 *  so uiox_kix_css_dispatch.c can index by them without including the
 *  syscall layer's header.
 *
 *  ── why they live here and not in 40_SCIX ─────────────────────────
 *  Same reason as 10_scfs's and 50_scpcs's copies: 40_SCIX is the
 *  syscall boundary ABOVE this layer, and a kernel layer including its
 *  header inverts the direction of calls.  The numbers are constants —
 *  they belong below, beside the table that indexes them.
 *
 *  ── ONE FILE, ROWS PER CLASS ──────────────────────────────────────
 *  This is the GENERAL numbers file for the CSS arm, not eth's.  Rows
 *  are grouped by the module that owns them and added as each class
 *  lands:
 *
 *    01_Com/eth            the socket group below
 *    01_Com/bt             rows when that class has a call vocabulary
 *    01_Com/usb            likewise
 *    02_Sensors/camera, mic · 03_NonSensors/speaker   likewise
 *
 *  A class's numbers belong beside the arm's other numbers — not in the
 *  class's own directory — because the dispatch table indexes ONE array
 *  spanning the whole arm, and a number it cannot see is a number it
 *  cannot route.
 *
 *  ── which block of uix_sys.h these come from ──────────────────────
 *  40_SCIX/uix_sys.h defines several of these names TWICE with
 *  DIFFERENT values, and in C the later #define wins silently.  The
 *  UPPER/canonical run is the one copied here — the same block
 *  uiox_kix_scfs_numbers.h chose, on the same evidence.
 *
 *  The collision that matters for this file:
 *
 *      SYS_SHUTDOWN   134 (canonical)  vs  293 (fallback)
 *
 *  293 is the socket teardown's number ONLY in the fallback block, where
 *  it also collides with PIPE2.  The socket call is 134.  Every define
 *  below is #ifndef-guarded so a legitimately-in-scope uix_sys.h wins
 *  the test and these fall away.
 *
 *  ── a wrong number does not fail to compile ───────────────────────
 *  It routes a syscall to another handler, silently.  This file's only
 *  job is to make that impossible for the CSS arm.
 *
 *  @version 1.0.0  @date 2026-11-20
 * ═══════════════════════════════════════════════════════════════════ */

 #ifndef UIOX_KIX_SCCCS_NUMBERS_H
 #define UIOX_KIX_SCCCS_NUMBERS_H
 
 /* ══ 01_Com/eth — socket calls ══════════════════════════════════════ */
 
 /* ── table (connection) ───────────────────────────────────────────── */
 #ifndef SYS_SOCKET
 #define SYS_SOCKET           97
 #endif
 #ifndef SYS_BIND
 #define SYS_BIND            104
 #endif
 #ifndef SYS_CONNECT
 #define SYS_CONNECT          98
 #endif
 #ifndef SYS_LISTEN
 #define SYS_LISTEN          106
 #endif
 #ifndef SYS_ACCEPT
 #define SYS_ACCEPT           30
 #endif
 
 /* ── table (datagram) ─────────────────────────────────────────────── */
 #ifndef SYS_SENDTO
 #define SYS_SENDTO          133
 #endif
 #ifndef SYS_RECVFROM
 #define SYS_RECVFROM         29
 #endif
 
 /* ── table (message) ──────────────────────────────────────────────── */
 #ifndef SYS_SENDMSG
 #define SYS_SENDMSG          28
 #endif
 #ifndef SYS_RECVMSG
 #define SYS_RECVMSG          27
 #endif
 
 /* ── table (teardown) ─────────────────────────────────────────────── */
 #ifndef SYS_SHUTDOWN
 #define SYS_SHUTDOWN        134
 #endif
 #ifndef SYS_SOCKETPAIR
 #define SYS_SOCKETPAIR      135
 #endif
 
 /* ══ 01_Com/bt — no rows yet ════════════════════════════════════════
  * The kernel has no command vocabulary for Bluetooth: the ACL/HCI path
  * is reached through uiox_bt_if inside 34_CSS, not by a syscall number.
  * Rows land here if and when that changes — and the class's entry
  * points go in 01_Com/bt/, not in this file. */
 
 /* ══ 01_Com/usb — no rows yet ═══════════════════════════════════════
  * Same position: uiox_usb_class.c registers USB device classes with the
  * core rather than exposing numbered calls. */
 
 /* ══ 02_Sensors, 03_NonSensors — no rows yet ════════════════════════
  * camera, mic, speaker, touchpwd: each has a driver and a userspace
  * layer, none has a syscall vocabulary.  Rows are added per class as
  * each one gets one. */
 
 #endif /* UIOX_KIX_SCCCS_NUMBERS_H */
 