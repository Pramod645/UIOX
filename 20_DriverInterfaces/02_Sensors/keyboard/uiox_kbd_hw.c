/**
 * @file    uiox_kbd_hw.c
 * @brief   UIOX Keyboard HAL — generic hardware lifecycle management.
 * @date    2026-05-27
 */

 #include "uiox_kbd_hw.h"
 #include <string.h>
 #include <errno.h>
 
 int uiox_kbd_hw_init(uiox_kbd_hw_t *hw, const uiox_kbd_hw_ops_t *ops)
 {
     if (!hw || !ops || !ops->init) return -EINVAL;
     hw->priv      = (void *)ops;
     hw->led_state = 0u;
     hw->backlight_level = 0u;
     return ops->init(hw);
 }
 
 void uiox_kbd_hw_deinit(uiox_kbd_hw_t *hw)
 {
     if (!hw || !hw->priv) return;
     const uiox_kbd_hw_ops_t *ops = (const uiox_kbd_hw_ops_t *)hw->priv;
     if (ops->deinit) ops->deinit(hw);
     hw->priv = NULL;
 }
 
 int uiox_kbd_hw_scan_row(uiox_kbd_hw_t *hw, uint8_t row, uint16_t *cols_out)
 {
     if (!hw || !hw->priv || !cols_out) return -EINVAL;
     const uiox_kbd_hw_ops_t *ops = (const uiox_kbd_hw_ops_t *)hw->priv;
     if (!ops->scan_row) return -ENOSYS;
     return ops->scan_row(hw, row, cols_out);
 }
 
 int uiox_kbd_hw_read_direct(uiox_kbd_hw_t *hw, uint32_t *keys_out)
 {
     if (!hw || !hw->priv || !keys_out) return -EINVAL;
     const uiox_kbd_hw_ops_t *ops = (const uiox_kbd_hw_ops_t *)hw->priv;
     if (!ops->read_direct) return -ENOSYS;
     return ops->read_direct(hw, keys_out);
 }
 
 int uiox_kbd_hw_set_leds(uiox_kbd_hw_t *hw, uint8_t led_mask)
 {
     if (!hw || !hw->priv) return -EINVAL;
     const uiox_kbd_hw_ops_t *ops = (const uiox_kbd_hw_ops_t *)hw->priv;
     if (!ops->set_leds) return -ENOSYS;
     hw->led_state = led_mask;
     return ops->set_leds(hw, led_mask);
 }
 
 int uiox_kbd_hw_set_backlight(uiox_kbd_hw_t *hw, uint8_t level)
 {
     if (!hw || !hw->priv) return -EINVAL;
     const uiox_kbd_hw_ops_t *ops = (const uiox_kbd_hw_ops_t *)hw->priv;
     if (!ops->set_backlight) return -ENOSYS;
     hw->backlight_level = level;
     return ops->set_backlight(hw, level);
 }
 