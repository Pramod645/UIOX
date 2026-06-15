/**
 * @file  uiox_nvme_demo.c
 * @brief UIOX NVMe SSD stack demo — stub HAL + full stack exercise.
 * @date  2026-06-12
 */

 #include "uiox_nvme_device.h"
 #include <stdio.h>
 #include <string.h>
 #include <stdint.h>
 #include <errno.h>
 
 /* =========================================================================
  * Stub BAR0 register bank and NVM storage
  * ====================================================================== */
 
 static uint32_t s_bar0[0x1100 / 4];   /* BAR0 MMIO: regs + doorbell area */
 
 /* Simulated NVM storage: 128 MB (262144 × 512 B LBAs) */
 #define SIM_NVM_NLBA    262144u
 static uint8_t s_nvm[SIM_NVM_NLBA * UIOX_NVME_LBA_SIZE];
 
 /* SMART log page (512 bytes) */
 static uint8_t s_smart[512];
 
 /* Canned Identify Controller response (4096 bytes) */
 static uint8_t s_id_ctrl[4096];
 /* Canned Identify Namespace response (4096 bytes) */
 static uint8_t s_id_ns[4096];
 /* Canned NS Active List response (4096 bytes) */
 static uint8_t s_id_nslist[4096];
 
 /* Simulation flags */
 static bool s_sim_error        = false;
 static bool s_sim_cfs          = false;
 static bool s_sim_health_warn  = false;
 static bool s_sim_temp_warn    = false;
 
 /* -------------------------------------------------------------------------
  * Preset canned Identify responses
  * ---------------------------------------------------------------------- */
 
 static void stub_preset_identify(void)
 {
     memset(s_id_ctrl,   0, sizeof(s_id_ctrl));
     memset(s_id_ns,     0, sizeof(s_id_ns));
     memset(s_id_nslist, 0, sizeof(s_id_nslist));
     memset(s_smart,     0, sizeof(s_smart));
 
     /* --- Identify Controller --- */
     /* VID: 0x144D (Samsung), SSVID: 0x144D */
     s_id_ctrl[0] = 0x4Du; s_id_ctrl[1] = 0x14u;
     s_id_ctrl[2] = 0x4Du; s_id_ctrl[3] = 0x14u;
     /* SN: "UIOX_NVM_SN_001     " (20 bytes) */
     memcpy(&s_id_ctrl[4], "UIOX_NVM_SN_001     ", 20u);
     /* MN: "UIOX PCIe 4.0 NVMe SSD 128GB           " (40 bytes) */
     memcpy(&s_id_ctrl[24],
            "UIOX PCIe 4.0 NVMe SSD 128GB           ", 40u);
     /* FR: "FW2.0.0 " (8 bytes) */
     memcpy(&s_id_ctrl[64], "FW2.0.0 ", 8u);
     /* MDTS: 6 → max transfer = 2^(6+12) = 256 KB */
     s_id_ctrl[77] = 6u;
     /* CNTLID: 1 */
     s_id_ctrl[78] = 1u;
     /* VER: 2.0.0 */
     s_id_ctrl[83] = 2u; s_id_ctrl[82] = 0u; s_id_ctrl[81] = 0u;
     /* NN: 1 namespace */
     s_id_ctrl[516] = 1u;
     /* ONCS: bit 2 = DSM/TRIM, bit 5 = volatile WC */
     s_id_ctrl[520] = 0x24u;
     /* VWC: byte 525 bit 0 = volatile write cache present */
     s_id_ctrl[525] = 0x01u;
     /* APSTA: byte 603 bit 0 = APST supported */
     s_id_ctrl[603] = 0x01u;
     /* WCTEMP = 70 °C warning, CCTEMP = 85 °C critical */
     s_id_ctrl[610] = 70u;
     s_id_ctrl[612] = 85u;
 
     /* --- Identify Namespace (NSID=1) --- */
     /* NSZE: SIM_NVM_NLBA (LE 64-bit) */
     uint64_t nsze = SIM_NVM_NLBA;
     memcpy(&s_id_ns[0], &nsze, 8u);
     memcpy(&s_id_ns[8], &nsze, 8u);   /* NCAP = NSZE */
     memcpy(&s_id_ns[16], &nsze, 8u);  /* NUSE = NSZE */
     /* FLBAS = 0 → LBAF[0] */
     s_id_ns[26] = 0u;
     /* LBAF[0]: LBADS = 9 → 2^9 = 512 B */
     s_id_ns[128] = 0u;   /* RP=0 */
     s_id_ns[129] = 0u;   /* MS=0 (no metadata) */
     s_id_ns[130] = 0u;
     s_id_ns[131] = 9u;   /* LBADS = 9 */
 
     /* --- Active NS ID list --- */
     /* NSID 1 at word 0 */
     s_id_nslist[0] = 1u;
 
     /* --- SMART log --- */
     /* Critical Warning byte 0: 0 = all good */
     s_smart[0] = 0u;
     /* Composite temperature: 45 °C = 318 K (LE 16-bit at bytes 1–2) */
     uint16_t temp_k = 318u;
     memcpy(&s_smart[1], &temp_k, 2u);
     /* Available spare: 100% */
     s_smart[3] = 100u;
     /* Spare threshold: 10% */
     s_smart[4] = 10u;
     /* Percentage used: 5% */
     s_smart[5] = 5u;
 }
 
 /* =========================================================================
  * Stub HAL ops
  * ====================================================================== */
 
 static int stub_init(uiox_nvme_hw_t *hw)
 {
     (void)hw;
     memset(s_bar0, 0, sizeof(s_bar0));
     memset(s_nvm, 0xA5u, sizeof(s_nvm));
     stub_preset_identify();
 
     /* CAP: MQES=255, CQR=0, TO=10 (5 s), CSS NVM supported, MPSMIN=0 */
     uint64_t cap = (uint64_t)255u                         /* MQES    */
                  | ((uint64_t)10u  << NVME_CAP_TO_SHIFT)  /* TO      */
                  | NVME_CAP_CSS_NVM;
     memcpy(&s_bar0[NVME_REG_CAP_LO / 4], &cap, 8u);
 
     /* VS: 2.0.0 = 0x00020000 */
     s_bar0[NVME_REG_VS / 4] = 0x00020000u;
 
     /* CSTS: RDY=0 initially */
     s_bar0[NVME_REG_CSTS / 4] = 0u;
 
     printf("  [hal] init  %s  BAR0=0x%08lX  IRQ_base=%u  gen=%s\n",
            hw->model, (unsigned long)hw->bar0,
            hw->irq_base, uiox_nvme_pcie_name(hw->pcie_gen));
     return 0;
 }
 static void stub_deinit(uiox_nvme_hw_t *hw) { (void)hw; }
 
 static int stub_ctrl_reset(uiox_nvme_hw_t *hw)
 {
     (void)hw;
     /* Clear CC.EN */
     s_bar0[NVME_REG_CC / 4] &= ~NVME_CC_EN;
     /* Clear CSTS.RDY */
     s_bar0[NVME_REG_CSTS / 4] &= ~NVME_CSTS_RDY;
     printf("  [hal] ctrl_reset: CC.EN=0  CSTS.RDY=0\n");
     return 0;
 }
 
 static int stub_ctrl_enable(uiox_nvme_hw_t *hw)
 {
     (void)hw;
     /* Set CC.EN, configure SQ/CQ entry sizes */
     s_bar0[NVME_REG_CC / 4] = NVME_CC_EN         |
                                 NVME_CC_CSS_NVM    |
                                 NVME_CC_IOSQES_64B |
                                 NVME_CC_IOCQES_16B |
                                 NVME_CC_AMS_RR;
     /* Simulate RDY assertion */
     s_bar0[NVME_REG_CSTS / 4] = NVME_CSTS_RDY;
     printf("  [hal] ctrl_enable: CC=0x%08X  CSTS=0x%08X\n",
            s_bar0[NVME_REG_CC / 4], s_bar0[NVME_REG_CSTS / 4]);
     return 0;
 }
 
 static uint32_t stub_reg_read32(uiox_nvme_hw_t *hw, uint32_t off)
 {
     (void)hw;
     /* Inject CFS if simulated */
     if (off == NVME_REG_CSTS && s_sim_cfs)
         return s_bar0[NVME_REG_CSTS / 4] | NVME_CSTS_CFS;
     uint32_t idx = (off >> 2u) & ((sizeof(s_bar0)/4u) - 1u);
     return s_bar0[idx];
 }
 
 static uint64_t stub_reg_read64(uiox_nvme_hw_t *hw, uint32_t off)
 {
     uint32_t lo = stub_reg_read32(hw, off);
     uint32_t hi = stub_reg_read32(hw, off + 4u);
     return (uint64_t)lo | ((uint64_t)hi << 32u);
 }
 
 static void stub_reg_write32(uiox_nvme_hw_t *hw, uint32_t off, uint32_t val)
 {
     (void)hw;
     uint32_t idx = (off >> 2u) & ((sizeof(s_bar0)/4u) - 1u);
     s_bar0[idx] = val;
     if (off == NVME_REG_CC)
         printf("  [hal] CC <- 0x%08X  EN=%d\n", val, !!(val & NVME_CC_EN));
     if (off == NVME_REG_INTMC)
         printf("  [hal] INTMC <- 0x%08X (unmask)\n", val);
 }
 
 static void stub_reg_write64(uiox_nvme_hw_t *hw, uint32_t off, uint64_t val)
 {
     stub_reg_write32(hw, off,      (uint32_t)( val        & 0xFFFFFFFFu));
     stub_reg_write32(hw, off + 4u, (uint32_t)((val >> 32u)& 0xFFFFFFFFu));
 }
 
 static void stub_sq_doorbell(uiox_nvme_hw_t *hw, uint16_t qid, uint16_t tail)
 {
     (void)hw;
     uint32_t off = NVME_DOORBELL_BASE + (uint32_t)qid * 2u * NVME_DOORBELL_STRIDE;
     s_bar0[off / 4u] = tail;
     printf("  [hal] SQ doorbell  qid=%u  tail=%u\n", qid, tail);
     /* Signal completion IRQ */
     hw->pending_irq |= (qid == 0u)
                         ? UIOX_NVME_IRQ_ADMIN_CQ
                         : UIOX_NVME_IRQ_IO_CQ0;
 }
 
 static void stub_cq_doorbell(uiox_nvme_hw_t *hw, uint16_t qid, uint16_t head)
 {
     (void)hw;
     uint32_t off = NVME_DOORBELL_BASE +
                    ((uint32_t)qid * 2u + 1u) * NVME_DOORBELL_STRIDE;
     s_bar0[off / 4u] = head;
 }
 
 static int stub_admin_cmd(uiox_nvme_hw_t *hw,
                            uiox_nvme_sqe_t *sqe,
                            uiox_nvme_cqe_t *cqe,
                            void *data, uint32_t data_len)
 {
     (void)hw;
     if (s_sim_error) return -EIO;
     memset(cqe, 0, sizeof(*cqe));
     cqe->cid    = sqe->cid;
     cqe->status = NVME_CQE_STATUS_P;  /* phase=1, SC=0 = success */
 
     printf("  [hal] admin_cmd  opc=0x%02X  nsid=%u  cdw10=0x%08X\n",
            sqe->opc, sqe->nsid, sqe->cdw10);
 
     if (!data || data_len == 0u) return 0;
 
     switch (sqe->opc) {
     case NVME_ADMIN_IDENTIFY:
         switch (sqe->cdw10 & 0xFFu) {
         case NVME_IDENTIFY_CNS_CTRL:
             memcpy(data, s_id_ctrl,
                    data_len < 4096u ? data_len : 4096u);
             break;
         case NVME_IDENTIFY_CNS_NS:
             memcpy(data, s_id_ns,
                    data_len < 4096u ? data_len : 4096u);
             break;
         case NVME_IDENTIFY_CNS_NS_LIST:
             memcpy(data, s_id_nslist,
                    data_len < 4096u ? data_len : 4096u);
             break;
         default: break;
         }
         break;
     case NVME_ADMIN_GET_LOG_PAGE:
         if ((sqe->cdw10 & 0xFFu) == NVME_LOG_SMART) {
             /* Inject warnings if simulated */
             uint8_t warn = 0u;
             if (s_sim_health_warn) warn |= 0x01u;  /* spare low    */
             if (s_sim_temp_warn)   warn |= 0x02u;  /* over-temp    */
             s_smart[0] = warn;
             memcpy(data, s_smart,
                    data_len < 512u ? data_len : 512u);
         }
         break;
     case NVME_ADMIN_SET_FEATURES:
         /* cdw0 of completion reflects granted queues for FEAT_NUM_QUEUES */
         if ((sqe->cdw10 & 0xFFu) == NVME_FEAT_NUM_QUEUES)
             cqe->dw0 = sqe->cdw11;  /* Echo back requested counts */
         break;
     default:
         break;
     }
     return 0;
 }
 
 static int stub_io_submit(uiox_nvme_hw_t *hw,
                            uint16_t qid, uiox_nvme_sqe_t *sqe)
 {
     (void)hw; (void)qid;
     printf("  [hal] io_submit  qid=%u  opc=0x%02X  nsid=%u\n",
            qid, sqe->opc, sqe->nsid);
     return s_sim_error ? -EIO : 0;
 }
 
 static int stub_io_poll(uiox_nvme_hw_t *hw,
                          uint16_t qid, uiox_nvme_cqe_t *cqe)
 {
     (void)hw; (void)qid;
     if (s_sim_error) { return -EIO; }
     memset(cqe, 0, sizeof(*cqe));
     cqe->status = NVME_CQE_STATUS_P;  /* success, phase=1 */
     return 1;  /* completion available */
 }
 
 static int stub_dma_read(uiox_nvme_hw_t *hw,
                           uint32_t nsid, uint64_t slba,
                           uint8_t *buf, uint32_t nlb)
 {
     (void)hw; (void)nsid;
     if (s_sim_error) return -EIO;
     if (slba + nlb > SIM_NVM_NLBA) return -ERANGE;
     memcpy(buf, &s_nvm[slba * UIOX_NVME_LBA_SIZE],
            (size_t)nlb * UIOX_NVME_LBA_SIZE);
     printf("  [hal] dma_read   ns=%u  slba=%llu  nlb=%u\n",
            nsid, (unsigned long long)slba, nlb);
     return 0;
 }
 
 static int stub_dma_write(uiox_nvme_hw_t *hw,
                            uint32_t nsid, uint64_t slba,
                            const uint8_t *buf, uint32_t nlb)
 {
     (void)hw; (void)nsid;
     if (s_sim_error) return -EIO;
     if (slba + nlb > SIM_NVM_NLBA) return -ERANGE;
     memcpy(&s_nvm[slba * UIOX_NVME_LBA_SIZE], buf,
            (size_t)nlb * UIOX_NVME_LBA_SIZE);
     printf("  [hal] dma_write  ns=%u  slba=%llu  nlb=%u\n",
            nsid, (unsigned long long)slba, nlb);
     return 0;
 }
 
 static void stub_gpio_w(uiox_nvme_hw_t *hw, uint32_t p, bool v)
 { (void)hw; printf("  [hal] GPIO pin=%u val=%d\n", p, (int)v); }
 
 static bool stub_gpio_r(uiox_nvme_hw_t *hw, uint32_t p)
 { (void)hw; (void)p; return true; }
 
 static void stub_isr(uiox_nvme_hw_t *hw)
 {
     if (!hw) return;
     if (s_sim_error)
         hw->pending_irq |= UIOX_NVME_IRQ_ERROR;
     else
         hw->pending_irq |= UIOX_NVME_IRQ_IO_CQ0;
     printf("  [hal] ISR  pending=0x%08X\n", hw->pending_irq);
 }
 
 static const uiox_nvme_hw_ops_t stub_ops = {
     .init         = stub_init,
     .deinit       = stub_deinit,
     .ctrl_reset   = stub_ctrl_reset,
     .ctrl_enable  = stub_ctrl_enable,
     .reg_read32   = stub_reg_read32,
     .reg_read64   = stub_reg_read64,
     .reg_write32  = stub_reg_write32,
     .reg_write64  = stub_reg_write64,
     .sq_doorbell  = stub_sq_doorbell,
     .cq_doorbell  = stub_cq_doorbell,
     .admin_cmd    = stub_admin_cmd,
     .io_submit    = stub_io_submit,
     .io_poll      = stub_io_poll,
     .dma_read     = stub_dma_read,
     .dma_write    = stub_dma_write,
     .gpio_write   = stub_gpio_w,
     .gpio_read    = stub_gpio_r,
     .isr          = stub_isr,
 };
 
 static uiox_nvme_hw_t s_hw = {
     .bar0      = 0xF0000000uL,
     .irq_base  = 32u,
     .num_irqs  = 4u,
     .caps      = UIOX_NVME_CAP_64BIT_DMA  | UIOX_NVME_CAP_MSI_X     |
                  UIOX_NVME_CAP_MULTI_NS   | UIOX_NVME_CAP_TRIM       |
                  UIOX_NVME_CAP_APST       | UIOX_NVME_CAP_SMART      |
                  UIOX_NVME_CAP_FW_UPDATE  | UIOX_NVME_CAP_VOLATILE_WC|
                  UIOX_NVME_CAP_SGL        | UIOX_NVME_CAP_PERST_GPIO,
     .pcie_gen  = UIOX_NVME_PCIE_GEN4,
     .model     = "UIOX M.2 2280 PCIe 4.0 x4 NVMe Controller",
     .active_nsid = 1u,
     .sq_depth  = UIOX_NVME_IO_Q_DEPTH,
     .cq_depth  = UIOX_NVME_IO_Q_DEPTH,
 };
 
 /* =========================================================================
  * Event callback
  * ====================================================================== */
 
 static void on_nvme_event(uiox_nvme_ev_t ev,
                            const uiox_nvme_evt_t *data, void *ctx)
 {
     (void)ctx;
     if (data)
         printf("  [event] %-16s  ns=%u  slba=%llu  nlb=%u"
                "  status=%d\n",
                uiox_nvme_ev_name(ev),
                data->nsid,
                (unsigned long long)data->slba,
                data->nlb,
                data->status);
     else
         printf("  [event] %-16s\n", uiox_nvme_ev_name(ev));
 }
 
 /* =========================================================================
  * main
  * ====================================================================== */
 
 int main(void)
 {
     printf("=== UIOX NVMe SSD PCIe/NVMe Stack Demo ===\n\n");
 
     /* ------------------------------------------------------------------ */
     printf("--- Open ---\n");
     uiox_nvme_device_t      dev;
     uiox_nvme_open_params_t p = {
         .hw      = &s_hw,
         .hw_ops  = &stub_ops,
         .evt_cb  = on_nvme_event,
     };
     int rc = uiox_nvme_open(&dev, &p);
     if (rc < 0) { printf("[error] open: %d\n", rc); return 1; }
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Start (reset → enable → Identify → IO-Q → NS → features)"
            " ---\n");
     rc = uiox_nvme_start(&dev);
     printf("  State: %s  rc=%d\n",
            uiox_nvme_state_name(dev.subsys.state), rc);
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Device info ---\n");
     uiox_nvme_print_info(&dev);
 
     /* ------------------------------------------------------------------ */
     printf("\n--- NVMe version + CAP register ---\n");
     {
         uint32_t vs  = uiox_nvme_hw_reg_read32(&s_hw, NVME_REG_VS);
         uint64_t cap = uiox_nvme_hw_reg_read64(&s_hw, NVME_REG_CAP_LO);
         uint32_t cc  = uiox_nvme_hw_reg_read32(&s_hw, NVME_REG_CC);
         uint32_t csts= uiox_nvme_hw_reg_read32(&s_hw, NVME_REG_CSTS);
         printf("  VS    = 0x%08X  (NVMe v%u.%u)\n",
                vs, (vs >> 16u) & 0xFFu, (vs >> 8u) & 0xFFu);
         printf("  CAP   = 0x%016llX  MQES=%u\n",
                (unsigned long long)cap, (uint32_t)(cap & NVME_CAP_MQES_MASK));
         printf("  CC    = 0x%08X  EN=%d\n", cc, !!(cc & NVME_CC_EN));
         printf("  CSTS  = 0x%08X  RDY=%d  CFS=%d\n",
                csts, !!(csts & NVME_CSTS_RDY), !!(csts & NVME_CSTS_CFS));
     }
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Write LBAs 0–7 (8 × 512 B = 4 KB) ---\n");
     static uint8_t tx[8 * UIOX_NVME_LBA_SIZE];
     for (uint32_t i = 0u; i < sizeof(tx); i++)
         tx[i] = (uint8_t)(i & 0xFFu);
     rc = uiox_nvme_write(&dev, 1u, 0u, tx, 8u);
     printf("  write rc=%d\n", rc);
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Read back LBAs 0–7 ---\n");
     static uint8_t rx[8 * UIOX_NVME_LBA_SIZE];
     rc = uiox_nvme_read(&dev, 1u, 0u, rx, 8u);
     printf("  read  rc=%d  match=%s\n",
            rc, (memcmp(tx, rx, sizeof(tx)) == 0) ? "YES" : "NO");
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Write LBAs 100–103 (multi-block) ---\n");
     static uint8_t tx2[4 * UIOX_NVME_LBA_SIZE];
     memset(tx2, 0xBEu, sizeof(tx2));
     rc = uiox_nvme_write(&dev, 1u, 100u, tx2, 4u);
     printf("  write rc=%d\n", rc);
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Flush namespace 1 ---\n");
     rc = uiox_nvme_flush(&dev, 1u);
     printf("  flush rc=%d\n", rc);
 
     /* ------------------------------------------------------------------ */
     printf("\n--- TRIM (DSM Deallocate) LBAs 200–263 ---\n");
     rc = uiox_nvme_trim(&dev, 1u, 200u, 64u);
     printf("  trim  rc=%d\n", rc);
 
     /* ------------------------------------------------------------------ */
     printf("\n--- SMART log page read ---\n");
     static uint8_t smart_buf[512];
     rc = uiox_nvme_smart_log(&dev, smart_buf);
     printf("  smart rc=%d  critical_warn=0x%02X  temp=%u K"
            "  spare=%u%%  used=%u%%\n",
            rc, smart_buf[0],
            (uint32_t)(smart_buf[1] | ((uint32_t)smart_buf[2] << 8u)),
            smart_buf[3], smart_buf[5]);
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Block buffer pool alloc / write-back ---\n");
     uiox_nvme_blk_t *blk = uiox_nvme_blk_alloc();
     if (blk) {
         memset(blk->data, 0xCDu,
                UIOX_NVME_SECTORS_PER_BLK * UIOX_NVME_LBA_SIZE);
         blk->nsid  = 1u;
         blk->slba  = 500u;
         blk->nlb   = UIOX_NVME_SECTORS_PER_BLK;
         blk->dirty = true;
         rc = uiox_nvme_write(&dev, blk->nsid, blk->slba,
                               blk->data, blk->nlb);
         printf("  dirty write-back rc=%d  blk_free=%u\n",
                rc, uiox_nvme_blk_free_cnt());
         uiox_nvme_blk_free(blk);
     }
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Manual command record: Identify NS 1 ---\n");
     {
         uiox_nvme_cmd_t *cmd = uiox_nvme_cmd_alloc();
         if (cmd) {
             static uint8_t id_buf[4096];
             cmd->sqe.opc   = NVME_ADMIN_IDENTIFY;
             cmd->sqe.nsid  = 1u;
             cmd->sqe.cdw10 = NVME_IDENTIFY_CNS_NS;
             rc = uiox_nvme_if_admin(&dev.subsys.nif, cmd,
                                      id_buf, sizeof(id_buf));
             uint64_t nsze;
             memcpy(&nsze, &id_buf[0], 8u);
             printf("  Identify NS rc=%d  NSZE=%llu  state=%d\n",
                    rc, (unsigned long long)nsze, (int)cmd->state);
             uiox_nvme_cmd_free(cmd);
         }
     }
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Simulate health warning (spare below threshold) ---\n");
     s_sim_health_warn  = true;
     dev.subsys.health_poll_ms = UIOX_NVME_HEALTH_INTERVAL_MS;
     uiox_nvme_tick(&dev, 100u);
     s_sim_health_warn  = false;
     s_smart[0]         = 0u;
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Simulate temperature warning ---\n");
     s_sim_temp_warn    = true;
     dev.subsys.health_poll_ms = UIOX_NVME_HEALTH_INTERVAL_MS;
     uiox_nvme_tick(&dev, 110u);
     s_sim_temp_warn    = false;
     s_smart[0]         = 0u;
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Simulate controller fatal status (CFS) ---\n");
     s_sim_cfs = true;
     uiox_nvme_tick(&dev, 120u);
     printf("  State after CFS: %s\n",
            uiox_nvme_state_name(dev.subsys.state));
     /* Recover */
     s_sim_cfs                    = false;
     s_hw.pending_irq             = 0u;
     dev.subsys.state             = UIOX_NVME_STATE_READY;
     dev.subsys.proto.initialized = true;
     s_bar0[NVME_REG_CSTS / 4]   = NVME_CSTS_RDY;
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Simulate I/O error ---\n");
     s_sim_error = true;
     stub_isr(&s_hw);
     uiox_nvme_tick(&dev, 130u);
     printf("  State after error: %s\n",
            uiox_nvme_state_name(dev.subsys.state));
     s_sim_error                  = false;
     s_hw.pending_irq             = 0u;
     dev.subsys.state             = UIOX_NVME_STATE_READY;
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Format namespace 1 (LBAF=0, 512 B LBAs) ---\n");
     rc = uiox_nvme_format_ns(&dev, 1u, 0u);
     printf("  format rc=%d\n", rc);
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Tick loop (5 × 10 ms) ---\n");
     for (uint32_t t = 200u; t <= 240u; t += 10u)
         uiox_nvme_tick(&dev, t);
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Namespace info ---\n");
     {
         const uiox_nvme_ns_t *ns = uiox_nvme_ns_info(&dev, 1u);
         if (ns)
             printf("  NS[1]: nsze=%llu  lba_size=%u B  "
                    "capacity=%llu GB\n",
                    (unsigned long long)ns->nsze,
                    ns->lba_size,
                    (unsigned long long)
                    (uiox_nvme_capacity(&dev, 1u) >> 30u));
     }
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Controller identity ---\n");
     {
         const uiox_nvme_ctrl_id_t *id = uiox_nvme_ctrl_id(&dev);
         if (id) {
             printf("  Model    : %.40s\n", id->model);
             printf("  Serial   : %.20s\n", id->serial);
             printf("  Firmware : %.8s\n",  id->fw_rev);
             printf("  VID      : 0x%04X\n", id->vid);
         }
     }
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Statistics ---\n");
     uiox_nvme_print_stats(&dev);
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Final device info ---\n");
     uiox_nvme_print_info(&dev);
 
     /* ------------------------------------------------------------------ */
     printf("\n--- Stop and close (flush all NS + shutdown + power-off) ---\n");
     uiox_nvme_stop(&dev);
     printf("  State: %s\n", uiox_nvme_state_name(dev.subsys.state));
     uiox_nvme_close(&dev);
     printf("  Device: CLOSED\n");
 
     printf("\n=== UIOX NVMe SSD PCIe/NVMe Demo complete ===\n");
     return 0;
 }
 