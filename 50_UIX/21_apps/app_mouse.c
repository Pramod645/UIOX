/*
 * 50_UIX/21_apps/app_mouse.c
 *
 * Mouse application — EL0.  Waits for events and prints them.
 *
 *   21_apps/app_mouse.c → dev/mouse/ → sys_read / uix_poll
 *      → SCFS table → the mouse driver
 *
 * No 30_KIX header.
 *
 * ── WHY read() AND NOT ioctl  ───────────────────────────────────────
 * MOUSE is family 3 (EVENT), so the read branch is the interface:
 * uiox_mouse_if.h publishes uiox_event_ops_t {pending, read} and no
 * command table.  The cursor arrives inside every event (absolute x/y,
 * post-acceleration), so no command is needed for it either.
 *
 * ── WHAT IT CANNOT DO ───────────────────────────────────────────────
 *   warp(x, y)     no command exists; the cursor moves by events only
 *   add_zone / add_callback  kernel-side registration
 *
 * v1.0.0  @date 2026-11-18
 */

 #include "mouse/uiox_dev_mouse.h"
 #include "uiox_dev.h"
 #include "uix_stdio.h"
 
 static const char *ev_name(int t)
 {
     switch (t) {
     case UIOX_DEV_MOUSE_EV_MOVE:        return "MOVE";
     case UIOX_DEV_MOUSE_EV_BTN_PRESS:   return "BTN_PRESS";
     case UIOX_DEV_MOUSE_EV_BTN_RELEASE: return "BTN_RELEASE";
     case UIOX_DEV_MOUSE_EV_CLICK:       return "CLICK";
     case UIOX_DEV_MOUSE_EV_DBLCLICK:    return "DBLCLICK";
     case UIOX_DEV_MOUSE_EV_SCROLL_V:    return "SCROLL_V";
     case UIOX_DEV_MOUSE_EV_SCROLL_H:    return "SCROLL_H";
     case UIOX_DEV_MOUSE_EV_CONNECT:     return "CONNECT";
     case UIOX_DEV_MOUSE_EV_DISCONNECT:  return "DISCONNECT";
     case UIOX_DEV_MOUSE_EV_GESTURE:     return "GESTURE";
     default:                            return "?";
     }
 }
 
 /* Signed decimal, no float.  A -20 delta prints as -20. */
 static void print_i(int v)
 {
     if (v < 0) uix_printf("-%u", (unsigned int)(-v));
     else       uix_printf("%u",  (unsigned int)v);
 }
 
 int app_mouse_main(void)
 {
     uiox_dev_mouse_t       m;
     uiox_dev_mouse_event_t ev;
     uiox_dev_err_t         rc;
     unsigned int           got = 0u, i;
 
     uix_printf("mouse: opening /dev/mouse0\n");
 
     rc = uiox_dev_mouse_open(&m, 0, UIOX_O_RDONLY);
     if (rc != UIOX_DEV_OK) {
         uix_printf("mouse: open failed (%d) — no /dev/mouse0.\n", (int)rc);
         uix_printf("       Is the node created, and did uiox_mouse_probe() run?\n");
         return 1;
     }
 
     /* Bounded: a mouse nobody touches would otherwise block forever. */
     for (i = 0u; i < 8u; i++) {
         rc = uiox_dev_mouse_wait(&m, 1000);          /* 1 s per event */
         if (rc == UIOX_DEV_ENODEV) {
             uix_printf("mouse: fd not serviceable — stopping\n");
             break;
         }
         if (rc != UIOX_DEV_OK) {
             uix_printf("mouse: no event within 1 s — stopping\n");
             break;
         }
 
         rc = uiox_dev_mouse_read(&m, &ev);
         if (rc != UIOX_DEV_OK) {
             uix_printf("mouse: read returned nothing usable — stopping\n");
             break;
         }
 
         got++;
         uix_printf("mouse: [%u] %s  pos=(%d,%d)  d=(", i, ev_name(ev.type),
                    ev.x, ev.y);
         print_i(ev.dx); uix_printf(",");
         print_i(ev.dy); uix_printf(")  scroll=(");
         print_i(ev.dz); uix_printf(",");
         print_i(ev.dw); uix_printf(")  btn=0x%02X  clicks=%u\n",
                                   ev.buttons, ev.click_count);
     }
 
     /* The cursor comes from the remembered event, not a command. */
     {
         int cx = 0, cy = 0;
         if (uiox_dev_mouse_cursor(&m, &cx, &cy) == UIOX_DEV_OK)
             uix_printf("mouse: cursor (%d, %d)\n", cx, cy);
         else
             uix_printf("mouse: cursor unknown (no event read)\n");
     }
 
     uix_printf("mouse: %u event(s) read\n", got);
     uix_printf("mouse: NOTE — warp() has no command; the cursor moves by\n");
     uix_printf("       events only, and add_zone/add_callback are kernel-side.\n");
 
     (void)uiox_dev_mouse_close(&m);
     uix_printf("mouse: closed\n");
     return 0;
 }
 
 int main(void)
 {
     return app_mouse_main();
 }
 