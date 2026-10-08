/**
 * @file    uiox_kbd_hw.c
 * @brief   UIOX Keyboard HAL — generic hardware lifecycle management.
 * @date    2026-05-27
 */

 #include "uiox_kbd_hw.h"
 
 int uiox_kbd_hw_init(uiox_kbd_hw_t *hw, const uiox_kbd_hw_ops_t *ops)
 {
     if (!hw || !ops || !ops->init) return -EINVAL;
     hw->ops      = (void *)ops;
     hw->led_state = 0u;
     hw->backlight_level = 0u;
     return ops->init(hw);
 }
 
 void uiox_kbd_hw_deinit(uiox_kbd_hw_t *hw)
 {
     if (!hw || !hw->ops) return;
     const uiox_kbd_hw_ops_t *ops = (const uiox_kbd_hw_ops_t *)hw->ops;
     if (ops->deinit) ops->deinit(hw);
     hw->ops = NULL;
 }
 
 int uiox_kbd_hw_scan_row(uiox_kbd_hw_t *hw, uint8_t row, uint16_t *cols_out)
 {
     if (!hw || !hw->ops || !cols_out) return -EINVAL;
     const uiox_kbd_hw_ops_t *ops = (const uiox_kbd_hw_ops_t *)hw->ops;
     if (!ops->scan_row) return -ENOSYS;
     return ops->scan_row(hw, row, cols_out);
 }
 
 int uiox_kbd_hw_read_direct(uiox_kbd_hw_t *hw, uint32_t *keys_out)
 {
     if (!hw || !hw->ops || !keys_out) return -EINVAL;
     const uiox_kbd_hw_ops_t *ops = (const uiox_kbd_hw_ops_t *)hw->ops;
     if (!ops->read_direct) return -ENOSYS;
     return ops->read_direct(hw, keys_out);
 }
 
 int uiox_kbd_hw_set_leds(uiox_kbd_hw_t *hw, uint8_t led_mask)
 {
     if (!hw || !hw->ops) return -EINVAL;
     const uiox_kbd_hw_ops_t *ops = (const uiox_kbd_hw_ops_t *)hw->ops;
     if (!ops->set_leds) return -ENOSYS;
     hw->led_state = led_mask;
     return ops->set_leds(hw, led_mask);
 }
 
 int uiox_kbd_hw_set_backlight(uiox_kbd_hw_t *hw, uint8_t level)
 {
     if (!hw || !hw->ops) return -EINVAL;
     const uiox_kbd_hw_ops_t *ops = (const uiox_kbd_hw_ops_t *)hw->ops;
     if (!ops->set_backlight) return -ENOSYS;
     hw->backlight_level = level;
     return ops->set_backlight(hw, level);
 }
 