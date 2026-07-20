/**
 * @file    uiox_spk_hw.c
 * @brief   UIOX Speaker HAL — generic hardware lifecycle management.
 * @date    2026-06-01
 */

 #include "uiox_spk_hw.h"
 #include <string.h>
 #include <errno.h>
 
 int uiox_spk_hw_init(uiox_spk_hw_t *hw, const uiox_spk_hw_ops_t *ops)
 {
     if (!hw || !ops || !ops->init) return -EINVAL;
     hw->priv          = (void *)ops;
     hw->playing       = false;
     hw->muted         = false;
     hw->volume        = 80u;
     hw->underrun_count= 0u;
     hw->dma_half      = false;
     hw->dma_done      = false;
     hw->dma_head      = 0;
     hw->dma_tail      = 0;
     return ops->init(hw);
 }
 
 void uiox_spk_hw_deinit(uiox_spk_hw_t *hw)
 {
     if (!hw || !hw->priv) return;
     const uiox_spk_hw_ops_t *ops = (const uiox_spk_hw_ops_t *)hw->priv;
     if (ops->deinit) ops->deinit(hw);
     hw->priv = NULL;
 }
 
 int uiox_spk_hw_start(uiox_spk_hw_t *hw)
 {
     if (!hw || !hw->priv) return -EINVAL;
     const uiox_spk_hw_ops_t *ops = (const uiox_spk_hw_ops_t *)hw->priv;
     if (!ops->start) return -ENOSYS;
     int rc = ops->start(hw);
     if (rc == 0) hw->playing = true;
     return rc;
 }
 
 void uiox_spk_hw_stop(uiox_spk_hw_t *hw)
 {
     if (!hw || !hw->priv) return;
     const uiox_spk_hw_ops_t *ops = (const uiox_spk_hw_ops_t *)hw->priv;
     if (ops->stop) ops->stop(hw);
     hw->playing = false;
 }
 
 int uiox_spk_hw_set_fmt(uiox_spk_hw_t *hw, const uiox_spk_audio_fmt_t *fmt)
 {
     if (!hw || !hw->priv || !fmt) return -EINVAL;
     const uiox_spk_hw_ops_t *ops = (const uiox_spk_hw_ops_t *)hw->priv;
     if (!ops->set_format) return -ENOSYS;
     int rc = ops->set_format(hw, fmt);
     if (rc == 0) memcpy(&hw->fmt, fmt, sizeof(*fmt));
     return rc;
 }
 
 int uiox_spk_hw_set_volume(uiox_spk_hw_t *hw, uint8_t vol_pct)
 {
     if (!hw || !hw->priv) return -EINVAL;
     if (vol_pct > 100u) vol_pct = 100u;
     const uiox_spk_hw_ops_t *ops = (const uiox_spk_hw_ops_t *)hw->priv;
     if (!ops->set_volume) return -ENOSYS;
     int rc = ops->set_volume(hw, vol_pct);
     if (rc == 0) hw->volume = vol_pct;
     return rc;
 }
 
 int uiox_spk_hw_set_mute(uiox_spk_hw_t *hw, bool mute)
 {
     if (!hw || !hw->priv) return -EINVAL;
     const uiox_spk_hw_ops_t *ops = (const uiox_spk_hw_ops_t *)hw->priv;
     if (!ops->set_mute) return -ENOSYS;
     int rc = ops->set_mute(hw, mute);
     if (rc == 0) hw->muted = mute;
     return rc;
 }
 
 int uiox_spk_hw_dma_submit(uiox_spk_hw_t *hw,
                              uintptr_t phys, uint32_t bytes, bool last)
 {
     if (!hw || !hw->priv) return -EINVAL;
     const uiox_spk_hw_ops_t *ops = (const uiox_spk_hw_ops_t *)hw->priv;
     if (!ops->dma_submit) return -ENOSYS;
     return ops->dma_submit(hw, phys, bytes, last);
 }
 