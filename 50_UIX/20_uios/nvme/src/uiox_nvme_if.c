/**
 * @file  uiox_nvme_if.c
 * @brief UIOX NVMe interface driver — SQ/CQ, doorbell, IRQ dispatch.
 * @date  2026-06-12
 */

 #include "uiox_nvme_if.h"
 #include <string.h>
 #include <errno.h>
 
 int uiox_nvme_if_config(uiox_nvme_if_t *nif, uiox_nvme_hw_t *hw)
 {
     if (!nif || !hw) return -EINVAL;
     memset(nif, 0, sizeof(*nif));
     nif->hw          = hw;
     nif->primed      = true;
     nif->io_cq_phase = 1u;  /* phase starts at 1 */
     uiox_nvme_buf_init();
     return 0;
 }
 
 int uiox_nvme_if_start(uiox_nvme_if_t *nif)
 {
     if (!nif || !nif->primed) return -EINVAL;
     int rc = uiox_nvme_hw_ctrl_reset(nif->hw);
     if (rc < 0) return rc;
     rc = uiox_nvme_hw_ctrl_enable(nif->hw);
     if (rc < 0) return rc;
     /* Unmask all MSI-X vectors */
     uiox_nvme_hw_reg_write32(nif->hw, NVME_REG_INTMC, 0xFFFFFFFFu);
     return 0;
 }
 
 void uiox_nvme_if_stop(uiox_nvme_if_t *nif)
 {
     if (!nif) return;
     /* Mask all interrupts */
     uiox_nvme_hw_reg_write32(nif->hw, NVME_REG_INTMS, 0xFFFFFFFFu);
 }
 
 int uiox_nvme_if_admin(uiox_nvme_if_t *nif,
                         uiox_nvme_cmd_t *cmd,
                         void *data, uint32_t data_len)
 {
     if (!nif || !cmd) return -EINVAL;
     /* Assign command ID */
     cmd->sqe.cid = nif->hw->cmd_id++;
     int rc = uiox_nvme_hw_admin_cmd(nif->hw, &cmd->sqe, &cmd->cqe,
                                      data, data_len);
     nif->stats.admin_cmds++;
     if (rc < 0) {
         nif->stats.errors++;
         cmd->state  = UIOX_NVME_CMD_ERROR;
         cmd->status = rc;
     } else {
         cmd->state  = UIOX_NVME_CMD_DONE;
         cmd->status = 0;
     }
     return rc;
 }
 
 int uiox_nvme_if_io_submit(uiox_nvme_if_t *nif, uiox_nvme_cmd_t *cmd)
 {
     if (!nif || !cmd) return -EINVAL;
     cmd->sqe.cid = nif->hw->cmd_id++;
     cmd->qid     = 1u;  /* Use I/O queue 1 */
     int rc = uiox_nvme_hw_io_submit(nif->hw, cmd->qid, &cmd->sqe);
     if (rc < 0) {
         nif->stats.errors++; cmd->state = UIOX_NVME_CMD_ERROR;
     } else {
         nif->stats.io_cmds++;
         /* Ring SQ doorbell */
         nif->io_sq_tail = (uint16_t)((nif->io_sq_tail + 1u) %
                            nif->hw->sq_depth);
         uiox_nvme_hw_sq_doorbell(nif->hw, cmd->qid, nif->io_sq_tail);
     }
     return rc;
 }
 
 int uiox_nvme_if_io_complete(uiox_nvme_if_t *nif, uiox_nvme_cmd_t *cmd)
 {
     if (!nif || !cmd) return -EINVAL;
     int rc = uiox_nvme_hw_io_poll(nif->hw, cmd->qid, &cmd->cqe);
     if (rc > 0) {
         /* Update CQ head and ring doorbell */
         nif->io_cq_head = (uint16_t)((nif->io_cq_head + 1u) %
                            nif->hw->cq_depth);
         uiox_nvme_hw_cq_doorbell(nif->hw, cmd->qid, nif->io_cq_head);
         /* Check phase tag */
         if ((cmd->cqe.status & NVME_CQE_STATUS_P) != nif->io_cq_phase)
             return 0;   /* Not our entry yet */
         /* Toggle phase when we wrap */
         if (nif->io_cq_head == 0u)
             nif->io_cq_phase ^= 1u;
         cmd->state  = (NVME_CQE_SC(&cmd->cqe) == 0u)
                       ? UIOX_NVME_CMD_DONE : UIOX_NVME_CMD_ERROR;
         cmd->status = (cmd->state == UIOX_NVME_CMD_DONE) ? 0 : -EIO;
         return 1;
     }
     return 0;
 }
 
 int uiox_nvme_if_read(uiox_nvme_if_t *nif,
                        uint32_t nsid, uint64_t slba,
                        uint8_t *buf, uint32_t nlb)
 {
     if (!nif || !buf || !nlb) return -EINVAL;
     int rc = uiox_nvme_hw_dma_read(nif->hw, nsid, slba, buf, nlb);
     if (rc == 0) {
         nif->stats.lbas_read  += nlb;
         nif->stats.bytes_read +=
             (uint64_t)nlb * UIOX_NVME_LBA_SIZE;
     } else {
         nif->stats.errors++;
     }
     return rc;
 }
 
 int uiox_nvme_if_write(uiox_nvme_if_t *nif,
                         uint32_t nsid, uint64_t slba,
                         const uint8_t *buf, uint32_t nlb)
 {
     if (!nif || !buf || !nlb) return -EINVAL;
     int rc = uiox_nvme_hw_dma_write(nif->hw, nsid, slba, buf, nlb);
     if (rc == 0) {
         nif->stats.lbas_written  += nlb;
         nif->stats.bytes_written +=
             (uint64_t)nlb * UIOX_NVME_LBA_SIZE;
     } else {
         nif->stats.errors++;
     }
     return rc;
 }
 
 int uiox_nvme_if_flush(uiox_nvme_if_t *nif, uint32_t nsid)
 {
     if (!nif) return -EINVAL;
     uiox_nvme_cmd_t *cmd = uiox_nvme_cmd_alloc();
     if (!cmd) return -ENOMEM;
     cmd->sqe.opc  = NVME_IO_FLUSH;
     cmd->sqe.nsid = nsid;
     int rc = uiox_nvme_if_io_submit(nif, cmd);
     if (rc == 0) {
         /* Blocking poll for flush completion */
         for (uint32_t i = 0u; i < 10000u && rc == 0; i++)
             rc = uiox_nvme_if_io_complete(nif, cmd);
         nif->stats.flushes++;
     }
     uiox_nvme_cmd_free(cmd);
     return rc > 0 ? 0 : rc;
 }
 
 int uiox_nvme_if_trim(uiox_nvme_if_t *nif,
                        uint32_t nsid, uint64_t slba, uint32_t nlb)
 {
     if (!nif) return -EINVAL;
     uiox_nvme_cmd_t *cmd = uiox_nvme_cmd_alloc();
     if (!cmd) return -ENOMEM;
     cmd->sqe.opc   = NVME_IO_DSM;
     cmd->sqe.nsid  = nsid;
     cmd->sqe.cdw10 = 0x00000000u;  /* NR = 0 (1 range) */
     cmd->sqe.cdw11 = 0x00000004u;  /* AD = 1 (deallocate) */
     /* In real driver: build DSM range descriptor via PRP */
     (void)slba; (void)nlb;
     int rc = uiox_nvme_if_io_submit(nif, cmd);
     if (rc == 0) {
         for (uint32_t i = 0u; i < 10000u && rc == 0; i++)
             rc = uiox_nvme_if_io_complete(nif, cmd);
         nif->stats.trims++;
     }
     uiox_nvme_cmd_free(cmd);
     return rc > 0 ? 0 : rc;
 }
 
 uiox_nvme_evt_t *uiox_nvme_if_irq_handle(uiox_nvme_if_t *nif,
                                            uint32_t now_ms)
 {
     if (!nif) return NULL;
     uint32_t irq = nif->hw->pending_irq;
     if (!irq) return NULL;
     nif->hw->pending_irq = 0u;
     nif->stats.irq_count++;
 
     uiox_nvme_evt_t *e = uiox_nvme_evt_alloc();
     if (!e) { nif->stats.errors++; return NULL; }
     e->timestamp_ms = now_ms;
 
     if (irq & UIOX_NVME_IRQ_ERROR) {
         e->type   = UIOX_NVME_EVT_ERROR;
         e->status = -EIO;
         nif->stats.errors++;
     } else if (irq & UIOX_NVME_IRQ_IO_CQ0) {
         e->type   = UIOX_NVME_EVT_CMD_DONE;
         e->status = 0;
     } else {
         e->type   = UIOX_NVME_EVT_CMD_DONE;
         e->status = 0;
     }
     return e;
 }
 
 void uiox_nvme_if_stats_get(const uiox_nvme_if_t *nif,
                               uiox_nvme_if_stats_t *out)
 { if (!nif || !out) return; memcpy(out, &nif->stats, sizeof(*out)); }
 