/**
 * @file  uiox_nvme_if.h
 * @brief UIOX NVMe interface driver — SQ/CQ, doorbell, MSI-X, completion.
 * @date  2026-06-12
 */

 #ifndef UIOX_NVME_IF_H
 #define UIOX_NVME_IF_H
 
 #include "uiox_nvme_hw.h"
 #include "uiox_nvme_buf.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 typedef struct {
     uint64_t  lbas_read;
     uint64_t  lbas_written;
     uint64_t  bytes_read;
     uint64_t  bytes_written;
     uint32_t  admin_cmds;
     uint32_t  io_cmds;
     uint32_t  flushes;
     uint32_t  trims;
     uint32_t  irq_count;
     uint32_t  errors;
     uint32_t  retries;
 } uiox_nvme_if_stats_t;
 
 typedef struct {
     uiox_nvme_hw_t      *hw;
     uiox_nvme_if_stats_t stats;
     bool                 primed;
     uint16_t             io_sq_tail;    /**< Current I/O SQ tail pointer  */
     uint16_t             io_cq_head;    /**< Current I/O CQ head pointer  */
     uint8_t              io_cq_phase;   /**< Expected phase tag           */
 } uiox_nvme_if_t;
 
 int  uiox_nvme_if_config      (uiox_nvme_if_t *nif, uiox_nvme_hw_t *hw);
 int  uiox_nvme_if_start       (uiox_nvme_if_t *nif);
 void uiox_nvme_if_stop        (uiox_nvme_if_t *nif);
 
 /* Admin commands (blocking) */
 int  uiox_nvme_if_admin       (uiox_nvme_if_t *nif,
                                 uiox_nvme_cmd_t *cmd,
                                 void *data, uint32_t data_len);
 
 /* I/O command submit + poll */
 int  uiox_nvme_if_io_submit   (uiox_nvme_if_t *nif,
                                 uiox_nvme_cmd_t *cmd);
 int  uiox_nvme_if_io_complete (uiox_nvme_if_t *nif,
                                 uiox_nvme_cmd_t *cmd);
 
 /* High-level DMA read / write */
 int  uiox_nvme_if_read        (uiox_nvme_if_t *nif,
                                 uint32_t nsid, uint64_t slba,
                                 uint8_t *buf, uint32_t nlb);
 int  uiox_nvme_if_write       (uiox_nvme_if_t *nif,
                                 uint32_t nsid, uint64_t slba,
                                 const uint8_t *buf, uint32_t nlb);
 int  uiox_nvme_if_flush       (uiox_nvme_if_t *nif, uint32_t nsid);
 int  uiox_nvme_if_trim        (uiox_nvme_if_t *nif,
                                 uint32_t nsid, uint64_t slba,
                                 uint32_t nlb);
 
 /* IRQ handler */
 uiox_nvme_evt_t *uiox_nvme_if_irq_handle(uiox_nvme_if_t *nif,
                                           uint32_t now_ms);
 
 void uiox_nvme_if_stats_get   (const uiox_nvme_if_t *nif,
                                 uiox_nvme_if_stats_t *out);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_NVME_IF_H */
 