/**
 * @file    uiox_mouse_demo.c
 * @brief   UIOX Mouse stack end-to-end demonstration.
 *
 * Demonstrates: HAL init → USB HID mouse → move → click →
 *   double-click → scroll → hot zones → acceleration → stats.
 *
 * @date    2026-06-01
 */

 #include "uiox_mouse_device.h"
 #include <stdio.h>
 #include <string.h>
 #include <stdint.h>
 #include <errno.h>
 
 /* =========================================================================
  * Simulated mouse report sequence
  * ====================================================================== */
 
 typedef struct { int8_t dx; int8_t dy; int8_t dz; uint8_t btn; uint32_t at; }
     sim_report_t;
 
 static const sim_report_t s_sim[] = {
     /* dx   dy   dz  btn   at_tick */
     {  10,   0,   0, 0x00,  1 },  /* move right          */
     {  10,   5,   0, 0x00,  2 },  /* move right+down     */
     {   0,   0,   0, 0x01,  3 },  /* LEFT press          */
     {   0,   0,   0, 0x00,  5 },  /* LEFT release → click*/
     {   0,   0,   0, 0x01,  6 },  /* LEFT press (2nd)    */
     {   0,   0,   0, 0x00,  7 },  /* LEFT release → dblclick */
     {   0,   0,  -1, 0x00,  9 },  /* scroll up           */
     {   0,   0,  -1, 0x00, 10 },  /* scroll up           */
     {   0,   0,   0, 0x02, 11 },  /* RIGHT press         */
     {   0,   0,   0, 0x00, 12 },  /* RIGHT release       */
     { -20, -10,   0, 0x00, 14 },  /* move left+up        */
     {   5,   3,   0, 0x04, 16 },  /* MIDDLE press + move */
     {   5,   3,   0, 0x00, 17 },  /* MIDDLE release      */
 };
 
 static uint32_t  s_tick = 0;
 static bool      s_connected = true;
 
 /* =========================================================================
  * Stub HAL ops
  * ====================================================================== */
 
 static int stub_init(uiox_mouse_hw_t *hw)
 {
     (void)hw;
     printf("  [hal] init  USB HID mouse  poll=%u Hz\n",
            hw->poll_rate_hz);
     return 0;
 }
 
 static void stub_deinit(uiox_mouse_hw_t *hw) { (void)hw; }
 
 static int stub_enable(uiox_mouse_hw_t *hw)
 {
     (void)hw;
     printf("  [hal] enable\n");
     hw->connected = true;
     return 0;
 }
 
 static void stub_disable(uiox_mouse_hw_t *hw)
 { (void)hw; printf("  [hal] disable\n"); }
 
 static int stub_read_report(uiox_mouse_hw_t *hw, uiox_mouse_raw_t *raw)
 {
     (void)hw;
     memset(raw, 0, sizeof(*raw));
     raw->connected = s_connected;
 
     /* Find report for current tick */
     for (size_t i = 0; i < sizeof(s_sim)/sizeof(s_sim[0]); i++) {
         if (s_sim[i].at == s_tick) {
             raw->dx      = s_sim[i].dx;
             raw->dy      = s_sim[i].dy;
             raw->dz      = s_sim[i].dz;
             raw->buttons = s_sim[i].btn;
             raw->ts_ns   = (uint64_t)s_tick * 8000000ULL; /* 125 Hz */
             return 1;
         }
     }
     return 0;  /* No data this tick */
 }
 
 static int stub_set_rate(uiox_mouse_hw_t *hw, uint32_t rate_hz)
 {
     (void)hw;
     printf("  [hal] poll rate → %u Hz\n", rate_hz);
     return 0;
 }
 
 static int stub_set_dpi(uiox_mouse_hw_t *hw, uint16_t dpi)
 {
     (void)hw;
     printf("  [hal] DPI → %u\n", dpi);
     return 0;
 }
 
 static int stub_i2c_read(uiox_mouse_hw_t *hw, uint8_t reg,
                           uint8_t *buf, uint16_t len)
 { (void)hw; (void)reg; memset(buf, 0, len); return 0; }
 
 static int stub_i2c_write(uiox_mouse_hw_t *hw, uint8_t reg,
                            const uint8_t *buf, uint16_t len)
 { (void)hw; (void)reg; (void)buf; (void)len; return 0; }
 
 static bool stub_connected(uiox_mouse_hw_t *hw)
 { (void)hw; return s_connected; }
 
 static void stub_isr(uiox_mouse_hw_t *hw) { (void)hw; }
 
 static const uiox_mouse_hw_ops_t stub_ops = {
     .init        = stub_init,
     .deinit      = stub_deinit,
     .enable      = stub_enable,
     .disable     = stub_disable,
     .read_report = stub_read_report,
     .set_rate    = stub_set_rate,
     .set_dpi     = stub_set_dpi,
     .i2c_read    = stub_i2c_read,
     .i2c_write   = stub_i2c_write,
     .connected   = stub_connected,
     .isr         = stub_isr,
 };
 
 /* =========================================================================
  * Hardware device instance
  * ====================================================================== */
 
 static uiox_mouse_hw_t s_hw = {
     .base_addr      = 0x40010000uL,
     .irq            = 56,
     .caps           = UIOX_MOUSE_CAP_USB          |
                       UIOX_MOUSE_CAP_SCROLL_WHEEL  |
                       UIOX_MOUSE_CAP_HSCROLL       |
                       UIOX_MOUSE_CAP_5_BUTTONS     |
                       UIOX_MOUSE_CAP_HIGHRES,
     .if_type        = UIOX_MOUSE_IF_USB_HID,
     .poll_rate_hz   = 125u,
     .resolution_dpi = 2,   /* class indicator */
 };
 
 /* =========================================================================
  * Zone callbacks
  * ====================================================================== */
 
 static void on_zone_enter(int32_t x, int32_t y, uint8_t btn, void *ctx)
 { (void)btn; (void)ctx; printf("  [zone] ENTER '%s' at (%d,%d)\n",
                                (const char *)ctx, x, y); }
 
 static void on_zone_leave(int32_t x, int32_t y, uint8_t btn, void *ctx)
 { (void)btn; (void)ctx; printf("  [zone] LEAVE '%s' at (%d,%d)\n",
                                (const char *)ctx, x, y); }
 
 static void on_zone_click(int32_t x, int32_t y, uint8_t btn, void *ctx)
 { (void)btn; (void)ctx; printf("  [zone] CLICK '%s' at (%d,%d)\n",
                                (const char *)ctx, x, y); }
 
 /* =========================================================================
  * Global event callback
  * ====================================================================== */
 
 static void on_event(const uiox_mouse_event_t *ev, void *ctx)
 {
     (void)ctx;
     static const char *btn_names[] = {"LEFT","RIGHT","MIDDLE","BACK","FWD"};
     switch (ev->type) {
     case UIOX_MOUSE_EV_MOVE:
         printf("  [event] MOVE       (%4d,%4d)  delta=(%+d,%+d)\n",
                ev->x, ev->y, ev->dx, ev->dy);
         break;
     case UIOX_MOUSE_EV_BTN_PRESS:
         printf("  [event] BTN_PRESS  %s  pos=(%d,%d)\n",
                ev->button < 5u ? btn_names[ev->button] : "?",
                ev->x, ev->y);
         break;
     case UIOX_MOUSE_EV_BTN_RELEASE:
         printf("  [event] BTN_REL    %s  pos=(%d,%d)\n",
                ev->button < 5u ? btn_names[ev->button] : "?",
                ev->x, ev->y);
         break;
     case UIOX_MOUSE_EV_CLICK:
         printf("  [event] CLICK      %s  pos=(%d,%d)\n",
                ev->button < 5u ? btn_names[ev->button] : "?",
                ev->x, ev->y);
         break;
     case UIOX_MOUSE_EV_DBLCLICK:
         printf("  [event] DBLCLICK   %s  pos=(%d,%d)\n",
                ev->button < 5u ? btn_names[ev->button] : "?",
                ev->x, ev->y);
         break;
     case UIOX_MOUSE_EV_SCROLL_V:
         printf("  [event] SCROLL_V   dz=%+d\n", ev->dz);
         break;
     case UIOX_MOUSE_EV_SCROLL_H:
         printf("  [event] SCROLL_H   dw=%+d\n", ev->dw);
         break;
     case UIOX_MOUSE_EV_CONNECT:
         printf("  [event] CONNECT\n"); break;
     case UIOX_MOUSE_EV_DISCONNECT:
         printf("  [event] DISCONNECT\n"); break;
     default: break;
     }
 }
 
 /* =========================================================================
  * main
  * ====================================================================== */
 
 int main(void)
 {
     printf("=== UIOX Mouse Stack Demo ===\n\n");
 
    /* ------------------------------------------------------------------ */
    /* 1. Open device                                                      */
    /* ------------------------------------------------------------------ */

    printf("--- Open ---\n");
    uiox_mouse_device_t dev;
    uiox_mouse_open_params_t p;
    memset(&p, 0, sizeof(p));

    p.hw      = &s_hw;
    p.hw_ops  = &stub_ops;
    p.proto   = UIOX_MOUSE_PROTO_HID_BOOT;

    /* Event processing config */
    p.event_cfg.debounce_ms        = 10u;
    p.event_cfg.click_timeout_ms   = 200u;
    p.event_cfg.dblclick_timeout_ms= 400u;
    p.event_cfg.accel_factor       = 1.5f;
    p.event_cfg.accel_threshold    = 5.0f;
    p.event_cfg.screen_w           = 1920;
    p.event_cfg.screen_h           = 1080;
    p.event_cfg.invert_y           = false;
    p.event_cfg.invert_scroll      = false;

    int rc = uiox_mouse_open(&dev, &p);
    if (rc < 0) {
        printf("[error] uiox_mouse_open failed: %d\n", rc);
        return 1;
    }
    printf("  Protocol   : HID Boot (USB)\n");
    printf("  Poll rate  : %u Hz\n", s_hw.poll_rate_hz);
    printf("  Screen     : %dx%d\n",
           p.event_cfg.screen_w, p.event_cfg.screen_h);
    printf("  Accel      : factor=%.1f  threshold=%.1f\n",
           p.event_cfg.accel_factor, p.event_cfg.accel_threshold);

    /* ------------------------------------------------------------------ */
    /* 2. Register event callback                                          */
    /* ------------------------------------------------------------------ */

    printf("\n--- Register event callback ---\n");
    uiox_mouse_add_callback(&dev, on_event, NULL, 0u);
    printf("  Global event callback registered\n");

    /* ------------------------------------------------------------------ */
    /* 3. Register hot zones                                               */
    /* ------------------------------------------------------------------ */

    printf("\n--- Register hot zones ---\n");

    /* Top-left corner: Home zone */
    uiox_mouse_zone_t home_zone = {
        .x        = 0,   .y   = 0,
        .w        = 100, .h   = 100,
        .on_enter = on_zone_enter,
        .on_leave = on_zone_leave,
        .on_click = on_zone_click,
        .ctx      = (void *)"HOME",
    };
    uiox_mouse_add_zone(&dev, &home_zone);
    printf("  Zone 'HOME'   (0,0)-(100,100)\n");

    /* Centre: Main area */
    uiox_mouse_zone_t main_zone = {
        .x        = 400,  .y  = 200,
        .w        = 500,  .h  = 400,
        .on_enter = on_zone_enter,
        .on_leave = on_zone_leave,
        .on_click = on_zone_click,
        .ctx      = (void *)"MAIN_AREA",
    };
    uiox_mouse_add_zone(&dev, &main_zone);
    printf("  Zone 'MAIN_AREA' (400,200)-(900,600)\n");

    /* Bottom-right: Close button */
    uiox_mouse_zone_t close_zone = {
        .x        = 1880, .y  = 0,
        .w        = 40,   .h  = 40,
        .on_enter = on_zone_enter,
        .on_leave = on_zone_leave,
        .on_click = on_zone_click,
        .ctx      = (void *)"CLOSE_BTN",
    };
    uiox_mouse_add_zone(&dev, &close_zone);
    printf("  Zone 'CLOSE_BTN' (1880,0)-(1920,40)\n");

    /* ------------------------------------------------------------------ */
    /* 4. Start device                                                     */
    /* ------------------------------------------------------------------ */

    printf("\n--- Start ---\n");
    rc = uiox_mouse_start(&dev);
    printf("  Mouse: ACTIVE  rc=%d  connected=%s\n",
           rc, uiox_mouse_connected(&dev) ? "YES" : "NO");

    /* ------------------------------------------------------------------ */
    /* 5. Warp cursor to centre                                            */
    /* ------------------------------------------------------------------ */

    printf("\n--- Cursor warp to centre ---\n");
    uiox_mouse_warp(&dev, 960, 540);
    int32_t cx = 0, cy = 0;
    uiox_mouse_cursor(&dev, &cx, &cy);
    printf("  Cursor warped to (%d, %d)\n", cx, cy);

    /* ------------------------------------------------------------------ */
    /* 6. Simulated input tick loop                                        */
    /* ------------------------------------------------------------------ */

    printf("\n--- Input simulation (%zu ticks) ---\n",
           sizeof(s_sim)/sizeof(s_sim[0]) + 5u);

    /* Find max tick */
    uint32_t max_tick = 0;
    for (size_t i = 0; i < sizeof(s_sim)/sizeof(s_sim[0]); i++)
        if (s_sim[i].at > max_tick) max_tick = s_sim[i].at;

    for (s_tick = 1; s_tick <= max_tick + 5u; s_tick++) {
        uint32_t now_ms = s_tick * 8u;  /* 125 Hz → 8 ms per tick */
        printf("\n  [tick %2u  t=%u ms]\n", s_tick, now_ms);
        uiox_mouse_tick(&dev, now_ms);
    }

    /* ------------------------------------------------------------------ */
    /* 7. Simulate disconnection                                           */
    /* ------------------------------------------------------------------ */

    printf("\n--- Simulate disconnect ---\n");
    s_connected = false;
    uiox_mouse_tick(&dev, 200u);
    printf("  Connected: %s\n", uiox_mouse_connected(&dev) ? "YES" : "NO");

    /* Reconnect */
    printf("\n--- Simulate reconnect ---\n");
    s_connected = true;
    uiox_mouse_tick(&dev, 300u);
    printf("  Connected: %s\n", uiox_mouse_connected(&dev) ? "YES" : "NO");

    /* ------------------------------------------------------------------ */
    /* 8. Final cursor position                                            */
    /* ------------------------------------------------------------------ */

    printf("\n--- Final cursor position ---\n");
    uiox_mouse_cursor(&dev, &cx, &cy);
    printf("  Cursor: (%d, %d)\n", cx, cy);

    /* ------------------------------------------------------------------ */
    /* 9. Statistics                                                       */
    /* ------------------------------------------------------------------ */

    printf("\n--- Statistics ---\n");
    uiox_mouse_print_stats(&dev);

    /* ------------------------------------------------------------------ */
    /* 10. Stop and close                                                  */
    /* ------------------------------------------------------------------ */

    printf("\n--- Stop and close ---\n");
    uiox_mouse_stop(&dev);
    printf("  State: %s\n", uiox_mouse_state_name(dev.subsys.state));
    uiox_mouse_close(&dev);
    printf("  Device: CLOSED\n");

    printf("\n=== UIOX Mouse Demo complete ===\n");
    return 0;
}
