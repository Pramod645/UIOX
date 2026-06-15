/**
 * @file  uiox_nvme_hw.c
 * @brief UIOX NVMe HAL implementation.
 * @date  2026-06-12
 */

 #include "uiox_nvme_hw.h"
 #include <string.h>
 #include <errno.h>
 
 #define OPS(hw) ((const uiox_nvme_hw_ops_t *)(hw)->priv)
 
 int uiox_nvme_hw_init(uiox_nvme_hw_t *hw,
                        const uiox_nvme_hw_ops_t *ops)
 {
     if (!hw || !ops || !ops->init) return -EINVAL;
     hw->priv        = (void *)ops;
     hw->pending_irq = 0u;
     hw->ready       = false;
     hw->cmd_id      = 0u;
     hw->active_nsid = 1u;
     hw->sq_depth    = UIOX_NVME_IO_Q_DEPTH;
     hw->cq_depth    = UIOX_NVME_IO_Q_DEPTH;
     memset(&hw->ctrl_id, 0, sizeof(hw->ctrl_id));
     memset(hw->ns, 0, sizeof(hw->ns));
     return ops->init(hw);
 }
 
 void uiox_nvme_hw_deinit(uiox_nvme_hw_t *hw)
 {
     if (!hw || !hw->priv) return;
     if (OPS(hw)->deinit) OPS(hw)->deinit(hw);
     hw->priv = NULL;
 }
 
 int uiox_nvme_hw_ctrl_reset(uiox_nvme_hw_t *hw)
 {
     if (!hw || !hw->priv) return -EINVAL;
     if (!OPS(hw)->ctrl_reset) return -ENOSYS;
     return OPS(hw)->ctrl_reset(hw);
 }
 
 int uiox_nvme_hw_ctrl_enable(uiox_nvme_hw_t *hw)
 {
     if (!hw || !hw->priv) return -EINVAL;
     if (!OPS(hw)->ctrl_enable) return -ENOSYS;
     int rc = OPS(hw)->ctrl_enable(hw);
     if (rc == 0) hw->ready = true;
     return rc;
 }
 
 uint32_t uiox_nvme_hw_reg_read32(uiox_nvme_hw_t *hw, uint32_t offset)
 {
     if (!hw || !hw->priv || !OPS(hw)->reg_read32) return 0u;
     return OPS(hw)->reg_read32(hw, offset);
 }
 
 uint64_t uiox_nvme_hw_reg_read64(uiox_nvme_hw_t *hw, uint32_t offset)
 {
     if (!hw || !hw->priv || !OPS(hw)->reg_read64) return 0u;
     return OPS(hw)->reg_read64(hw, offset);
 }
 
 void uiox_nvme_hw_reg_write32(uiox_nvme_hw_t *hw,
                                uint32_t offset, uint32_t val)
 {
     if (!hw || !hw->priv || !OPS(hw)->reg_write32) return;
     OPS(hw)->reg_write32(hw, offset, val);
 }
 
 void uiox_nvme_hw_reg_write64(uiox_nvme_hw_t *hw,
                                uint32_t offset, uint64_t val)
 {
     if (!hw || !hw->priv || !OPS(hw)->reg_write64) return;
     OPS(hw)->reg_write64(hw, offset, val);
 }
 
 void uiox_nvme_hw_sq_doorbell(uiox_nvme_hw_t *hw,
                                uint16_t qid, uint16_t tail)
 {
     if (!hw || !hw->priv || !OPS(hw)->sq_doorbell) return;
     OPS(hw)->sq_doorbell(hw, qid, tail);
 }
 
 void uiox_nvme_hw_cq_doorbell(uiox_nvme_hw_t *hw,
                                uint16_t qid, uint16_t head)
 {
     if (!hw || !hw->priv || !OPS(hw)->cq_doorbell) return;
     OPS(hw)->cq_doorbell(hw, qid, head);
 }
 
 int uiox_nvme_hw_admin_cmd(uiox_nvme_hw_t *hw,
                             uiox_nvme_sqe_t *sqe,
                             uiox_nvme_cqe_t *cqe,
                             void *data, uint32_t data_len)
 {
     if (!hw || !hw->priv || !sqe || !cqe) return -EINVAL;
     if (!OPS(hw)->admin_cmd) return -ENOSYS;
     return OPS(hw)->admin_cmd(hw, sqe, cqe, data, data_len);
 }
 
 int uiox_nvme_hw_io_submit(uiox_nvme_hw_t *hw,
                             uint16_t qid, uiox_nvme_sqe_t *sqe)
 {
     if (!hw || !hw->priv || !sqe) return -EINVAL;
     if (!OPS(hw)->io_submit) return -ENOSYS;
     return OPS(hw)->io_submit(hw, qid, sqe);
 }
 
 int uiox_nvme_hw_io_poll(uiox_nvme_hw_t *hw,
                           uint16_t qid, uiox_nvme_cqe_t *cqe)
 {
     if (!hw || !hw->priv || !cqe) return -EINVAL;
     if (!OPS(hw)->io_poll) return -ENOSYS;
     return OPS(hw)->io_poll(hw, qid, cqe);
 }
 
 int uiox_nvme_hw_dma_read(uiox_nvme_hw_t *hw,
                            uint32_t nsid, uint64_t slba,
                            uint8_t *buf, uint32_t nlb)
 {
     if (!hw || !hw->priv || !buf) return -EINVAL;
     if (!OPS(hw)->dma_read) return -ENOSYS;
     return OPS(hw)->dma_read(hw, nsid, slba, buf, nlb);
 }
 
 int uiox_nvme_hw_dma_write(uiox_nvme_hw_t *hw,
                             uint32_t nsid, uint64_t slba,
                             const uint8_t *buf, uint32_t nlb)
 {
     if (!hw || !hw->priv || !buf) return -EINVAL;
     if (!OPS(hw)->dma_write) return -ENOSYS;
     return OPS(hw)->dma_write(hw, nsid, slba, buf, nlb);
 }
 