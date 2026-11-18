/*
 * 50_UIX/21_apps/app_kbd.c
 *
 * Keyboard application — EL0.  Waits for events and prints them.
 *
 *   21_apps/app_kbd.c → dev/kbd/ → dev/uiox_dev_event.c (shape)
 *      → sys_read / uix_poll → SCFS table → the keyboard driver
 *
 * No 30_KIX header.
 *
 * ── WHY THIS IS A read() AND NOT AN ioctl() ─────────────────────────
 * KBD is family 3 (EVENT), the one family where the read branch is the
 * interface: uiox_kbd_if.h publishes uiox_event_ops_t {pending, read}
 * and no command table.  So this app waits on the fd and reads records.
 *
 * ── WHAT IT CANNOT DO ───────────────────────────────────────────────
 *   set_leds / set_backlight  no ioctl exists
 *   add_shortcut / add_callback  function-pointer registration, kernel-side
 *
 * v1.0.0  @date 2026-11-18
 */

 #include "kbd/uiox_dev_kbd.h"
 #include "uiox_dev.h"
 #include "uix_stdio.h"
 
 static const char *ev_name(unsigned char t)
 {
     switch (t) {
     case UIOX_DEV_KBD_EV_PRESS:   return "PRESS";
     case UIOX_DEV_KBD_EV_RELEASE: return "RELEASE";
     case UIOX_DEV_KBD_EV_REPEAT:  return "REPEAT";
     case UIOX_DEV_KBD_EV_SPECIAL: return "SPECIAL";
     default:                      return "?";
     }
 }
 
 /* 64-bit decimal without %llu: split into two 32-bit halves. */
 static void print_u64(unsigned long long v)
 {
     unsigned int hi = (unsigned int)(v >> 32);
     unsigned int lo = (unsigned int)(v & 0xFFFFFFFFull);
     if (hi != 0u) uix_printf("%u%09u", hi, lo);
     else          uix_printf("%u", lo);
 }
 
 int app_kbd_main(void)
 {
     uiox_dev_kbd_t       k;
     uiox_dev_kbd_event_t ev;
     uiox_dev_err_t       rc;
     unsigned int         got = 0u, i;
 
     uix_printf("kbd: opening /dev/kbd0\n");
 
     rc = uiox_dev_kbd_open(&k, 0, UIOX_O_RDONLY);
     if (rc != UIOX_DEV_OK) {
         uix_printf("kbd: open failed (%d) — no /dev/kbd0.\n", (int)rc);
         uix_printf("     Is the node created, and did uiox_kbd_probe() run?\n");
         return 1;
     }
 
     /* ── wait, then read, eight times ────────────────────────────────
      * A bounded loop: an open keyboard with nobody typing would
      * otherwise block forever in a demo that has no other exit. */
     for (i = 0u; i < 8u; i++) {
         rc = uiox_dev_kbd_wait(&k, 1000);          /* 1 s per event */
         if (rc == UIOX_DEV_ENODEV) {
             uix_printf("kbd: fd not serviceable — stopping\n");
             break;
         }
         if (rc != UIOX_DEV_OK) {
             uix_printf("kbd: no event within 1 s (no key pressed?) — stopping\n");
             break;
         }
 
         rc = uiox_dev_kbd_read(&k, &ev);
         if (rc != UIOX_DEV_OK) {
             uix_printf("kbd: read returned nothing — stopping\n");
             break;
         }
 
         got++;
         uix_printf("kbd: [%u] %s  keycode=%u  unicode=0x%X  row=%u col=%u"
                    "  mods=0x%02X  ts=", i, ev_name(ev.ev_type),
                    (unsigned int)ev.keycode, ev.unicode,
                    ev.row, ev.col, ev.modifiers);
         print_u64(ev.ts_ns);
         uix_printf(" ns\n");
     }
 
     uix_printf("kbd: %u event(s) read\n", got);
     uix_printf("kbd: NOTE — no LED or backlight command exists at EL0,\n");
     uix_printf("     and add_shortcut/add_callback are kernel-side.\n");
 
     (void)uiox_dev_kbd_close(&k);
     uix_printf("kbd: closed\n");
     return 0;
 }
 
 int main(void)
 {
     return app_kbd_main();
 }
 