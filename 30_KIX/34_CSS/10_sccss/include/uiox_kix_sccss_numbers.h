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
 /* ══ 02_Sensors/WiFi — CSS-PRIVATE numbers ══════════════════════════
 * ── THESE ARE BORROWED, NOT BSD'S ───────────────────────────────────
 * Every number above this block is a genuine BSD syscall number that
 * this tree routes.  These are not.  802.11 has no BSD syscall
 * vocabulary — there is no SYS_WIFI_SCAN in any BSD — so the block
 * below is a CSS-PRIVATE allocation taken from the range this kernel
 * leaves unimplemented.
 *
 *   400–511   unused: verified against every header in the tree
 *
 * ── WHY 400 AND NOT A LOW NUMBER ────────────────────────────────────
 * A low free slot would sit among real BSD numbers and read as one —
 * which is the failure the file's own banner warns about: "a wrong
 * number does not fail to compile; it routes a syscall to another
 * handler, silently."  A block at 400, far above anything BSD defines
 * for these families, cannot be confused with a borrowed BSD number by
 * a reader who checks.
 *
 * ── THE MARK IS LOAD-BEARING ────────────────────────────────────────
 * SCCSS_PRIVATE marks them so a future audit can find every one.  If
 * BSD ever allocates 400–511, these must move — and the grep for the
 * marker is how that migration starts.
 *
 * ── WHAT IS NOT HERE ────────────────────────────────────────────────
 *   wifi_connect    carries a passphrase.  Whether a credential may
 *                   cross the trap from EL0 is a design decision, and
 *                   no number is allocated until it is made.
 *   wifi_bss_list   returns a pointer into kernel memory (proto's
 *                   bss_cache).  It becomes a copy-into-caller call or
 *                   it gets no number.
 */
#ifndef SCCSS_PRIVATE
#define SCCSS_PRIVATE   400u    /* first CSS-private syscall number */
#endif

/* the block — one number per call, 400 upward */
#ifndef SYS_WIFI_SCAN
#define SYS_WIFI_SCAN        400u   /* css-private — 802.11 scan      */
#endif
#ifndef SYS_WIFI_DISCONNECT
#define SYS_WIFI_DISCONNECT  401u   /* css-private — leave the BSS    */
#endif
#ifndef SYS_WIFI_QUALITY
#define SYS_WIFI_QUALITY     402u   /* css-private — rssi/link quality */
#endif
#ifndef SYS_WIFI_STATE
#define SYS_WIFI_STATE       403u   /* css-private — connection state  */
#endif

/* ══ 01_Com/bt — CSS-PRIVATE numbers, continuation of the 400 block ══
 * Bluetooth has no BSD syscall vocabulary either.  Same allocation, same
 * mark, same place in the range this kernel leaves unimplemented:
 *
 *   400-403  wifi   (above)
 *   404      free   (kept — a fifth WiFi call would take it)
 *   405-408  bt
 */
#ifndef SYS_BT_SCAN_START
#define SYS_BT_SCAN_START    405u   /* css-private — inquiry start     */
#endif
#ifndef SYS_BT_SCAN_STOP
#define SYS_BT_SCAN_STOP     406u   /* css-private — inquiry stop      */
#endif
#ifndef SYS_BT_DISCONNECT
#define SYS_BT_DISCONNECT    407u   /* css-private — drop the link     */
#endif
#ifndef SYS_BT_STATE
#define SYS_BT_STATE         408u   /* css-private — subsystem state   */
#endif
/* ══ 02_Sensors/mic — CSS-PRIVATE numbers, continuation of the block ══
 *   400-403  wifi
 *   404      free   (kept)
 *   405-408  bt
 *   409-412  mic
 */
#ifndef SYS_MIC_READ
#define SYS_MIC_READ       409u   /* css-private — PCM into caller buf */
#endif
#ifndef SYS_MIC_GAIN
#define SYS_MIC_GAIN       410u   /* css-private — set gain (dB)       */
#endif
#ifndef SYS_MIC_MUTE
#define SYS_MIC_MUTE       411u   /* css-private — mute on/off         */
#endif
#ifndef SYS_MIC_STATE
#define SYS_MIC_STATE      412u   /* css-private — subsystem state     */
#endif

/* ══ 03_NonSensors/speaker — CSS-PRIVATE, continuation of the block ══
 *   400-403 wifi · 404 free · 405-408 bt · 409-412 mic · 413-416 spk
 */
#ifndef SYS_SPK_WRITE
#define SYS_SPK_WRITE      413u   /* css-private — PCM into the ring    */
#endif
#ifndef SYS_SPK_VOLUME
#define SYS_SPK_VOLUME     414u   /* css-private — master volume 0..100 */
#endif
#ifndef SYS_SPK_MUTE
#define SYS_SPK_MUTE       415u   /* css-private — mute on/off          */
#endif
#ifndef SYS_SPK_STATE
#define SYS_SPK_STATE      416u   /* css-private — subsystem state      */
#endif

/* ══ 02_Sensors/camera — CSS-PRIVATE, continuation of the block ══════
 *   400-403 wifi · 404 free · 405-408 bt · 409-412 mic
 *   413-416 spk  · 417-420 cam
 */
#ifndef SYS_CAM_STATE
#define SYS_CAM_STATE       417u   /* css-private — pipeline state    */
#endif
#ifndef SYS_CAM_EXPOSURE
#define SYS_CAM_EXPOSURE    418u   /* css-private — exposure (µs)     */
#endif
#ifndef SYS_CAM_GAIN
#define SYS_CAM_GAIN        419u   /* css-private — gain ×100         */
#endif
#ifndef SYS_CAM_STREAM
#define SYS_CAM_STREAM      420u   /* css-private — start/stop stream */
#endif


 #endif /* UIOX_KIX_SCCCS_NUMBERS_H */
 