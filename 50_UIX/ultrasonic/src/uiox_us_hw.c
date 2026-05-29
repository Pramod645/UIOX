/**
 * @file    uiox_us_hw.c
 * @brief   UIOX Ultrasonic HAL — generic hardware lifecycle management.
 * @date    2026-05-26
 */

 #include "uiox_us_hw.h"
 #include <string.h>
 #include <errno.h>
 
 int uiox_us_hw_init(uiox_us_hw_t *hw, const uiox_us_hw_ops_t *ops)
 {
     if (!hw || !ops || !ops->init) return -EINVAL;
     hw->priv    = (void *)ops;
     hw->rx_head = 0;
     hw->rx_tail = 0;
     memset(hw->echo_done,    0, sizeof(hw->echo_done));
     memset(hw->echo_timeout, 0, sizeof(hw->echo_timeout));
     return ops->init(hw);
 }
 
 int uiox_us_hw_trigger(uiox_us_hw_t *hw, uint8_t ch,
                         const uiox_us_trig_cfg_t *cfg)
 {
     if (!hw || !hw->priv || !cfg) return -EINVAL;
     const uiox_us_hw_ops_t *ops = (const uiox_us_hw_ops_t *)hw->priv;
     if (!ops->trigger) return -ENOSYS;
     hw->echo_done[ch]    = false;
     hw->echo_timeout[ch] = false;
     return ops->trigger(hw, ch, cfg);
 }
 
 int64_t uiox_us_hw_echo_wait(uiox_us_hw_t *hw, uint8_t ch,
                                uint32_t timeout_ms)
 {
     if (!hw || !hw->priv) return -EINVAL;
     const uiox_us_hw_ops_t *ops = (const uiox_us_hw_ops_t *)hw->priv;
     if (!ops->echo_wait) return -ENOSYS;
     return ops->echo_wait(hw, ch, timeout_ms);
 }
 
 int uiox_us_hw_read_temp(uiox_us_hw_t *hw, int32_t *milli_celsius)
 {
     if (!hw || !hw->priv || !milli_celsius) return -EINVAL;
     const uiox_us_hw_ops_t *ops = (const uiox_us_hw_ops_t *)hw->priv;
     if (!ops->read_temp_mc) return -ENOSYS;
     return ops->read_temp_mc(hw, milli_celsius);
 }
 
 void uiox_us_hw_deinit(uiox_us_hw_t *hw)
 {
     if (!hw || !hw->priv) return;
     const uiox_us_hw_ops_t *ops = (const uiox_us_hw_ops_t *)hw->priv;
     if (ops->deinit) ops->deinit(hw);
     hw->priv = NULL;
 }
 