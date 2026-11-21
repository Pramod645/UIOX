/*
 * 34_CSS/02_Sensors/WiFi/uiox_kix_css_wifi.c
 *
 * WiFi's four syscall entry points.  Each unpacks its register slots,
 * gates what it dereferences, and calls the subsystem API in
 * uiox_wifi_subsys.h.  None of them holds WiFi logic: scan, disconnect,
 * quality and state are the subsystem's, and a line of any of it here
 * would be a second copy.
 *
 * ── the gate is this family's ───────────────────────────────────────
 * uiox_kix_sccss.h declares no user-address gate — whether one is
 * needed depends on what a family's calls take.  Of the four here, only
 * quality() writes through a caller pointer, so only it needs one.
 *
 * The bounds below are 50_scpcs's, character for character: two
 * subsystems that disagree about which addresses are acceptable is
 * worse than either refusing them.
 *
 * @version 1.0.0  @date 2026-11-21
 */

 #include "uiox_kix_css_wifi.h"

 /* ── Result codes ────────────────────────────────────────────────────
  * The values uiox_wifi_subsys.h's callers already see, repeated because
  * the kernel builds -nostdinc. */
 #define WIFI_EFAULT   ((int64_t)-14)
 #define WIFI_EINVAL   ((int64_t)-22)
 #define WIFI_ENOSYS   ((int64_t)-38)
 #define WIFI_ENODEV   ((int64_t)-19)
 
 /* ── The user-address gate ───────────────────────────────────────────
  * local static, see the banner.  Returns 0 when [p, p+len) lies inside
  * the user window and is aligned; WIFI_EFAULT otherwise.  A zero length
  * is always valid.
  *
  * The wrap test comes BEFORE the range test: base + len can overflow,
  * and a wrapped end would pass a naive comparison.  The alignment test
  * is a mask, not a modulo — a 64-bit '%' lowers to __aeabi_uldivmod on
  * arm32, which is a libgcc helper this freestanding link does not carry. */
 #define WIFI_USER_BASE   ((uiox_uint64_t)0x00001000ull)
 #define WIFI_USER_TOP    ((uiox_uint64_t)0xC0000000ull)
 
 static int wifi_check_user_ptr(uiox_uintptr_t p, uiox_uint64_t len,
                                uiox_uint64_t align)
 {
     uiox_uint64_t a   = (uiox_uint64_t)p;
     uiox_uint64_t end;
 
     if (len == 0u) return 0;
     if (a  < WIFI_USER_BASE) return WIFI_EFAULT;
 
     end = a + len;
     if (end < a) return WIFI_EFAULT;        /* the sum wrapped */
     if (end > WIFI_USER_TOP) return WIFI_EFAULT;
 
     if (align && (a & (align - 1ull)) != 0u) return WIFI_EFAULT;
 
     return 0;
 }
 
 /* ── scan(timeout_ms) ────────────────────────────────────────────────
  * One value, nothing dereferenced — but the timeout is checked, because
  * uiox_wifi_subsys_scan() passes it to the protocol layer's scan
  * timeout and a zero would have it give up before the first beacon. */
 int64_t uiox_kix_css_wifi_scan(uiox_uint64_t timeout_ms)
 {
     uiox_wifi_subsys_t *sys = uiox_kix_css_wifi_current();
 
     if (!sys || !sys->wif.hw) return WIFI_ENODEV;   /* not bound yet */
     if (!timeout_ms)  return WIFI_EINVAL;
 
     return (int64_t)uiox_wifi_subsys_scan(sys, (uiox_uint32_t)timeout_ms);
 }
 
 /* ── disconnect(void) ────────────────────────────────────────────────
  * No arguments.  A subsystem already stopped is left alone rather than
  * faulted: there is nothing to leave. */
 int64_t uiox_kix_css_wifi_disconnect(void)
 {
     uiox_wifi_subsys_t *sys = uiox_kix_css_wifi_current();
 
     if (!sys) return WIFI_ENODEV;
 
     return (int64_t)uiox_wifi_subsys_disconnect(sys);
 }
 
 /* ── quality(out) ────────────────────────────────────────────────────
  * out is MANDATORY and WRITTEN THROUGH for a whole uiox_wifi_quality_t,
  * so it is gated.  A NULL is EFAULT, not a silent no-op.
  *
  * uiox_wifi_subsys_quality() takes a non-const pointer and fills it, so
  * the cast below is the one the API asks for. */
 int64_t uiox_kix_css_wifi_quality(uiox_uintptr_t out)
 {
     uiox_wifi_subsys_t *sys = uiox_kix_css_wifi_current();
 
     if (!sys)   return WIFI_ENODEV;
     if (!out)   return WIFI_EFAULT;
     if (wifi_check_user_ptr(out, (uiox_uint64_t)sizeof(uiox_wifi_quality_t),
                             4u))
         return WIFI_EFAULT;
 
     uiox_wifi_subsys_quality(sys, (uiox_wifi_quality_t *)(uintptr_t)out);
     return 0;
 }
 
 /* ── state(void) ─────────────────────────────────────────────────────
  * Returns the subsystem state as a small integer — one of
  * UIOX_WIFI_SUBSYS_STOPPED .. _RECONNECTING.  A caller polling for a
  * connection reads this between scan and quality.
  *
  * Reading sys->state directly is the accessor: the header exposes the
  * field and declares no getter for it. */
 int64_t uiox_kix_css_wifi_state(void)
 {
     uiox_wifi_subsys_t *sys = uiox_kix_css_wifi_current();
 
     if (!sys) return WIFI_ENODEV;
 
     return (int64_t)sys->state;
 }
 