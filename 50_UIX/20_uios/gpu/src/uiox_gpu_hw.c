/**
 * @file    uiox_gpu_hw.c
 * @brief   UIOX GPU HAL — generic hardware lifecycle management.
 * @date    2026-06-01
 */

 #include "uiox_gpu_hw.h"
 #include <string.h>
 #include <errno.h>
 
 int uiox_gpu_hw_init(uiox_gpu_hw_t *hw, const uiox_gpu_hw_ops_t *ops)
 {
     if (!hw || !ops || !ops->init) return -EINVAL;
     hw->priv             = (void *)ops;
     hw->powered          = false;
     hw->fault            = false;
     hw->cmd_head         = 0;
     hw->cmd_tail         = 0;
     hw->fence_seqno      = 0;
     hw->submitted_seqno  = 0;
     return ops->init(hw);
 }
 
 void uiox_gpu_hw_deinit(uiox_gpu_hw_t *hw)
 {
     if (!hw || !hw->priv) return;
     const uiox_gpu_hw_ops_t *ops = (const uiox_gpu_hw_ops_t *)hw->priv;
     if (ops->deinit) ops->deinit(hw);
     hw->priv = NULL;
 }
 
 int uiox_gpu_hw_power_on(uiox_gpu_hw_t *hw)
 {
     if (!hw || !hw->priv) return -EINVAL;
     const uiox_gpu_hw_ops_t *ops = (const uiox_gpu_hw_ops_t *)hw->priv;
     if (!ops->power_on) return -ENOSYS;
     int rc = ops->power_on(hw);
     if (rc == 0) hw->powered = true;
     return rc;
 }
 
 void uiox_gpu_hw_power_off(uiox_gpu_hw_t *hw)
 {
     if (!hw || !hw->priv) return;
     const uiox_gpu_hw_ops_t *ops = (const uiox_gpu_hw_ops_t *)hw->priv;
     if (ops->power_off) ops->power_off(hw);
     hw->powered = false;
 }
 
 int uiox_gpu_hw_cmd_submit(uiox_gpu_hw_t *hw,
                             uintptr_t phys, uint32_t size,
                             uint32_t fence_val)
 {
     if (!hw || !hw->priv) return -EINVAL;
     const uiox_gpu_hw_ops_t *ops = (const uiox_gpu_hw_ops_t *)hw->priv;
     if (!ops->cmd_submit) return -ENOSYS;
     hw->submitted_seqno = fence_val;
     return ops->cmd_submit(hw, phys, size, fence_val);
 }
 
 int uiox_gpu_hw_fence_wait(uiox_gpu_hw_t *hw,
                             uint32_t fence_val, uint32_t timeout_ms)
 {
     if (!hw || !hw->priv) return -EINVAL;
     const uiox_gpu_hw_ops_t *ops = (const uiox_gpu_hw_ops_t *)hw->priv;
     if (!ops->fence_wait) return -ENOSYS;
     return ops->fence_wait(hw, fence_val, timeout_ms);
 }
 
 int uiox_gpu_hw_shader_load(uiox_gpu_hw_t *hw,
                              const uint8_t *binary, uint32_t size,
                              uint32_t stage_flags,
                              uint32_t *shader_id_out)
 {
     if (!hw || !hw->priv || !binary) return -EINVAL;
     const uiox_gpu_hw_ops_t *ops = (const uiox_gpu_hw_ops_t *)hw->priv;
     if (!ops->shader_load) return -ENOSYS;
     return ops->shader_load(hw, binary, size, stage_flags, shader_id_out);
 }
 