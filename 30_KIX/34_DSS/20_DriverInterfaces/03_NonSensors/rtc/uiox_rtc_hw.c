/**
 * @file  uiox_rtc_hw.c
 * @brief UIOX RTC HAL implementation.
 * @date  2026-06-10
 */

 #include "uiox_rtc_hw.h"
 
 /* Retrieve ops pointer stored in hw->priv */
 #define OPS(hw) ((const uiox_rtc_hw_ops_t *)(hw)->priv)
 
 int uiox_rtc_hw_init(uiox_rtc_hw_t *hw, const uiox_rtc_hw_ops_t *ops)
 {
     if (!hw || !ops || !ops->init) return -EINVAL;
     hw->priv        = (void *)ops;
     hw->bat_state   = UIOX_RTC_BAT_UNKNOWN;
     hw->pending_irq = 0u;
     return ops->init(hw);
 }
 
 void uiox_rtc_hw_deinit(uiox_rtc_hw_t *hw)
 {
     if (!hw || !hw->priv) return;
     if (OPS(hw)->deinit) OPS(hw)->deinit(hw);
     hw->priv = NULL;
 }
 
 uint8_t uiox_rtc_hw_reg_read(uiox_rtc_hw_t *hw, uint8_t reg)
 {
     if (!hw || !hw->priv || !OPS(hw)->reg_read) return 0u;
     return OPS(hw)->reg_read(hw, reg);
 }
 
 void uiox_rtc_hw_reg_write(uiox_rtc_hw_t *hw, uint8_t reg, uint8_t val)
 {
     if (!hw || !hw->priv || !OPS(hw)->reg_write) return;
     OPS(hw)->reg_write(hw, reg, val);
 }
 
 uiox_rtc_bat_t uiox_rtc_hw_bat_check(uiox_rtc_hw_t *hw)
 {
     if (!hw || !hw->priv || !OPS(hw)->bat_check)
         return UIOX_RTC_BAT_UNKNOWN;
     hw->bat_state = OPS(hw)->bat_check(hw);
     return hw->bat_state;
 }
 
 int uiox_rtc_hw_time_read(uiox_rtc_hw_t *hw,
                            uint8_t *s, uint8_t *m, uint8_t *h,
                            uint8_t *md, uint8_t *mo, uint16_t *yr)
 {
     if (!hw || !hw->priv || !OPS(hw)->time_read) return -EINVAL;
     return OPS(hw)->time_read(hw, s, m, h, md, mo, yr);
 }
 
 int uiox_rtc_hw_time_write(uiox_rtc_hw_t *hw,
                             uint8_t s, uint8_t m, uint8_t h,
                             uint8_t md, uint8_t mo, uint16_t yr)
 {
     if (!hw || !hw->priv || !OPS(hw)->time_write) return -EINVAL;
     return OPS(hw)->time_write(hw, s, m, h, md, mo, yr);
 }
 
 int uiox_rtc_hw_alarm_read(uiox_rtc_hw_t *hw,
                             uint8_t *s, uint8_t *m, uint8_t *h)
 {
     if (!hw || !hw->priv || !OPS(hw)->alarm_read) return -EINVAL;
     return OPS(hw)->alarm_read(hw, s, m, h);
 }
 
 int uiox_rtc_hw_alarm_write(uiox_rtc_hw_t *hw,
                              uint8_t s, uint8_t m, uint8_t h)
 {
     if (!hw || !hw->priv || !OPS(hw)->alarm_write) return -EINVAL;
     return OPS(hw)->alarm_write(hw, s, m, h);
 }
 
 int uiox_rtc_hw_alarm_enable(uiox_rtc_hw_t *hw, bool en)
 {
     if (!hw || !hw->priv || !OPS(hw)->alarm_enable) return -EINVAL;
     return OPS(hw)->alarm_enable(hw, en);
 }
 
 int uiox_rtc_hw_nvram_read(uiox_rtc_hw_t *hw, uint8_t off, uint8_t *val)
 {
     if (!hw || !hw->priv || !OPS(hw)->nvram_read) return -EINVAL;
     return OPS(hw)->nvram_read(hw, off, val);
 }
 
 int uiox_rtc_hw_nvram_write(uiox_rtc_hw_t *hw, uint8_t off, uint8_t val)
 {
     if (!hw || !hw->priv || !OPS(hw)->nvram_write) return -EINVAL;
     return OPS(hw)->nvram_write(hw, off, val);
 }
 