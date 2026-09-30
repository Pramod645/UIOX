/**
 * @file    uiox_mic_hw.c
 * @brief   UIOX Microphone HAL implementation.
 * @date    2026-06-03
 */

 #include "uiox_mic_hw.h"
 
 int uiox_mic_hw_init(uiox_mic_hw_t *hw, const uiox_mic_hw_ops_t *ops)
 {
     if (!hw || !ops || !ops->init) return -EINVAL;
     hw->priv          = (void *)ops;
     hw->capturing     = false;
     hw->muted         = false;
     hw->gain_db       = 20u;
     hw->overrun_count = 0u;
     hw->dma_half      = false;
     hw->dma_done      = false;
     hw->dma_head      = 0;
     hw->dma_tail      = 0;
     return ops->init(hw);
 }
 
 void uiox_mic_hw_deinit(uiox_mic_hw_t *hw)
 {
     if (!hw || !hw->priv) return;
     const uiox_mic_hw_ops_t *ops = (const uiox_mic_hw_ops_t *)hw->priv;
     if (ops->deinit) ops->deinit(hw);
     hw->priv = NULL;
 }
 
 int uiox_mic_hw_start(uiox_mic_hw_t *hw)
 {
     if (!hw || !hw->priv) return -EINVAL;
     const uiox_mic_hw_ops_t *ops = (const uiox_mic_hw_ops_t *)hw->priv;
     if (!ops->start) return -ENOSYS;
     int rc = ops->start(hw);
     if (rc == 0) hw->capturing = true;
     return rc;
 }
 
 void uiox_mic_hw_stop(uiox_mic_hw_t *hw)
 {
     if (!hw || !hw->priv) return;
     const uiox_mic_hw_ops_t *ops = (const uiox_mic_hw_ops_t *)hw->priv;
     if (ops->stop) ops->stop(hw);
     hw->capturing = false;
 }
 
 int uiox_mic_hw_set_fmt(uiox_mic_hw_t *hw, const uiox_mic_audio_fmt_t *fmt)
 {
     if (!hw || !hw->priv || !fmt) return -EINVAL;
     const uiox_mic_hw_ops_t *ops = (const uiox_mic_hw_ops_t *)hw->priv;
     if (!ops->set_format) return -ENOSYS;
     int rc = ops->set_format(hw, fmt);
     if (rc == 0) memcpy(&hw->fmt, fmt, sizeof(*fmt));
     return rc;
 }
 
 int uiox_mic_hw_set_gain(uiox_mic_hw_t *hw, uint8_t gain_db)
 {
     if (!hw || !hw->priv) return -EINVAL;
     if (gain_db > 40u) gain_db = 40u;
     const uiox_mic_hw_ops_t *ops = (const uiox_mic_hw_ops_t *)hw->priv;
     if (!ops->set_gain) return -ENOSYS;
     int rc = ops->set_gain(hw, gain_db);
     if (rc == 0) hw->gain_db = gain_db;
     return rc;
 }
 
 int uiox_mic_hw_set_mute(uiox_mic_hw_t *hw, bool mute)
 {
     if (!hw || !hw->priv) return -EINVAL;
     const uiox_mic_hw_ops_t *ops = (const uiox_mic_hw_ops_t *)hw->priv;
     if (!ops->set_mute) return -ENOSYS;
     int rc = ops->set_mute(hw, mute);
     if (rc == 0) hw->muted = mute;
     return rc;
 }
 
 int uiox_mic_hw_dma_submit(uiox_mic_hw_t *hw,
                             uintptr_t phys, uint32_t bytes, bool last)
 {
     if (!hw || !hw->priv) return -EINVAL;
     const uiox_mic_hw_ops_t *ops = (const uiox_mic_hw_ops_t *)hw->priv;
     if (!ops->dma_submit) return -ENOSYS;
     return ops->dma_submit(hw, phys, bytes, last);
 }
 