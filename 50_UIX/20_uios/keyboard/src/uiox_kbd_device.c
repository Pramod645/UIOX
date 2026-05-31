/**
 * @file    uiox_kbd_device.c
 * @brief   UIOX Keyboard device API implementation.
 * @date    2026-05-27
 */

 #include "uiox_kbd_device.h"
 #include <string.h>
 #include <errno.h>
 #include <stdio.h>
 
 int uiox_kbd_open(uiox_kbd_device_t           *dev,
                    const uiox_kbd_open_params_t *p)
 {
     if (!dev || !p || !p->hw || !p->hw_ops) return -EINVAL;
     memset(dev, 0, sizeof(*dev));
     dev->hw = p->hw;
 
     /* 1. HAL init */
     int rc = uiox_kbd_hw_init(p->hw, p->hw_ops);
     if (rc < 0) return rc;
 
     /* 2. Subsystem init */
     rc = uiox_kbd_subsys_init(&dev->subsys,
                                p->hw,
                                p->if_type,
                                &p->event_cfg,
                                p->layout);
     if (rc < 0) return rc;
 
     /* 3. Backlight dim config */
     uiox_kbd_subsys_set_dim(&dev->subsys,
                              p->dim_timeout_ms,
                              p->active_backlight,
                              p->dim_backlight);
 
     dev->open = true;
     return 0;
 }
 
 int uiox_kbd_start(uiox_kbd_device_t *dev)
 {
     if (!dev || !dev->open) return -EINVAL;
     return uiox_kbd_subsys_start(&dev->subsys);
 }
 
 void uiox_kbd_stop(uiox_kbd_device_t *dev)
 {
     if (!dev || !dev->open) return;
     uiox_kbd_subsys_stop(&dev->subsys);
 }
 
 void uiox_kbd_close(uiox_kbd_device_t *dev)
 {
     if (!dev || !dev->open) return;
     uiox_kbd_stop(dev);
     uiox_kbd_hw_deinit(dev->hw);
     dev->open = false;
 }
 
 int uiox_kbd_tick(uiox_kbd_device_t *dev,
                    uint32_t           now_ms,
                    uint64_t           ts_ns)
 {
     if (!dev || !dev->open) return -EINVAL;
     return uiox_kbd_subsys_tick(&dev->subsys, now_ms, ts_ns);
 }
 
 bool uiox_kbd_poll(uiox_kbd_device_t *dev, uiox_kbd_event_t *ev_out)
 {
     if (!dev || !dev->open || !ev_out) return false;
     return uiox_kbd_buf_pop(&dev->subsys.cooked_rb, ev_out);
 }
 
 int uiox_kbd_add_shortcut(uiox_kbd_device_t     *dev,
                            uint8_t                modifiers,
                            uint16_t               keycode,
                            uiox_kbd_shortcut_fn_t fn,
                            void                  *ctx,
                            const char            *name)
 {
     if (!dev || !dev->open) return -EINVAL;
     return uiox_kbd_subsys_add_shortcut(&dev->subsys,
                                          modifiers, keycode,
                                          fn, ctx, name);
 }
 
 int uiox_kbd_add_callback(uiox_kbd_device_t   *dev,
                            uiox_kbd_event_fn_t  fn,
                            void                *ctx,
                            uint8_t              filter_type)
 {
     if (!dev || !dev->open) return -EINVAL;
     return uiox_kbd_subsys_add_callback(&dev->subsys,
                                          fn, ctx, filter_type);
 }
 
 int uiox_kbd_set_layout(uiox_kbd_device_t *dev, uiox_kbd_layout_t layout)
 {
     if (!dev || !dev->open) return -EINVAL;
     return uiox_kbd_map_set_layout(layout);
 }
 
 int uiox_kbd_set_leds(uiox_kbd_device_t *dev, uint8_t led_mask)
 {
     if (!dev || !dev->open) return -EINVAL;
     return uiox_kbd_hw_set_leds(dev->hw, led_mask);
 }
 
 int uiox_kbd_set_backlight(uiox_kbd_device_t *dev, uint8_t level)
 {
     if (!dev || !dev->open) return -EINVAL;
     return uiox_kbd_hw_set_backlight(dev->hw, level);
 }
 
 uint8_t uiox_kbd_modifiers(const uiox_kbd_device_t *dev)
 {
     if (!dev || !dev->open) return 0u;
     return uiox_kbd_event_modifiers(&dev->subsys.event_ctx);
 }
 
 bool uiox_kbd_lock_state(const uiox_kbd_device_t *dev, uint8_t led_bit)
 {
     if (!dev || !dev->open) return false;
     return uiox_kbd_event_lock(&dev->subsys.event_ctx, led_bit);
 }
 
 const char *uiox_kbd_layout_name(void)
 {
     return uiox_kbd_map_name();
 }
 
 void uiox_kbd_print_stats(const uiox_kbd_device_t *dev)
 {
     if (!dev || !dev->open) return;
     const uiox_kbd_subsys_t *s = &dev->subsys;
     printf("  Scan cycles   : %u\n",   s->scan_count);
     printf("  Key presses   : %llu\n", (unsigned long long)s->total_keypresses);
     printf("  Key releases  : %llu\n", (unsigned long long)s->total_releases);
     printf("  Auto-repeats  : %llu\n", (unsigned long long)s->total_repeats);
     printf("  RX overflows  : %u\n",   s->raw_rb.overflow);
     printf("  Layout        : %s\n",   uiox_kbd_map_name());
     printf("  Modifiers     : 0x%02X\n", s->event_ctx.modifiers);
     printf("  CapsLock      : %s\n",   s->event_ctx.caps_lock ? "ON" : "OFF");
     printf("  NumLock       : %s\n",   s->event_ctx.num_lock  ? "ON" : "OFF");
     printf("  LED state     : 0x%02X\n", s->led_state);
 }
 