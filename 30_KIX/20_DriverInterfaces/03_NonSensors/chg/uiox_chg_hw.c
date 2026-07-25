/**
 * @file  uiox_chg_hw.c
 * @brief UIOX Charger HAL implementation.
 * @date  2026-06-11
 */

 #include "uiox_chg_hw.h"
 
 #define OPS(hw) ((const uiox_chg_hw_ops_t *)(hw)->priv)
 
 int uiox_chg_hw_init(uiox_chg_hw_t *hw, const uiox_chg_hw_ops_t *ops)
 {
     if (!hw || !ops || !ops->init) return -EINVAL;
     hw->priv        = (void *)ops;
     hw->pending_irq = 0u;
     hw->fault_flags = UIOX_CHG_FAULT_NONE;
     hw->src         = UIOX_CHG_SRC_NONE;
     hw->chrg_state  = UIOX_CHG_CHRG_IDLE;
     memset(hw->adc_mv, 0, sizeof(hw->adc_mv));
     return ops->init(hw);
 }
 
 void uiox_chg_hw_deinit(uiox_chg_hw_t *hw)
 {
     if (!hw || !hw->priv) return;
     if (OPS(hw)->deinit) OPS(hw)->deinit(hw);
     hw->priv = NULL;
 }
 
 int uiox_chg_hw_reg_read(uiox_chg_hw_t *hw, uint8_t reg, uint8_t *val)
 {
     if (!hw || !hw->priv || !val) return -EINVAL;
     if (!OPS(hw)->reg_read) return -ENOSYS;
     return OPS(hw)->reg_read(hw, reg, val);
 }
 
 int uiox_chg_hw_reg_write(uiox_chg_hw_t *hw, uint8_t reg, uint8_t val)
 {
     if (!hw || !hw->priv) return -EINVAL;
     if (!OPS(hw)->reg_write) return -ENOSYS;
     return OPS(hw)->reg_write(hw, reg, val);
 }
 
 int uiox_chg_hw_reg_rmw(uiox_chg_hw_t *hw,
                          uint8_t reg, uint8_t mask, uint8_t bits)
 {
     if (!hw || !hw->priv) return -EINVAL;
     if (OPS(hw)->reg_rmw)
         return OPS(hw)->reg_rmw(hw, reg, mask, bits);
     /* Fallback: read-modify-write */
     uint8_t val = 0u;
     int rc = uiox_chg_hw_reg_read(hw, reg, &val);
     if (rc < 0) return rc;
     val = (uint8_t)((val & ~mask) | (bits & mask));
     return uiox_chg_hw_reg_write(hw, reg, val);
 }
 
 int uiox_chg_hw_adc_read(uiox_chg_hw_t *hw,
                           uiox_chg_adc_ch_t ch, int32_t *val_mv)
 {
     if (!hw || !hw->priv || !val_mv) return -EINVAL;
     if (!OPS(hw)->adc_read) return -ENOSYS;
     int rc = OPS(hw)->adc_read(hw, ch, val_mv);
     if (rc == 0 && ch < UIOX_CHG_ADC_MAX)
         hw->adc_mv[ch] = *val_mv;
     return rc;
 }
 
 int uiox_chg_hw_set_ichg(uiox_chg_hw_t *hw, uint32_t ma)
 {
     if (!hw || !hw->priv) return -EINVAL;
     if (!OPS(hw)->set_ichg) return -ENOSYS;
     if (ma > hw->ibat_max_ma) ma = hw->ibat_max_ma;
     return OPS(hw)->set_ichg(hw, ma);
 }
 
 int uiox_chg_hw_set_vchg(uiox_chg_hw_t *hw, uint32_t mv)
 {
     if (!hw || !hw->priv) return -EINVAL;
     if (!OPS(hw)->set_vchg) return -ENOSYS;
     if (mv > hw->vbat_max_mv) mv = hw->vbat_max_mv;
     return OPS(hw)->set_vchg(hw, mv);
 }
 
 int uiox_chg_hw_set_iin_lim(uiox_chg_hw_t *hw, uint32_t ma)
 {
     if (!hw || !hw->priv) return -EINVAL;
     if (!OPS(hw)->set_iin_lim) return -ENOSYS;
     if (ma > hw->iin_max_ma) ma = hw->iin_max_ma;
     return OPS(hw)->set_iin_lim(hw, ma);
 }
 
 int uiox_chg_hw_set_vindpm(uiox_chg_hw_t *hw, uint32_t mv)
 {
     if (!hw || !hw->priv) return -EINVAL;
     if (!OPS(hw)->set_vindpm) return -ENOSYS;
     return OPS(hw)->set_vindpm(hw, mv);
 }
 
 int uiox_chg_hw_charge_en(uiox_chg_hw_t *hw, bool en)
 {
     if (!hw || !hw->priv) return -EINVAL;
     if (!OPS(hw)->charge_enable) return -ENOSYS;
     return OPS(hw)->charge_enable(hw, en);
 }
 
 int uiox_chg_hw_otg_en(uiox_chg_hw_t *hw, bool en)
 {
     if (!hw || !hw->priv) return -EINVAL;
     if (!OPS(hw)->otg_enable) return -ENOSYS;
     return OPS(hw)->otg_enable(hw, en);
 }
 
 int uiox_chg_hw_get_status(uiox_chg_hw_t *hw,
                             uiox_chg_chrg_t *chrg,
                             uiox_chg_src_t  *src,
                             uint32_t        *faults)
 {
     if (!hw || !hw->priv) return -EINVAL;
     if (!OPS(hw)->get_status) return -ENOSYS;
     int rc = OPS(hw)->get_status(hw, chrg, src, faults);
     if (rc == 0) {
         hw->chrg_state  = *chrg;
         hw->src         = *src;
         hw->fault_flags = *faults;
     }
     return rc;
 }
 
 int uiox_chg_hw_wdog_reset(uiox_chg_hw_t *hw)
 {
     if (!hw || !hw->priv) return -EINVAL;
     if (!OPS(hw)->wdog_reset) return -ENOSYS;
     return OPS(hw)->wdog_reset(hw);
 }
 
 int uiox_chg_hw_pd_tx(uiox_chg_hw_t *hw,
                        const uint8_t *buf, uint8_t len)
 {
     if (!hw || !hw->priv || !buf) return -EINVAL;
     if (!OPS(hw)->pd_tx_msg) return -ENOSYS;
     return OPS(hw)->pd_tx_msg(hw, buf, len);
 }
 
 int uiox_chg_hw_pd_rx(uiox_chg_hw_t *hw,
                        uint8_t *buf, uint8_t max_len)
 {
     if (!hw || !hw->priv || !buf) return -EINVAL;
     if (!OPS(hw)->pd_rx_msg) return -ENOSYS;
     return OPS(hw)->pd_rx_msg(hw, buf, max_len);
 }
 